# VMCR v2 — GLES 3.2 自实现工程文档 (修订版)

> 版本: 0.2 (2026-06-06 修订)
> 状态: 设计阶段
> 前置: v1 shim 路线已死 (FCL 完全不加载我们的 .so) — 详见 `V1_FAILURE_SUMMARY.md`

---

## 0. 决策与教训

### 0.1 v1 失败的 6 个独立证据 (再次确认)

| # | 证据 | 含义 |
|---|------|------|
| 1 | libGL.so / libEGL.so `.init_array` 构造器从未跑 | 我们的 .so 根本没 dlopen |
| 2 | `JNI_OnLoad` 从未调用 | 同上 |
| 3 | `adb logcat -s VMCR-Core:V` 空 | 同上 |
| 4 | `/data/local/tmp/vmcr_*` 全 0 字节 | 同上 |
| 5 | 包名改 `com.mio.plugin.renderer.vmcr` 仍不工作 | FCL 不是按包名索引 |
| 6 | 严格按官方 FCLRendererPlugin 模板 (namespace=`com.mio.plugin.renderer` + applicationIdSuffix + MainActivity) 仍不工作 | 模板**自己**也只是 Kotlin+预编译 .so, **没说**模板的实际加载行为 |

### 0.2 走 Zink/ANGLE 路线不可行的原因

- Zink (~50K LOC) 和 ANGLE (~100K LOC) 都是巨型项目
- 单人维护不可持续
- 我们只服务 MC + Adreno 735, 不需要 GL 4.6 全功能

### 0.3 最终决策: **自写最小化 GLES 3.2 实现**

**目标**: 写一个**完整 EGL 1.5 + GLES 2.0** 实现, 后端走 Vulkan, 让 MC 1.7.10 原版能进主菜单并加载世界。

**Phase 1 范围严格**:
- 只支持 GLES 2.0 (LWJGL3 在 ES 2.0 上下文也能跑 — LWJGL2 时代)
- 纹理上传 (2D, cube map)
- Shader 编译 (GLSL ES 1.00 → SPIR-V via glslang)
- VBO + IBO + 顶点属性
- FBO + Renderbuffer
- 混合 / 深度 / 模板
- Vulkan 后端 (复用 `vmcr_vulkan.cpp`)

**Phase 2 范围 (后续)**:
- GLES 3.0 (UBO, VAO, transform feedback)
- GLSL ES 3.00 编译
- 适配 1.16 ~ 1.19

**Phase 3 范围 (远期)**:
- GLES 3.1/3.2
- 适配 1.20+ 光影
- glslang SPIR-V 优化

---

## 1. 架构: 三层分离

```
┌──────────────────────────────────────────────────────────┐
│ Layer 1: 入口 (libEGL.so + libGL.so)                      │
│                                                          │
│  - libEGL.so: 导出 25 个 egl* 符号                        │
│  - libGL.so:  导出 150+ 个 gl* 符号                       │
│  - 不依赖任何外部 .so (包括系统 libEGL.so / libGLESv2.so) │
│  - 通过 dlsym 或直接符号引用调用 Layer 2                 │
└─────────────────────┬────────────────────────────────────┘
                      │
                      ▼
┌──────────────────────────────────────────────────────────┐
│ Layer 2: GLES 3.2 状态机 + EGL 表                         │
│                                                          │
│  - EglRegistry (display/context/surface 句柄表)          │
│  - GlesContext (per-thread GLES 状态)                    │
│  - StateTracker (当前所有 GL 状态)                        │
│  - ResourceTable (纹理/缓冲/program 句柄 → 内部对象)     │
└─────────────────────┬────────────────────────────────────┘
                      │
                      ▼
┌──────────────────────────────────────────────────────────┐
│ Layer 3: 后端 (Vulkan)                                    │
│                                                          │
│  - VmcrVkRenderer (复用现有 vulkan/ 目录)                │
│  - VkCommandBuffer 录制                                   │
│  - 资源创建: VkImage, VkBuffer, VkPipeline                │
│  - ANativeWindow → VkSurfaceKHR                          │
│  - 同步: Fence + Semaphore                                │
└──────────────────────────────────────────────────────────┘
```

**关键不变量**: Layer 1+2 完全自包含, 不需要 Android 系统 libEGL.so 或 libGLESv2.so.

---

## 2. 文件组织 (Phase 1 完整)

```
src/main/cpp/
├── egl/                        # Layer 1+2: EGL 实现
│   ├── egl_internal.h           # Display/Context/Surface 句柄 + 表
│   ├── egl_internal.cpp         # 句柄表 (thread-safe)
│   ├── egl_api.cpp              # 25 个 EGL 入口符号
│   └── egl_loader.cpp           # 加载 vk, 创建 VkInstance
│
├── gles/                       # Layer 1+2: GLES 实现
│   ├── gles_state.h             # 全局状态 (per-context)
│   ├── gles_state.cpp
│   ├── gles_context.h           # GLES Context (句柄 + 状态)
│   ├── gles_context.cpp
│   ├── gles_resource.h          # 资源类型 (Texture/Buffer/Program)
│   ├── gles_resource.cpp
│   ├── gles_buffer.cpp          # glBufferData, glBindBuffer
│   ├── gles_texture.cpp         # glTexImage2D, glBindTexture
│   ├── gles_shader.cpp          # glShaderSource, glCompileShader
│   ├── gles_program.cpp         # glLinkProgram, glUseProgram
│   ├── gles_framebuffer.cpp     # FBO + Renderbuffer
│   ├── gles_draw.cpp            # glDrawArrays, glDrawElements
│   ├── gles_clear.cpp           # glClear
│   ├── gles_state_set.cpp       # glEnable, glBlendFunc, etc.
│   ├── gles_query.cpp           # glGet*
│   └── gles_entry.cpp            # libGL.so 入口 (150+ 个 gl* 符号)
│
├── shader/                     # GLSL → SPIR-V
│   ├── glsl_compiler.cpp        # glslang 封装
│   ├── spirv_cache.cpp          # GLSL hash → SPIR-V 缓存
│   └── builtin_inject.cpp       # 注入 gl_FragColor 等
│
├── vk/                          # Layer 3: Vulkan (扩展现有)
│   ├── vk_instance.cpp          # VkInstance, VkPhysicalDevice
│   ├── vk_device.cpp            # VkDevice, VkQueue
│   ├── vk_swapchain.cpp         # VkSwapchainKHR, VkSurfaceKHR
│   ├── vk_image.cpp             # VkImage, VkImageView, VkSampler
│   ├── vk_buffer.cpp            # VkBuffer (vertex/index/uniform)
│   ├── vk_shader.cpp            # VkShaderModule
│   ├── vk_pipeline.cpp          # VkPipeline, VkPipelineLayout
│   ├── vk_command.cpp           # VkCommandBuffer 录制
│   ├── vk_renderpass.cpp        # VkRenderPass
│   ├── vk_sync.cpp              # Fence, Semaphore
│   ├── vk_descriptor.cpp        # DescriptorSet, DescriptorPool
│   └── vk_memory.cpp            # VkDeviceMemory + VMA
│
├── core/                       # 跨平台工具 (已有)
├── platform/                    # Android logger (已有)
├── jni/                         # JNI bridge (可选, Phase 1 不要)
│
└── CMakeLists.txt
```

---

## 3. 关键设计决策

### 3.1 不使用 .init_array 构造函数 (改用 JNI_OnLoad)

v1 的 `.init_array` 没被 FCL 触发, 说明 FCL 根本不加载 .so. 但**我们不能确定 FCL 永远不加载** — 这只是现状. 所以:

- Layer 1 的 libGL.so / libEGL.so 仍保留 .init_array (诊断用)
- 业务逻辑都靠 egl* / gl* 入口, 不用全局构造函数
- EGL context 创建时才初始化 Vulkan instance (按需)

### 3.2 EGL Context ↔ GLES Context 1:1

每个 `EGLContext` 对应一个 `GlesContext` (含状态机 + 资源表). `eglMakeCurrent` 切换活跃 GlesContext. `eglGetCurrentContext` 返回当前活跃.

### 3.3 Vulkan 在 eglInitialize 时创建

不要每次 eglCreateContext 都重建 VkDevice. **一次 eglInitialize → 创建 VkInstance, VkPhysicalDevice, VkDevice**. 多个 EGLContext 共享同一 VkDevice, 各自有独立 VkCommandPool.

### 3.4 资源句柄用 u32

GLES 用 `GLuint` 句柄. 我们内部 `HandleTable<u32, Texture>` 映射到内部对象. 无效句柄 = 0.

### 3.5 Shader 缓存: GLSL 源 hash → SPIR-V

每次 `glCompileShader` 编译 GLSL 源, 算 SHA-256:
- 命中缓存: 直接用缓存的 SPIR-V
- 未命中: 调 glslang 编译, 存缓存

缓存路径: `/data/data/com.mio.plugin.renderer.vmcr/files/spirv_cache/`.

### 3.6 Vulkan 不暴露给 MC

Layer 1 的 EGL 入口对 MC 完整. MC 不知道 Vulkan 存在. 我们自己翻译.

### 3.7 FCL 兼容性 — 不依赖任何系统 EGL/GLES 库

关键: 我们的 libGL.so / libEGL.so **不调用** 系统 `libEGL.so` 或 `libGLESv2.so`. 完全自包含.

FCL 的 `egl_loader.c` 用 `dlopen("libEGL.so")` 加载. 当 FCL 加载我们时, 拿到的是我们. 我们不接受任何 dlopen 调用, 自己处理.

---

## 4. 入口符号清单 (Phase 1)

### 4.1 libEGL.so 必须导出 (25 个)

```
eglGetDisplay
eglInitialize
eglTerminate
eglBindAPI
eglGetCurrentDisplay
eglGetCurrentContext
eglGetCurrentSurface
eglMakeCurrent
eglSwapBuffers
eglSwapInterval
eglChooseConfig
eglGetConfigAttrib
eglCreateWindowSurface
eglCreatePbufferSurface
eglDestroySurface
eglCreateContext
eglDestroyContext
eglGetError
eglQueryContext
eglQuerySurface
eglQueryString
eglWaitClient
eglWaitNative
eglReleaseThread
eglGetProcAddress
```

### 4.2 libGL.so 必须导出 (GLES 2.0 完整集, ~150 个)

分 7 类:

**State (40)**:
glGetError, glEnable, glDisable, glIsEnabled, glGetBooleanv, glGetIntegerv, glGetFloatv, glGetString, glHint, glPixelStorei, glReadPixels, ...

**Buffer (10)**:
glGenBuffers, glDeleteBuffers, glBindBuffer, glBufferData, glBufferSubData, glIsBuffer, ...

**Texture (20)**:
glGenTextures, glDeleteTextures, glBindTexture, glTexImage2D, glTexSubImage2D, glTexParameteri, glTexParameterf, glTexParameteriv, glTexParameterfv, glActiveTexture, glGenerateMipmap, glCompressedTexImage2D, glCompressedTexSubImage2D, glPixelStorei, ...

**Shader (15)**:
glCreateShader, glDeleteShader, glShaderSource, glCompileShader, glGetShaderiv, glGetShaderInfoLog, glCreateProgram, glDeleteProgram, glAttachShader, glDetachShader, glLinkProgram, glUseProgram, glGetProgramiv, glGetProgramInfoLog, glGetUniformLocation, glGetAttribLocation, glGetActiveUniform, glGetActiveAttrib, glUniform*, glVertexAttrib*, ...

**Framebuffer (15)**:
glGenFramebuffers, glDeleteFramebuffers, glBindFramebuffer, glFramebufferTexture2D, glFramebufferRenderbuffer, glCheckFramebufferStatus, glGenRenderbuffers, glDeleteRenderbuffers, glBindRenderbuffer, glRenderbufferStorage, glIsFramebuffer, glIsRenderbuffer, ...

**Draw (10)**:
glDrawArrays, glDrawElements, glClear, glClearColor, glClearDepthf, glClearStencil, glViewport, glScissor, glLineWidth, glPolygonOffset, glCullFace, glFrontFace, glDepthFunc, glDepthMask, glColorMask, glStencilFunc, glStencilMask, glStencilOp, glBlendFunc, glBlendEquation, ...

**Resource (15)**:
glIsTexture, glIsBuffer, glIsProgram, glIsShader, glGetTexParameter*, glGetBufferParameteriv, glGetVertexAttrib*, ...

(完整 150+ 个 GLES 2.0 入口, 见 GLES 2.0 规范表 6.1)

### 4.3 Phase 1 范围声明

| 类别 | 数量 | 复杂度 |
|------|------|--------|
| 占位/状态查询 | ~40 | 低 (记到全局变量即可) |
| Buffer 操作 | ~10 | 中 (分配 VkBuffer) |
| Texture 操作 | ~20 | 高 (VkImage 分配, 格式映射) |
| Shader/Program | ~20 | 高 (GLSL 编译, pipeline 缓存) |
| FBO | ~15 | 中 (VkFramebuffer, attachments) |
| Draw | ~10 | 高 (state -> pipeline -> cmd buffer) |
| Resource | ~15 | 低 |
| **总计** | **~130** | |

---

## 5. 阶段路线图 (修订)

### Phase 0: 骨架 (1 周) — 立即开始

**目标**: EGL 能跑, 没有任何 GL 命令, 但 MC 至少能进 EGL 初始化阶段

- [ ] 删除 `src/main/cpp/loader/` (v1 shim 转发器)
- [ ] 删除 `src/main/cpp/gles/` (v1 gles 转发器)
- [ ] 删除 `include/vmcr/{vendor_gl,vendor_egl,gl_safe,dl,backend,backend_router}.h`
- [ ] 删除 `src/main/cpp/vulkan/vmcr_probe.cpp` (probe tool, 不需要)
- [ ] 简化 `CMakeLists.txt`: 移除 `VMCR_ENABLE_LOADER`, `VMCR_ENABLE_VULKAN_HPP`, 等
- [ ] 加 `glslang` 子模块
- [ ] 写 `egl_internal.{h,cpp}`: 句柄表
- [ ] 写 `egl_api.cpp`: 25 个 egl* 入口 (Phase 0 仅返回成功, 翻译到 Vulkan 在 Phase 1)
- [ ] 写 `gl_state.{h,cpp}`: 全局状态
- [ ] 写 `gles_entry.cpp`: 130 个 gl* 入口 (Phase 0: 仅占位, 立即返回)
- [ ] 写 `vk_loader.cpp`: 初始化 VkInstance + VkDevice (按需)
- [ ] `AndroidManifest.xml` 保持模板风格 (namespace=`com.mio.plugin.renderer`, suffix=`.vmcr`, MainActivity)
- [ ] **本地构建 + push CI + 测试**: FCL 加载我们的 libEGL.so, 至少能进 EGL init

**Phase 0 验收**: 用户在 FCL 里启动 MC 1.7.10, **不再崩在 EGL 初始化** (现在崩在 GL 命令). logcat 应该看到我们 EGL 日志.

### Phase 1: 跑 MC 1.7.10 (4-6 周)

按 6 个 milestone:
1. **M1: Texture 上传** (1 周) — glTexImage2D → VkImage
2. **M2: Shader 编译** (1 周) — GLSL 1.00 → SPIR-V via glslang
3. **M3: VBO + 顶点属性** (1 周) — glBufferData + glVertexAttribPointer
4. **M4: FBO + 渲染** (1 周) — FBO 创建, 切换
5. **M5: glClear + glDrawArrays/Elements** (1 周) — 完整 draw call 翻译
6. **M6: 状态机** (1 周) — blend, depth, stencil

**Phase 1 验收**: MC 1.7.10 原版能进主菜单, 能加载世界, 能看见方块.

### Phase 2: 跑 MC 1.16~1.20 (4-6 周)

- GLES 3.0 (UBO, VAO 完整语义, transform feedback)
- GLSL ES 3.00
- sRGB 流程
- 性能优化 (pipeline cache, render pass merge)

### Phase 3: 光影 (后续, 看情况)

- GLSL ES 3.20
- 资源绑定优化
- 大纹理流式

---

## 6. 关键技术参考 (推荐)

### 6.1 必读

| 文档 | 来源 |
|------|------|
| OpenGL ES 2.0 规范 | https://registry.khronos.org/OpenGL/specs/es/2.0/es_full_spec_2.0.pdf |
| OpenGL ES 3.2 规范 | https://registry.khronos.org/OpenGL/specs/es/3.2/es_spec_3.2.pdf |
| Vulkan 1.3 规范 | https://registry.khronos.org/vulkan/specs/1.3-khr-extensions/html/vkspec.html |
| glslang README | https://github.com/KhronosGroup/glslang |

### 6.2 参考实现 (按代码量从小到大)

| 项目 | 行数 | 学什么 |
|------|------|--------|
| **regal** (Mesa) | ~30K | GL state machine 设计 |
| **gl4es** | ~15K | GL → GLES 翻译模式 |
| **Zink** (Mesa) | ~50K | GL → Vulkan 翻译 |
| **ANGLE** | ~100K | GLES → Vulkan 翻译 (我们的目标) |

我们借鉴 Zink 的翻译思路, 但范围缩到 GLES 2.0 核心.

### 6.3 关键算法 (Mesa3D 风格)

1. **State Tracker**: 维护当前所有 GL 状态. 每次 GL 调用更新状态, 不立即发 Vulkan 命令.
2. **Dirty Tracker**: 每个 draw call 比较当前状态和 pipeline, 计算 dirty flags, 只更新变化的部分.
3. **Pipeline Cache**: 相同 state hash 的 pipeline 复用 VkPipeline 对象.
4. **Resource Tracker**: 纹理/缓冲/Program 句柄 → 内部对象. 析构时释放.

---

## 7. 依赖 (新加)

| 依赖 | 来源 | 集成方式 |
|------|------|---------|
| glslang | https://github.com/KhronosGroup/glslang | git submodule, 3rd party |
| SPIRV-Tools | https://github.com/KhronosGroup/SPIRV-Tools | git submodule (可选, Phase 1 不要) |
| VMA | https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator | 已有, 复用 |
| Vulkan | NDK 自带 | 已有 |

---

## 8. 风险与缓解

| 风险 | 影响 | 缓解 |
|------|------|------|
| GLSL ES 1.00 翻译到 SPIR-V 出错 | 高 — 编译失败 | 用 glslang 标准库, 写 dEQP 兼容测试 |
| Vulkan 后端性能差 | 中 | Pipeline cache, render pass merge, 资源驻留 |
| 1.7.10 用的某些 GLES 2.0 边缘特性 | 中 | 留 1 周 buffer 处理例外 |
| glslang 编译后体积大 (~3MB) | 中 | 用 LTO, 或者用 spvc (Google's shaderc) 替代 |
| 单人维护风险 | 高 | 文档化所有设计决策, 写单元测试 |

### 8.1 范围控制

**严格不做的**:
- ❌ 桌面 GL (GL 4.6) — Zink 的事
- ❌ MC Mod 兼容 (RTX, Sodium)
- ❌ OpenGL ES 1.0 兼容
- ❌ 非 Android 平台

**可视情况做的**:
- ⚠️ OpenGL ES 3.1/3.2 (Phase 3)
- ⚠️ 光影 (Phase 3)
- ⚠️ 跨厂商 (PowerVR, Mali)

---

## 9. 实施 Checklist (Phase 0)

按顺序:

1. [ ] 备份: 当前代码已经 commit + push, 主分支干净
2. [ ] 删除 v1 死代码:
   - [ ] `rm -rf src/main/cpp/loader/`
   - [ ] `rm -rf src/main/cpp/gles/`
   - [ ] `rm -f include/vmcr/{vendor_gl,vendor_egl,gl_safe,dl,backend,backend_router}.h`
   - [ ] `rm -f src/main/cpp/vulkan/vmcr_probe.cpp`
3. [ ] 简化 `CMakeLists.txt`:
   - [ ] 移除 `VMCR_ENABLE_LOADER` 选项
   - [ ] 移除 `gles/CMakeLists.txt` 子目录
   - [ ] 移除 `loader/CMakeLists.txt` 子目录
4. [ ] 写 EGL 内部表: `egl_internal.{h,cpp}`
5. [ ] 写 25 个 EGL 入口: `egl_api.cpp`
6. [ ] 写 GL 状态: `gl_state.{h,cpp}`
7. [ ] 写 130 个 GL 入口 (占位): `gles_entry.cpp`
8. [ ] 写 Vulkan 加载器: `vk_loader.cpp`
9. [ ] 本地构建 (gradle assembleRelease)
10. [ ] 修复编译错误
11. [ ] 验证 APK 生成
12. [ ] commit + push
13. [ ] CI 通过后, 用户下载新 APK 测试

---

## 10. 时间预期 (单人)

| Phase | 时间 | 难度 |
|-------|------|------|
| 0 骨架 | 1 周 | ★ |
| 1 MC 1.7.10 | 4-6 周 | ★★★ |
| 2 MC 1.16~1.20 | 4-6 周 | ★★★★ |
| 3 光影 | 4-8 周 | ★★★★★ |
| 4 完善 | 持续 | ★★ |

Phase 0+1 = 5-7 周后, MC 1.7.10 原版可玩.

---

## 11. 文档结构 (docs/)

- `VMCR_V2_GLES32.md` — 本文档 (路线图)
- `V1_FAILURE_SUMMARY.md` — v1 失败归档
- `PHASE0_CHECKLIST.md` — Phase 0 详细 TODO (新建, 见下)
- `GLES_API_TABLE.md` — 130 个 GL 入口的详细分类 (Phase 0 后期)
- `EGL_VK_MAPPING.md` — EGL → Vulkan 翻译表 (Phase 1)
- `GLES_VK_MAPPING.md` — GLES → Vulkan 翻译表 (Phase 1)

---

> 修订结束. Phase 0 实施开始.
