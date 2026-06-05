// ===========================================================================
// src/main/cpp/vulkan/vk_device.cpp - 物理 / 逻辑设备 (Phase 2 详细实现)
// 当前为 stub, 实际 init 在 Phase 2
// ===========================================================================
#include "vmcr/backend.h"
#include "vmcr/log.h"

#include <vulkan/vulkan.h>
#include <vector>

namespace vmcr::vk {

constexpr const char* kTag = log::kTagVK;

class VkDevice {
public:
    bool init(VkInstance inst, VkPhysicalDevice pdev) noexcept {
        instance_ = inst;
        physical_ = pdev;

        // 查询 queue family
        uint32_t qfc = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(pdev, &qfc, nullptr);
        std::vector<VkQueueFamilyProperties> qfp(qfc);
        vkGetPhysicalDeviceQueueFamilyProperties(pdev, &qfc, qfp.data());

        graphics_family_ = UINT32_MAX;
        present_family_  = UINT32_MAX;
        for (uint32_t i = 0; i < qfc; ++i) {
            if (qfp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphics_family_ = i;
                present_family_  = i;  // 简化假设: graphics == present
                break;
            }
        }
        if (graphics_family_ == UINT32_MAX) {
            LOG_E(kTag, "no graphics queue family");
            return false;
        }

        float prio = 1.0f;
        VkDeviceQueueCreateInfo qci{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = graphics_family_,
            .queueCount = 1,
            .pQueuePriorities = &prio,
        };

        // 启用 1.3 特性
        VkPhysicalDeviceVulkan13Features f13{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .dynamicRendering = VK_TRUE,
            .timelineSemaphore = VK_TRUE,
            .synchronization2 = VK_TRUE,
        };

        const char* dev_exts[] = {
            "VK_KHR_swapchain",
            "VK_KHR_push_descriptor",
            "VK_KHR_draw_indirect_count",
        };

        VkDeviceCreateInfo dci{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &f13,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &qci,
            .enabledExtensionCount = sizeof(dev_exts)/sizeof(dev_exts[0]),
            .ppEnabledExtensionNames = dev_exts,
        };

        VkResult r = vkCreateDevice(pdev, &dci, nullptr, &device_);
        if (r != VK_SUCCESS) {
            LOG_E(kTag, "vkCreateDevice failed: %d", r);
            return false;
        }

        vkGetDeviceQueue(device_, graphics_family_, 0, &graphics_queue_);
        LOG_I(kTag, "[DEVICE] created, queueFamily=%u", graphics_family_);
        return true;
    }

    void destroy() noexcept {
        if (device_ != VK_NULL_HANDLE) {
            vkDestroyDevice(device_, nullptr);
            device_ = VK_NULL_HANDLE;
        }
    }

    VkDevice       handle()   const noexcept { return device_; }
    VkPhysicalDevice physical() const noexcept { return physical_; }
    VkQueue        graphics()  const noexcept { return graphics_queue_; }
    uint32_t       gfx_family() const noexcept { return graphics_family_; }

private:
    VkInstance          instance_       = VK_NULL_HANDLE;
    VkPhysicalDevice    physical_       = VK_NULL_HANDLE;
    VkDevice            device_         = VK_NULL_HANDLE;
    VkQueue             graphics_queue_ = VK_NULL_HANDLE;
    uint32_t            graphics_family_ = UINT32_MAX;
    uint32_t            present_family_   = UINT32_MAX;
};

}  // namespace vmcr::vk
