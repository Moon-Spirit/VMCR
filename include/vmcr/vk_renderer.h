// ===========================================================================
// include/vmcr/vk_renderer.h
// VulkanRenderer 的公开接口 (供 BackendRouter / Loader 使用)
// ===========================================================================
#pragma once

#include "vmcr/backend.h"

#include <vulkan/vulkan.h>

#ifdef __ANDROID__
#include <android/native_window.h>
#else
// 主机侧 fallback: 用 void* 占位
struct ANativeWindow;
#endif

#include <vector>

namespace vmcr::vk {

class VulkanRenderer : public vmcr::IRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer() override;

    bool init(const vmcr::InitParams& params) noexcept override;
    void destroy() noexcept override;

    void begin_frame() noexcept override;
    void submit(const vmcr::DrawCmd& cmd) noexcept override;
    void end_frame() noexcept override;

    void on_surface_changed(uint32_t w, uint32_t h) noexcept override;

    const char*  name() const noexcept override { return "vulkan"; }
    vmcr::RendererTier tier() const noexcept override { return vmcr::RendererTier::VulkanFull; }

    // ---- 内部状态 (供测试访问) ----
    VkInstance       instance()      const noexcept { return instance_; }
    VkPhysicalDevice physical()      const noexcept { return physical_; }
    VkDevice         device()        const noexcept { return device_; }
    VkQueue          gfx_queue()     const noexcept { return gfx_queue_; }
    uint32_t         gfx_family()    const noexcept { return gfx_family_; }
    VkSwapchainKHR   swapchain()     const noexcept { return swapchain_; }

private:
    bool create_instance() noexcept;
    bool pick_physical_device() noexcept;
    bool create_logical_device() noexcept;
    bool create_swapchain(ANativeWindow* window, uint32_t w, uint32_t h) noexcept;
    bool create_command_pool_and_buffer() noexcept;
    void record_clear_cmd(VkCommandBuffer cmd, uint32_t image_index) noexcept;
    void destroy_swapchain() noexcept;

    vmcr::InitParams params_{};

    VkInstance       instance_  = VK_NULL_HANDLE;
    VkPhysicalDevice physical_  = VK_NULL_HANDLE;
    VkDevice         device_    = VK_NULL_HANDLE;
    VkQueue          gfx_queue_ = VK_NULL_HANDLE;
    uint32_t         gfx_family_ = UINT32_MAX;
    uint32_t         present_family_ = UINT32_MAX;

    // Swapchain
    VkSwapchainKHR           swapchain_       = VK_NULL_HANDLE;
    VkFormat                 swapchain_format_ = VK_FORMAT_R8G8B8A8_UNORM;
    VkExtent2D               swapchain_extent_ = {0, 0};
    std::vector<VkImage>     swapchain_images_;
    std::vector<VkImageView> swapchain_views_;

    // Render pass (Phase 2 暂用传统 render pass, Phase 5 切 dynamic rendering)
    VkRenderPass   render_pass_ = VK_NULL_HANDLE;
    VkFramebuffer  framebuffers_[3] = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};

    // Command
    VkCommandPool   cmd_pool_    = VK_NULL_HANDLE;
    VkCommandBuffer cmd_buffers_[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};

    // Sync (使用 timeline semaphore)
    VkSemaphore     timeline_sem_   = VK_NULL_HANDLE;
    uint64_t        frame_value_    = 0;

    // State
    bool         surface_ready_    = false;
    bool         swapchain_ready_  = false;
    uint32_t     image_index_      = UINT32_MAX;
    uint32_t     frame_index_      = 0;
    uint32_t     width_            = 0;
    uint32_t     height_           = 0;
};

}  // namespace vmcr::vk
