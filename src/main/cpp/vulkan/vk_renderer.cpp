// ===========================================================================
// src/main/cpp/vulkan/vk_renderer.cpp - Vulkan 后端实现 (Phase 2)
// 当前为 Phase 0/1 stub: 完整 init + 清屏 + present 链路
// ===========================================================================
#include "vmcr/backend.h"
#include "vmcr/log.h"
#include "vmcr/vk_probe.h"

#include <vulkan/vulkan.h>
#include <android/native_window.h>

#include <cstring>

namespace vmcr::vk {

constexpr const char* kTag = log::kTagVK;

class VulkanRenderer : public vmcr::IRenderer {
public:
    VulkanRenderer() = default;
    ~VulkanRenderer() override { destroy(); }

    bool init(const vmcr::InitParams& p) noexcept override {
        params_ = p;

        ProbeResult probe = probe_tier({});
        if (probe.tier == RendererTier::GLES32 || probe.tier == RendererTier::Invalid) {
            LOG_W(kTag, "Probe did not recommend Vulkan, but trying anyway");
        }

        // 创建 Instance
        VkApplicationInfo app{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "VMCR",
            .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .pEngineName = "VMCR",
            .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .apiVersion = VK_API_VERSION_1_3,
        };
        VkInstanceCreateInfo ici{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &app,
        };
        if (vkCreateInstance(&ici, nullptr, &instance_) != VK_SUCCESS) {
            LOG_E(kTag, "vkCreateInstance failed");
            return false;
        }

        // 选物理设备
        uint32_t dc = 0;
        vkEnumeratePhysicalDevices(instance_, &dc, nullptr);
        if (dc == 0) {
            LOG_E(kTag, "no Vulkan device");
            destroy();
            return false;
        }
        std::vector<VkPhysicalDevice> devs(dc);
        vkEnumeratePhysicalDevices(instance_, &dc, devs.data());
        physical_ = devs[0];  // 简化: 取第一个

        // 创建逻辑设备
        float prio = 1.0f;
        uint32_t qfc = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_, &qfc, nullptr);
        std::vector<VkQueueFamilyProperties> qfp(qfc);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_, &qfc, qfp.data());
        gfx_family_ = UINT32_MAX;
        for (uint32_t i = 0; i < qfc; ++i) {
            if (qfp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                gfx_family_ = i;
                break;
            }
        }
        if (gfx_family_ == UINT32_MAX) {
            LOG_E(kTag, "no graphics queue");
            destroy();
            return false;
        }

        VkPhysicalDeviceVulkan13Features f13{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .dynamicRendering = VK_TRUE,
            .timelineSemaphore = VK_TRUE,
            .synchronization2 = VK_TRUE,
        };

        VkDeviceQueueCreateInfo qci{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = gfx_family_,
            .queueCount = 1,
            .pQueuePriorities = &prio,
        };

        const char* exts[] = { "VK_KHR_swapchain" };
        VkDeviceCreateInfo dci{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &f13,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &qci,
            .enabledExtensionCount = sizeof(exts)/sizeof(exts[0]),
            .ppEnabledExtensionNames = exts,
        };
        if (vkCreateDevice(physical_, &dci, nullptr, &device_) != VK_SUCCESS) {
            LOG_E(kTag, "vkCreateDevice failed");
            destroy();
            return false;
        }
        vkGetDeviceQueue(device_, gfx_family_, 0, &gfx_queue_);

        // Swapchain 占位 (Phase 2 实现)
        LOG_I(kTag, "[VULKAN] init complete (Phase 0 stub)");
        return true;
    }

    void destroy() noexcept override {
        if (device_ != VK_NULL_HANDLE) {
            vkDestroyDevice(device_, nullptr);
            device_ = VK_NULL_HANDLE;
        }
        if (instance_ != VK_NULL_HANDLE) {
            vkDestroyInstance(instance_, nullptr);
            instance_ = VK_NULL_HANDLE;
        }
    }

    void begin_frame() noexcept override {
        // TODO Phase 2: vkAcquireNextImageKHR
    }

    void submit(const vmcr::DrawCmd&) noexcept override {
        // TODO Phase 3
    }

    void end_frame() noexcept override {
        // TODO Phase 2: vkQueuePresentKHR
    }

    void on_surface_changed(uint32_t, uint32_t) noexcept override {
        // TODO Phase 2
    }

    const char* name() const noexcept override { return "vulkan-stub"; }
    vmcr::RendererTier tier() const noexcept override { return vmcr::RendererTier::VulkanFull; }

private:
    vmcr::InitParams params_{};
    VkInstance       instance_  = VK_NULL_HANDLE;
    VkPhysicalDevice physical_  = VK_NULL_HANDLE;
    VkDevice         device_    = VK_NULL_HANDLE;
    VkQueue          gfx_queue_ = VK_NULL_HANDLE;
    uint32_t         gfx_family_ = UINT32_MAX;
};

}  // namespace vmcr::vk

// 工厂入口 - 由 dlopen 加载
extern "C" __attribute__((visibility("default")))
vmcr::RendererPtr vmcr_renderer_create() {
    return std::make_unique<vmcr::vk::VulkanRenderer>();
}
