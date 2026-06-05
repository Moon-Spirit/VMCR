// ===========================================================================
// src/main/cpp/vulkan/vk_renderer_hpp.cpp
// VulkanRenderer 的 Vulkan-Hpp + VMA + vkbootstrap 增强实现
// 仅当 third_party 子模块 (Vulkan-Hpp / VMA / vkbootstrap) 存在时编译
//
// 用法:
//   cmake -DVMCR_USE_VULKAN_HPP=ON   (默认 ON, 子模块缺失时自动降级)
//   cmake -DVMCR_USE_VMA=ON          (默认 ON)
//   cmake -DVMCR_USE_VKBOOTSTRAP=ON  (默认 ON)
//
// 架构:
//   - vkb::Instance / vkb::Device / vkb::Swapchain (vkbootstrap)
//   - vk::raii::Instance / vk::raii::Device (Vulkan-Hpp RAII)
//   - VmaAllocator (VMA, GPU 显存分配)
// ===========================================================================
#include "vmcr/backend.h"
#include "vmcr/vk_hpp.h"
#include "vmcr/log.h"

#if !VMCR_HAS_VULKAN_HPP
// 第三方库缺失, 此模块不编译
namespace vmcr::vk { void* renderer_hpp_disabled = nullptr; }
#else

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#if VMCR_HAS_VKBOOTSTRAP
    #include <vkbootstrap/VkBootstrap.h>
    namespace vkb = vkb;
#endif

#if VMCR_HAS_VMA
    #include <vk_mem_alloc.h>
#endif

#include <android/native_window.h>
#include <vector>
#include <memory>

namespace vmcr::vk {

constexpr const char* kTag = log::kTagVK;

class VulkanRendererHpp : public vmcr::IRenderer {
public:
    VulkanRendererHpp() = default;
    ~VulkanRendererHpp() override { destroy(); }

    bool init(const vmcr::InitParams& params) noexcept override {
        params_ = params;

#if VMCR_HAS_VKBOOTSTRAP
        // 1) 使用 vkbootstrap 创建 Instance
        vkb::InstanceBuilder ib;
        auto inst_ret = ib
            .set_app_name("VMCR")
            .set_engine_name("VMCR")
            .request_validation_layers(false)   // 性能优先
            .require_api_version(1, 3, 0)
            .build();
        if (!inst_ret) {
            LOG_E(kTag, "vkb::InstanceBuilder failed: %s",
                 inst_ret.error().message().c_str());
            return false;
        }
        instance_ = inst_ret.value();
        surface_ = inst_ret->get_surface();   // 后续若需 surface
        phys_device_ = instance_->get_physical_device();
#else
        // Fallback: 直接用 Vulkan-Hpp 创建
        vk::raii::Context ctx;
        vk::ApplicationInfo app{
            "VMCR", VK_MAKE_API_VERSION(0, 1, 0, 0),
            "VMCR",  VK_MAKE_API_VERSION(0, 1, 0, 0),
            vk::ApiVersion13
        };
        instance_ = vk::raii::Instance(ctx, vk::InstanceCreateInfo{
            {}, &app
        });
        auto pds = instance_->enumeratePhysicalDevices();
        if (pds.empty()) {
            LOG_E(kTag, "no Vulkan device");
            return false;
        }
        phys_device_ = pds[0];
#endif

        auto pd_props = phys_device_.getProperties();
        LOG_I(kTag, "[VK-HPP] device: %s api=0x%x",
              pd_props.deviceName.data(), pd_props.apiVersion);

        // 2) 找 Graphics Queue
        auto qfps = phys_device_.getQueueFamilyProperties();
        for (uint32_t i = 0; i < qfps.size(); ++i) {
            if (qfps[i].queueFlags & vk::QueueFlagBits::eGraphics) {
                gfx_family_ = i;
                break;
            }
        }
        if (gfx_family_ == UINT32_MAX) {
            LOG_E(kTag, "no graphics queue");
            return false;
        }

#if VMCR_HAS_VKBOOTSTRAP
        // 3) 使用 vkbootstrap 创建设备
        float prio = 1.0f;
        vkb::DeviceBuilder db{phys_device_};
        auto dev_ret = db
            .custom_queue_setup([&](vkb::CustomQueueDescription& q) {
                q.add_queue_info({ gfx_family_, 1, 1.0f, vk::QueueFlagBits::eGraphics });
            })
            .build();
        if (!dev_ret) {
            LOG_E(kTag, "vkb::DeviceBuilder failed: %s",
                 dev_ret.error().message().c_str());
            return false;
        }
        device_ = dev_ret.value();
        gfx_queue_ = device_->get_queue(vkb::QueueType::graphics, 0);
#else
        // Fallback: 手创设备
        float prio = 1.0f;
        vk::DeviceQueueCreateInfo qci{{}, gfx_family_, 1, &prio};
        vk::PhysicalDeviceVulkan13Features f13{};
        f13.dynamicRendering = true;
        f13.synchronization2 = true;
        vk::PhysicalDeviceVulkan12Features f12{};
        f12.timelineSemaphore = true;
        f12.bufferDeviceAddress = true;
        f12.pNext = &f13;
        const char* exts[] = { "VK_KHR_swapchain" };
        device_ = vk::raii::Device(phys_device_, vk::DeviceCreateInfo{
            {}, 1, &qci, 0, nullptr,
            static_cast<uint32_t>(std::size(exts)), exts,
            nullptr
        });
        gfx_queue_ = device_->getQueue(gfx_family_, 0);
#endif

        LOG_I(kTag, "[VK-HPP] init OK, gfx_queue family=%u", gfx_family_);

#if VMCR_HAS_VMA
        // 4) 创建 VMA 分配器
        VmaAllocatorCreateInfo vma_ci{};
        vma_ci.physicalDevice = phys_device_;
        vma_ci.device = device_;
        vma_ci.instance = instance_;
        vma_ci.vulkanApiVersion = VK_API_VERSION_1_3;
        vma_result r = vmaCreateAllocator(&vma_ci, &vma_);
        if (r != VK_SUCCESS) {
            LOG_W(kTag, "VMA create failed (%d), continue without", r);
        } else {
            LOG_I(kTag, "[VK-HPP] VMA ready");
        }
#endif

        return true;
    }

    void destroy() noexcept override {
        if (device_) {
            device_.waitIdle();
        }
#if VMCR_HAS_VMA
        if (vma_) {
            vmaDestroyAllocator(vma_);
            vma_ = VK_NULL_HANDLE;
        }
#endif
        device_ = nullptr;
        instance_ = nullptr;
        LOG_I(kTag, "[VK-HPP] destroyed");
    }

    void begin_frame() noexcept override { /* Phase 4 */ }
    void submit(const vmcr::DrawCmd& cmd) noexcept override { (void)cmd; }
    void end_frame() noexcept override { /* Phase 4 */ }
    void on_surface_changed(uint32_t w, uint32_t h) noexcept override {
        (void)w; (void)h;
    }

    const char* name() const noexcept override { return "vulkan-hpp"; }
    vmcr::RendererTier tier() const noexcept override { return vmcr::RendererTier::VulkanFull; }

private:
    vmcr::InitParams params_{};

    vk::raii::Context                  context_;
#if VMCR_HAS_VKBOOTSTRAP
    std::unique_ptr<vkb::Instance>     instance_;
    std::unique_ptr<vkb::Device>       device_;
#else
    vk::raii::Instance                  instance_{context_};
    vk::raii::Device                    device_{nullptr};
#endif
    vk::PhysicalDevice                  phys_device_;
    vk::raii::SurfaceKHR               surface_{nullptr};
    vk::Queue                           gfx_queue_ = nullptr;
    uint32_t                            gfx_family_ = UINT32_MAX;

#if VMCR_HAS_VMA
    VmaAllocator                        vma_ = VK_NULL_HANDLE;
#endif
};

}  // namespace vmcr::vk

#endif  // VMCR_HAS_VULKAN_HPP
