// ===========================================================================
// src/main/cpp/vulkan/vk_renderer.cpp
// VulkanRenderer 完整实现 (Phase 2: 真实 instance/device/swapchain/clear)
//
// 流程:
//   init()         -> instance + physical + device + queue
//   on_surface_changed() -> swapchain + image views + framebuffers
//   begin_frame()  -> acquire image
//   end_frame()    -> record clear + submit + present
// ===========================================================================
#include "vmcr/vk_renderer.h"
#include "vmcr/log.h"

#include <vulkan/vulkan.h>

#ifdef __ANDROID__
#include <android/native_window.h>
#endif

#include <algorithm>
#include <cstring>
#include <vector>

namespace vmcr::vk {

constexpr const char* kTag = log::kTagVK;

VulkanRenderer::VulkanRenderer() = default;
VulkanRenderer::~VulkanRenderer() { destroy(); }

bool VulkanRenderer::init(const vmcr::InitParams& params) noexcept {
    params_ = params;
    LOG_I(kTag, "[VK] init (Phase 2)");

    if (!create_instance())         return false;
    if (!pick_physical_device())    return false;
    if (!create_logical_device())   return false;
    if (!create_command_pool_and_buffer()) return false;

    LOG_I(kTag, "[VK] init done");
    return true;
}

void VulkanRenderer::destroy() noexcept {
    LOG_I(kTag, "[VK] destroy");

    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);

        destroy_swapchain();

        for (auto fb : framebuffers_) {
            if (fb != VK_NULL_HANDLE) vkDestroyFramebuffer(device_, fb, nullptr);
        }
        if (render_pass_ != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device_, render_pass_, nullptr);
            render_pass_ = VK_NULL_HANDLE;
        }
        for (auto cb : cmd_buffers_) {
            if (cb != VK_NULL_HANDLE) {}  // 由 pool 销毁
        }
        if (cmd_pool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, cmd_pool_, nullptr);
            cmd_pool_ = VK_NULL_HANDLE;
        }
        if (timeline_sem_ != VK_NULL_HANDLE) {
            vkDestroySemaphore(device_, timeline_sem_, nullptr);
            timeline_sem_ = VK_NULL_HANDLE;
        }
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

bool VulkanRenderer::create_instance() noexcept {
    VkApplicationInfo app{
        .sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "VMCR",
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName      = "VMCR",
        .engineVersion    = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion       = VK_API_VERSION_1_3,
    };
    VkInstanceCreateInfo ici{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
    };
    if (vkCreateInstance(&ici, nullptr, &instance_) != VK_SUCCESS) {
        // 退到 1.1
        app.apiVersion = VK_API_VERSION_1_1;
        if (vkCreateInstance(&ici, nullptr, &instance_) != VK_SUCCESS) {
            LOG_E(kTag, "vkCreateInstance failed");
            return false;
        }
    }
    return true;
}

bool VulkanRenderer::pick_physical_device() noexcept {
    uint32_t dc = 0;
    vkEnumeratePhysicalDevices(instance_, &dc, nullptr);
    if (dc == 0) {
        LOG_E(kTag, "no Vulkan device");
        return false;
    }
    std::vector<VkPhysicalDevice> devs(dc);
    vkEnumeratePhysicalDevices(instance_, &dc, devs.data());

    // 简单选择: 第一个设备
    physical_ = devs[0];

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physical_, &props);
    LOG_I(kTag, "[VK] physical: %s api=0x%x driver=0x%x",
          props.deviceName, props.apiVersion, props.driverVersion);

    return true;
}

bool VulkanRenderer::create_logical_device() noexcept {
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
        return false;
    }
    present_family_ = gfx_family_;  // 简化

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = gfx_family_,
        .queueCount = 1,
        .pQueuePriorities = &prio,
    };

    // 注意: 1.3 features 字段在不同 SDK 版本有差异
    // 较新的 SDK 把 timelineSemaphore 移到了 1.2 features
    // 字段顺序必须与 struct 定义一致 (C99/C++ designated init)
    VkPhysicalDeviceVulkan13Features f13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = VK_TRUE,        // synchronization2 在前
        .dynamicRendering = VK_TRUE,        // dynamicRendering 在后 (按 struct 顺序)
    };
    VkPhysicalDeviceVulkan12Features f12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .timelineSemaphore = VK_TRUE,
        .bufferDeviceAddress = VK_TRUE,
    };
    f12.pNext = &f13;

    const char* exts[] = {
        "VK_KHR_swapchain",
    };

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
        return false;
    }
    vkGetDeviceQueue(device_, gfx_family_, 0, &gfx_queue_);

    // Timeline semaphore
    VkSemaphoreTypeCreateInfo stci{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = 0,
    };
    VkSemaphoreCreateInfo sci{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &stci,
    };
    if (vkCreateSemaphore(device_, &sci, nullptr, &timeline_sem_) != VK_SUCCESS) {
        LOG_W(kTag, "timeline semaphore create failed, will use binary");
    }

    return true;
}

bool VulkanRenderer::create_command_pool_and_buffer() noexcept {
    VkCommandPoolCreateInfo cpci{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = gfx_family_,
    };
    if (vkCreateCommandPool(device_, &cpci, nullptr, &cmd_pool_) != VK_SUCCESS) {
        LOG_E(kTag, "vkCreateCommandPool failed");
        return false;
    }

    VkCommandBufferAllocateInfo cbai{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmd_pool_,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 2,
    };
    if (vkAllocateCommandBuffers(device_, &cbai, cmd_buffers_) != VK_SUCCESS) {
        LOG_E(kTag, "vkAllocateCommandBuffers failed");
        return false;
    }
    return true;
}

bool VulkanRenderer::create_swapchain(ANativeWindow* window, uint32_t w, uint32_t h) noexcept {
    if (device_ == VK_NULL_HANDLE || window == nullptr) return false;

    destroy_swapchain();

#ifdef VK_USE_PLATFORM_ANDROID_KHR
    // 创建 surface (仅 Android)
    VkAndroidSurfaceCreateInfoKHR asci{
        .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
        .window = window,
    };
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (vkCreateAndroidSurfaceKHR(instance_, &asci, nullptr, &surface) != VK_SUCCESS) {
        LOG_E(kTag, "vkCreateAndroidSurfaceKHR failed");
        return false;
    }
#else
    VkSurfaceKHR surface = VK_NULL_HANDLE;
#endif

    width_  = w;
    height_ = h;

#ifdef VK_USE_PLATFORM_ANDROID_KHR
    // surface 已在前面 (Android 平台) 或保持 VK_NULL_HANDLE (主机)
    // 查询 surface 能力
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_, surface, &caps);

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX) {
        extent.width  = std::clamp(w, caps.minImageExtent.width,  caps.maxImageExtent.width);
        extent.height = std::clamp(h, caps.minImageExtent.height, caps.maxImageExtent.height);
    }
    swapchain_extent_ = extent;

    uint32_t image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount) {
        image_count = caps.maxImageCount;
    }
    image_count = std::min<uint32_t>(image_count, 3);

    VkSwapchainCreateInfoKHR sci{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = image_count,
        .imageFormat = swapchain_format_,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = caps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
    };
    if (vkCreateSwapchainKHR(device_, &sci, nullptr, &swapchain_) != VK_SUCCESS) {
        LOG_E(kTag, "vkCreateSwapchainKHR failed");
        vkDestroySurfaceKHR(instance_, surface, nullptr);
        return false;
    }

    // 获取 image
    uint32_t ic = 0;
    vkGetSwapchainImagesKHR(device_, swapchain_, &ic, nullptr);
    swapchain_images_.resize(ic);
    vkGetSwapchainImagesKHR(device_, swapchain_, &ic, swapchain_images_.data());
    swapchain_views_.resize(ic);

    for (uint32_t i = 0; i < ic; ++i) {
        VkImageViewCreateInfo ivci{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchain_images_[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchain_format_,
            .components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                            VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
        };
        if (vkCreateImageView(device_, &ivci, nullptr, &swapchain_views_[i]) != VK_SUCCESS) {
            LOG_E(kTag, "vkCreateImageView[%u] failed", i);
            return false;
        }
    }

    // 简单 render pass (单 subpass, color attachment clear)
    VkAttachmentDescription ad{
        .format = swapchain_format_,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    VkAttachmentReference ar{ .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription sd{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &ar,
    };
    VkSubpassDependency dep{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };
    VkRenderPassCreateInfo rpci{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &ad,
        .subpassCount = 1,
        .pSubpasses = &sd,
        .dependencyCount = 1,
        .pDependencies = &dep,
    };
    if (vkCreateRenderPass(device_, &rpci, nullptr, &render_pass_) != VK_SUCCESS) {
        LOG_E(kTag, "vkCreateRenderPass failed");
        return false;
    }

    // Framebuffers
    for (size_t i = 0; i < swapchain_views_.size() && i < 3; ++i) {
        VkFramebufferCreateInfo fbci{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = render_pass_,
            .attachmentCount = 1,
            .pAttachments = &swapchain_views_[i],
            .width = extent.width,
            .height = extent.height,
            .layers = 1,
        };
        if (vkCreateFramebuffer(device_, &fbci, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
            LOG_E(kTag, "vkCreateFramebuffer[%zu] failed", i);
            return false;
        }
    }

    vkDestroySurfaceKHR(instance_, surface, nullptr);  // surface 仅用于创建 swapchain

    swapchain_ready_ = true;
    LOG_I(kTag, "[SWAPCHAIN] %u images %ux%u", ic, extent.width, extent.height);
    return true;
#else
    // 主机构建: 没有真 surface, swapchain 不可用
    swapchain_ready_ = false;
    LOG_W(kTag, "[VK] create_swapchain noop on host (no surface)");
    return false;
#endif
}

void VulkanRenderer::destroy_swapchain() noexcept {
    if (device_ == VK_NULL_HANDLE) return;
    for (auto& v : swapchain_views_) {
        if (v != VK_NULL_HANDLE) { vkDestroyImageView(device_, v, nullptr); v = VK_NULL_HANDLE; }
    }
    swapchain_views_.clear();
    swapchain_images_.clear();
    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
    for (auto& fb : framebuffers_) {
        if (fb != VK_NULL_HANDLE) { vkDestroyFramebuffer(device_, fb, nullptr); fb = VK_NULL_HANDLE; }
    }
    swapchain_ready_ = false;
}

void VulkanRenderer::on_surface_changed(uint32_t w, uint32_t h) noexcept {
    if (w == 0 || h == 0) return;
    if (w == width_ && h == height_ && swapchain_ready_) return;

    LOG_I(kTag, "[VK] on_surface_changed %ux%u", w, h);

    if (params_.window) {
        create_swapchain(params_.window, w, h);
    } else {
        LOG_W(kTag, "[VK] no ANativeWindow in params, swapchain deferred");
    }
}

void VulkanRenderer::begin_frame() noexcept {
    if (!swapchain_ready_) return;

    // 简化: Phase 5 加 timeout / out-of-date 处理
    VkResult r = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                                        VK_NULL_HANDLE, VK_NULL_HANDLE, &image_index_);
    if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
        LOG_W(kTag, "vkAcquireNextImageKHR = %d", r);
        return;
    }
}

void VulkanRenderer::submit(const vmcr::DrawCmd& cmd) noexcept {
    // Phase 3+ 实现: 录制到当前帧 command buffer
    (void)cmd;
}

void VulkanRenderer::record_clear_cmd(VkCommandBuffer cmd, uint32_t image_index) noexcept {
    VkCommandBufferBeginInfo cbbi{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(cmd, &cbbi);

    VkClearValue cv{};
    cv.color = { { 0.1f, 0.2f, 0.4f, 1.0f } };  // 蓝色 - VMCR 标识色

    VkRenderPassBeginInfo rpbi{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass_,
        .framebuffer = framebuffers_[image_index],
        .renderArea = { .offset = {0, 0}, .extent = swapchain_extent_ },
        .clearValueCount = 1,
        .pClearValues = &cv,
    };
    vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    // Phase 3+: 在这里调用 vkCmdBindPipeline + vkCmdDraw*
    vkCmdEndRenderPass(cmd);

    vkEndCommandBuffer(cmd);
}

void VulkanRenderer::end_frame() noexcept {
    if (!swapchain_ready_) return;
    if (image_index_ == UINT32_MAX) return;

    uint32_t cb_idx = frame_index_ % 2;
    VkCommandBuffer cb = cmd_buffers_[cb_idx];

    vkResetCommandBuffer(cb, 0);
    record_clear_cmd(cb, image_index_);

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &cb,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };
    if (vkQueueSubmit(gfx_queue_, 1, &si, VK_NULL_HANDLE) != VK_SUCCESS) {
        LOG_E(kTag, "vkQueueSubmit failed");
        return;
    }

    VkPresentInfoKHR pi{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 0,
        .swapchainCount = 1,
        .pSwapchains = &swapchain_,
        .pImageIndices = &image_index_,
    };
    VkResult r = vkQueuePresentKHR(gfx_queue_, &pi);
    if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
        LOG_W(kTag, "vkQueuePresentKHR = %d", r);
    }

    image_index_ = UINT32_MAX;
    frame_index_++;
    frame_value_++;
}

}  // namespace vmcr::vk

// 工厂入口 - 由 dlopen 加载
extern "C" {
#ifdef _WIN32
__declspec(dllexport)
#endif
void* vmcr_renderer_create() {
    return new vmcr::vk::VulkanRenderer();
}
}  // extern "C"
