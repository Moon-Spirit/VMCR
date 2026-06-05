# ===========================================================================
# 降级矩阵
# ===========================================================================

## 设备 → 档位

| 设备 | GPU | Vulkan | 档位 | 备注 |
| :--- | :--- | :--- | :--- | :--- |
| SM8635 | Adreno 735 | 1.3 | VulkanFull | 满分 |
| SM8650 | Adreno 750 | 1.3 | VulkanFull | 满分 + mesh shader |
| SM8550 | Adreno 740 | 1.3 | VulkanFull | 满分 |
| SM8475 | Adreno 730 | 1.3 | VulkanFull | 满分 |
| SD 8 Gen 1 | Adreno 730 | 1.3 | VulkanFull | 满分 |
| SD 845 | Adreno 630 | 1.1 | VulkanLimited | 无 dynamic rendering |
| SD 660 | Adreno 512 | 1.0 | GLES32 | 无 1.1 内核 |
| Kirin 990 | Mali-G76 | 1.1 | VulkanLimited | 缺部分 1.3 特性 |
| Kirin 9000 | Mali-G78 | 1.3 | VulkanFull | 接近 Adreno |
| Exynos 2200 | Xclipse 920 | 1.3 | VulkanFull | 接近 Adreno |
| Exynos 990 | Mali-G77 | 1.1 | VulkanLimited | 弱 |
| Helio G99 | Mali-G57 | 1.1 | VulkanLimited | storage 16 MiB |
| 无 Vulkan | — | — | GLES32 | 直接降级 |

## 缺失特性 → 渲染模式

参见 ARCHITECTURE.md § 10 表格.
