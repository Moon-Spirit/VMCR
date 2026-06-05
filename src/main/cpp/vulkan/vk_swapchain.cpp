// ===========================================================================
// src/main/cpp/vulkan/vk_swapchain.cpp - Swapchain 管理 (Phase 2 详细实现)
// 当前为 stub
// ===========================================================================
#include "vmcr/backend.h"
#include "vmcr/log.h"

#include <vulkan/vulkan.h>

namespace vmcr::vk {

constexpr const char* kTag = log::kTagVK;

class VkSwapchain {
public:
    bool init(VkDevice dev, VkPhysicalDevice pdev, ANativeWindow* window,
              uint32_t w, uint32_t h) noexcept {
        (void)dev; (void)pdev; (void)window; (void)w; (void)h;
        LOG_I(kTag, "[SWAPCHAIN] init %ux%u (stub - Phase 2 will implement)", w, h);
        return true;
    }

    void destroy() noexcept {
        // TODO: vkDestroySwapchainKHR
    }
};

}  // namespace vmcr::vk
