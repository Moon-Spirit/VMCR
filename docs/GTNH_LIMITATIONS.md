# VMCR 已知限制与 GTNH (1.7.10) 兼容性说明

## 1.7.10 (GT: New Horizons) 支持

### ✅ 工作的部分
- **libGL.so / libEGL.so 劫持**: FCL 通过 `custom_renderer.json` / `AndroidManifest meta-data` 加载我们的 .so
- **GLES 3.2 转发路径**: 1.7.10 的 OpenGL 调用通过 LWJGL3 (Pojav/FCL 实现) 路由到 `libGL.so`, 我们的 forwarder 转发到原厂驱动
- **Vulkan 主路径 (Phase 2+)**: 探测逻辑, instance/device/queue 创建. 真机上需要 Vulkan 1.x 驱动
- **Adreno 735 优化**: SM8635 设备上走快速路径

### ⚠️ 限制 (1.7.10 特定)
- **Fabric Mod 不工作**: 1.7.10 是 Forge 时代. JNI Mixin chunk 拦截仅在 1.16.5+ (Fabric Loader 时代) 生效
- **chunk mesh 截取**: 通过 Fabric Mixin 实现, 1.7.10 没有 Fabric. chunk mesh 走标准 GLES forwarder 路径, 没有 SSBO 上传加速
- **光影 (Iris 兼容)**: Phase 4 才开始. 1.7.10 的 Iris 包可能需要特殊处理
- **LWJGL2 兼容**: 1.7.10 原版用 LWJGL2. FCL 的 LWJGL3 实现才能与本插件一起工作

### 建议的 1.7.10 配置
1. 在 FCL 启动器中**先选 VMCR 启动器运行一次** (这样 FCL 会解压/准备 LWJGL3)
2. 然后正常进入 GTNH
3. 若崩溃, 切换到默认 GLES 渲染器

## Phase 状态

| Phase | 状态 | MC 1.7.10 适用性 | MC 1.16.5+ 适用性 |
| :--- | :---: | :---: | :---: |
| 0 - 探测 | ✅ | ✅ | ✅ |
| 1 - GLES 转发 | ✅ | ✅ (主路径) | ✅ |
| 2 - Vulkan 设备/交换链 | ✅ (代码) | ⚠️ 需 Vulkan 1.x 驱动 | ✅ |
| 3 - JNI chunk 截取 | 🚧 | ❌ 无 Fabric | ✅ |
| 4 - 光影 (Iris) | 🚧 | ⚠️ 需 Iris 1.7.10 兼容包 | ✅ |
| 5 - 性能调优 | 🚧 | ✅ | ✅ |

## 真机验证

需要在以下设备上验证:
- Adreno 735 (SM8635): Phase 2 优化目标
- Mali-G76/G77 (中端): 测试降级路径
- Adreno 630/640 (SD845/SD855): 旧 Vulkan 1.1 设备, 降级 GLES

VMCR 主机侧构建 (CMake + VS2022 / gcc 11) 已验证. 设备验证需要:
1. 真机
2. 编译 APK
3. 安装到 FCL
4. 启动 MC, 观察 logcat
