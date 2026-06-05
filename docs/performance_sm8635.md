# ===========================================================================
# SM8635 设备性能调优笔记
# ===========================================================================

## 概述

SM8635 (Snapdragon 8s Gen 3) 搭载 **Adreno 735** GPU, 属于 Adreno 7xx 系列,
Vulkan 1.3 完整支持, tile-based deferred rendering (TBDR) 架构.

## 关键能力

| 项 | 值 |
| :--- | :--- |
| API | 1.3.250+ |
| 驱动 | 512.450.0+ |
| shader cores | ~1024 |
| 内存 | 共系统内存 (LPDDR5X) |
| tile size | 256x256 (estimated) |
| FP16 throughput | 2x FP32 |
| INT8 dot | 支持 |

## 推荐优化 (见 ARCHITECTURE.md § 11)

1. **Lazy depth** - `VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT` + `VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED`
2. **Persistent pipeline cache** - 加载 `vkGetPipelineCacheData` 保存的 cache, 冷启动 -40%
3. **AHB 直通** - `VK_ANDROID_external_memory_android_hardware_buffer`
4. **FP16 vertex** - 着色器使用 `mediump`
5. **NEON mesh build** - chunk 网格 CPU 构建, `-mcpu=cortex-a75`
6. **descriptor_buffer** - `VK_EXT_descriptor_buffer` 降低 CPU 开销 50%
7. **draw_indirect_count** - `VK_KHR_draw_indirect_count` 减少 CPU multi-draw 循环

## 已知问题

* 部分驱动版本 `vkGetPhysicalDeviceSurfaceSupportKHR` 返回不一致, 需要 fallback
* `VK_KHR_dynamic_rendering_local_read` 在 512.450 之前驱动可能 panic
* 持续高负载下 thermal throttling, 帧率从 60 跌到 45
