// ===========================================================================
// include/vmcr/vk_probe.h - 特性探测结果
// ===========================================================================
#pragma once

#include <cstdint>
#include "vmcr/backend.h"

namespace vmcr::vk {

// 探测选项
struct ProbeOptions {
    bool   enable_validation = false;     // 仅调试期
    bool   prefer_discrete   = false;     // 偏好独显
    bool   verbose           = false;
};

// 探测结果
struct ProbeResult {
    RendererTier tier          = RendererTier::Invalid;
    uint32_t     score         = 0;        // 评分
    uint32_t     api_version   = 0;        // VK_API_VERSION
    uint32_t     driver_version = 0;
    char         vendor_name[64]   = {0};
    char         device_name[128]  = {0};

    // 关键特性 (按 1.3 内核 / 扩展分别记录)
    bool has_dynamic_rendering       = false;
    bool has_timeline_semaphore      = false;
    bool has_synchronization2        = false;
    bool has_maintenance4            = false;
    bool has_buffer_device_address   = false;
    bool has_push_descriptor         = false;
    bool has_draw_indirect_count     = false;
    bool has_descriptor_buffer       = false;
    bool has_ahb_external_memory     = false;
    bool has_external_memory_fd      = false;
    bool has_swapchain               = false;
    bool has_create_renderpass2      = false;

    // 能力
    uint64_t max_storage_buffer_range = 0;
    uint64_t max_memory_allocation    = 0;
};

// 探测入口
// 1) 加载 libvulkan.so
// 2) 尝试创建 1.3 实例, 失败回退 1.1
// 3) 枚举物理设备并评分
// 4) 选出最高分, 返回 tier
ProbeResult probe_tier(const ProbeOptions& opt = {}) noexcept;

}  // namespace vmcr::vk
