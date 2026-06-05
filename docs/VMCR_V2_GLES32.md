# VMCR v2 — GLES 3.2 自实现工程文档

> 版本: 0.1 (2026-06-06)
> 状态: 设计阶段, 未开工
> 前置: v1 (FCL shim 转发器) 已封存 — 见末尾"附录 A: v1 失败总结"

---

## 0. 背景与动机

### 0.1 v1 路线 (FCL 启动器 GLES 转发器) 走死

v1 的核心思路: 提供 `libGL.so` / `libEGL.so` 两个 shim, **直接 forward 到厂商 `libGLESv2.so` / `libEGL.so`**。
本意是让 MC 的 LWJGL3 通过我们的 shim 调用, 我们可以在 shim 里做白名单过滤、`GL_MAX_TEXTURE_SIZE` 兜底等。

**实测结果**: FCL 完全不 dlopen 我们的 shim. 验证:
- libGL.so / libEGL.so 的 `.init_array` 构造器从未执行
- `JNI_OnLoad` 也未调用
- `adb logcat -s VMCR-Core:V` 无任何输出
- `/data/local/tmp/vmcr_*` 文件全 0 字节
- 即便改了包名到 `com.mio.plugin.renderer.vmcr` (匹配 FCL 约定), 仍未加载

FCL 内部: 识别 `renderer=...` meta-data 仅用于 UI 显示; 实际渲染走它**自己**的 EGL 桥 + 系统 `libGLESv2.so`. 我们的 shim 在 FCL 进程里**根本不存在**.

### 0.2 走 zink / SwiftShader / ANGLE 路线

要让 MC 在 Android 上跑 Vulkan, 必须有"GLES → Vulkan"翻译层. 业界做法:

| 项目 | 翻译方向 | 代码量 | License | 我们能用吗 |
|------|----------|--------|---------|------------|
| **Mesa Zink** | OpenGL → Vulkan | ~50K LOC C++ | MIT | 可移植, 但工作量极大 |
| **ANGLE** | OpenGL ES → Vulkan/D3D | ~100K LOC C++ | BSD | 微软维护, 难上游 |
| **SwiftShader** | OpenGL ES → Vulkan (CPU) | ~80K LOC C++ | Apache 2.0 | 谷歌维护, 软件渲染 |
| **libhybris / Mesa3D** | 直接用 Mesa GL → GPU driver | 巨型 | MIT | 难在 Android 上跑 |

移植 Zink 是最干净的路线, 但工作量**不可接受** (单人就 6-12 个月).

### 0.3 我们的选择: **自写 GLES 3.2 翻译层**

目标: 写一个**最小可用的 GLES 3.2 → Vulkan 翻译层**, 让 FCL 加载我们的 `libGL.so`/`libEGL.so` 后能跑 1.7.10 原版 + 1.16+ + 1.20+ (光影可选).

设计原则:
- **EGL/GLES 是入口** — 必须是完整 EGL 1.5 + GLES 3.2 实现, 不能依赖厂商 .so
- **Vulkan 是后端** — 所有 GLES 命令翻译到 Vulkan, 走 Adreno 驱动
- **glslang + SPIRV-Tools 翻译 shader** — GLSL ES 3.20 → SPIR-V
- **Phase 1 只跑 1.7.10 原版** — 不需要光影, 不需要 Mod 兼容
- **Phase 2 跑 1.16+** — 包含 GLSL 3.30 + 大纹理 + UBO
- **Phase 3 跑 1.20+ 光影** — 可选, 需完整 GLSL 3.20 + 资源绑定

---

## 1. 目标与非目标

### 1.1 目标 (Phase 1 — MVP)

**最小可用**: MC 1.7.10 原版能进主菜单, 能加载世界, 能看见方块和天空. 无需光影.

具体:
- 完整 EGL 1.5 子集 (足够 FCL EGLBridge 调用)
- 完整 GLES 2.0 + GLES 3.0 + 关键 GLES 3.1/3.2 扩展
- GLSL ES 1.00 + 3.00 编译 (通过 glslang)
- 纹理上传 + 采样 (2D, cube map)
- VBO + IBO + 顶点属性
- Uniform Block (UBO) — GLES 3.0 引入
- Framebuffer Object (FBO) + Renderbuffer
- 基本混合 / 深度 / 模板
- 适配 MC 1.7.10 的固定管线模拟 (它仍部分依赖 GL_QUADS 等)

### 1.2 目标 (Phase 2)

- MC 1.16.5 ~ 1.19 原版
- 光影 (ShadersMod / OptiFine) — 必需支持 GLSL ES 3.30
- 大纹理 (4096+ 视口)
- 完整 sRGB 流程
- 完整 buffer storage (immutable storage)

### 1.3 非目标

- ❌ 完整 GL 4.6 兼容 (Zink 才有这规模)
- ❌ CPU 软件渲染 (SwiftShader 路线)
- ❌ MC 模组兼容 (RTX 模组, Sodium 等 — 那些基于 LWJGL3 1.19+)
- ❌ 跨 Android 版本差异修复
- ❌ 跑 Windows / Linux / iOS (只服务 Android + FCL)

---

## 2. 架构总览

```
┌──────────────────────────────────────────────────────────────────┐
│                    MC 1.7.10 / 1.20+ (Java)                       │
│                       LWJGL3 bindings                            │
└────────────────────────────┬─────────────────────────────────────┘
                             │  dlsym / eglGetProcAddress
                             ▼
┌──────────────────────────────────────────────────────────────────┐
│  libGL.so + libEGL.so (我们的 — 完整 EGL + GLES 3.2 实现)        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐            │
│  │  EGL 1.5     │  │  GLES 3.2   │  │  State Track  │            │
│  │  (入口)      │  │  (命令入口)  │  │  (状态跟踪)   │            │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘            │
│         └─────────────────┴──────────────────┘                   │
│                           ▼                                       │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │             Command Builder (命令缓冲)                      │ │
│  │  GLES draw call → Vulkan command buffer                    │ │
│  └────────────────────────┬───────────────────────────────────┘ │
└───────────────────────────┼──────────────────────────────────────┘
                            │
        ┌───────────────────┼──────────────────────┐
        ▼                   ▼                      ▼
  ┌──────────┐        ┌──────────┐         ┌────────────┐
  │ glslang  │        │  SPIR-V  │         │ Vulkan     │
  │ GLSL →   │──SPIR-V──▶ Cache  │         │ Backend    │
  │ SPIR-V   │        │  (.spv)  │         │ (vk_renderer)│
  └──────────┘        └──────────┘         └─────┬──────┘
                                                  │ VkCall
                                                  ▼
                                          ┌────────────┐
                                          │ Adreno 735 │
                                          │ Vulkan 1.3 │
                                          └────────────┘
```

**关键点**:
- 入口 (`libGL.so` + `libEGL.so`) 是**完整实现**, 不依赖厂商 .so
- 内部 EGL 维护自己的 display/context/surface, 翻译到 Vulkan surface (Android `ANativeWindow`)
- GLES 命令被 State Tracker 记录, 翻译为 Vulkan command buffer
- 着色器由 glslang 离线编译为 SPIR-V, 缓存到磁盘
- Vulkan 后端复用现有 `vmcr_vulkan.cpp` 基础设施

---

## 3. 组件拆解

### 3.1 EGL 1.5 (我们自写)

**目标 API**:
```c
EGLDisplay eglGetDisplay(EGLNativeDisplayType);
EGLBoolean eglInitialize(EGLDisplay, EGLint*, EGLint*);
EGLBoolean eglBindAPI(EGLenum);
EGLBoolean eglChooseConfig(EGLDisplay, const EGLint*, EGLConfig*, EGLint, EGLint*);
EGLContext eglCreateContext(EGLDisplay, EGLConfig, EGLContext, const EGLint*);
EGLSurface eglCreateWindowSurface(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint*);
EGLBoolean eglMakeCurrent(EGLDisplay, EGLSurface, EGLSurface, EGLContext);
EGLBoolean eglSwapBuffers(EGLDisplay, EGLSurface);
EGLBoolean eglQuerySurface(EGLDisplay, EGLSurface, EGLint, EGLint*);
// 等等
```

**实现**:
- 维护全局表: `display_map`, `context_map`, `surface_map`
- EGL 上下文 → 内部 GLES Context (含 state tracker)
- EGL Window Surface → `ANativeWindow` + Vulkan `VkSurfaceKHR`
- `eglSwapBuffers` 调 `vkQueuePresentKHR`

**翻译到 Vulkan**:
- `EGL_WINDOW_BIT` 标志 → `VkSurfaceKHR` + `VK_KHR_android_surface`
- 内部用 Vulkan 后端

### 3.2 GLES 3.2 状态跟踪

**状态分类**:
- 顶点输入 (Vertex Array Object, VAO)
- 缓冲区 (Buffer Object — VBO, IBO, UBO)
- 纹理 (Texture, Sampler)
- 着色器 (Program, Pipeline)
- 帧缓冲 (Framebuffer, Renderbuffer)
- 视口与裁剪 (Viewport, Scissor)
- 混合 (Blend State)
- 深度 / 模板 (Depth, Stencil)
- 光栅化 (Rasterizer — cull face, polygon mode, etc.)

**State Tracker 设计**:
- 全部状态用 `struct` 表示, dirty 标志位
- 每次 GL 调用只更新状态, 不立即发 Vulkan 命令
- `glDraw*` 时 flush: 状态 → Vulkan pipeline, 顶点 → Vulkan buffer, 纹理绑定 → descriptor set
- 用 dirty 标志减少冗余 set

**关键文件**:
- `src/main/cpp/gles/state_tracker.h/cpp` — 状态结构
- `src/main/cpp/gles/context.h/cpp` — GLES Context (含 state tracker)
- `src/main/cpp/gles/dirty_tracker.h` — dirty 标志

### 3.3 GLES 3.2 → Vulkan 翻译

**映射表**:

| GLES 资源 | Vulkan 等价物 |
|-----------|---------------|
| `GL_TEXTURE_2D` | `VkImage` + `VkImageView` |
| `GL_TEXTURE_CUBE_MAP` | `VkImage` (cube-compatible) + 6 个 `VkImageView` |
| `GL_ARRAY_BUFFER` | `VkBuffer` (vertex) |
| `GL_ELEMENT_ARRAY_BUFFER` | `VkBuffer` (index) |
| `GL_UNIFORM_BUFFER` | `VkBuffer` + `VkDescriptorSet` |
| `GL_FRAMEBUFFER` | `VkFramebuffer` + attachments |
| `GL_RENDERBUFFER` | `VkImage` (no sampled) |
| `GL_PROGRAM` (shader) | `VkShaderModule` (cached) + `VkPipeline` |
| `GL_VERTEX_ARRAY` | `VkPipeline` + `VkVertexInputState` |

**关键文件**:
- `src/main/cpp/gles/translate/buffer.cpp` — VBO/IBO/UBO 翻译
- `src/main/cpp/gles/translate/texture.cpp` — 纹理上传 / 采样
- `src/main/cpp/gles/translate/framebuffer.cpp` — FBO
- `src/main/cpp/gles/translate/draw.cpp` — draw call 翻译
- `src/main/cpp/gles/translate/shader.cpp` — 着色器 pipeline

### 3.4 GLSL ES 3.20 → SPIR-V

**步骤**:
1. MC 调 `glShaderSource` → 拿到 GLSL 源码
2. 我们调 glslang 编译为 SPIR-V
3. `VkShaderModuleCreateInfo` 创建 `VkShaderModule`
4. 缓存 (GLSL 源 hash → `VkShaderModule`)

**GLSL → SPIR-V 路径**:
```cpp
// glslang includes: SPIRV/GlslangToSpv.h
const char* sources[] = { glsl_source };
glslang::TShader shader(EShLangFragment);
shader.setStrings(sources, 1);
shader.setEnvInput(EShSourceGlsl, EShLangFragment, EShClientOpenGL, 100);
shader.setEnvClient(EShClientVulkan, EShTargetVulkan_1_0);  // 目标 Vulkan 1.0
shader.parse(...);
glslang::TProgram program;
program.addShader(&shader);
program.link();
glslang::SpvOptions options;
glslang::GlslangToSpv(*program.getIntermediate(EShLangFragment), spirv, &options);
```

**注意事项**:
- GLES 3.20 的内置变量 (gl_FragColor 等) 需要映射到 Vulkan 的 `layout(location=N) out vec4`
- 部分 GLES 扩展 (e.g. GL_OES_standard_derivatives) 需要手动注入

### 3.5 命令缓冲 (Command Buffer)

**设计**:
- 每帧构建一个 Vulkan command buffer
- 录制顺序: bind pipeline → bind descriptor sets → bind vertex/index buffers → push constants → draw indexed
- `vkBeginCommandBuffer` 在 `glDraw*` 时
- `vkEndCommandBuffer` + `vkQueueSubmit` 在 `eglSwapBuffers` 时

**性能优化**:
- Pipeline cache (相同状态合并)
- Render pass merging
- 资源驻留 (avoid VK_WHOLE_SIZE re-bind)

### 3.6 FCL 集成 (前端)

FCL 用 `com.mio.plugin.renderer.vmcr` 包名约定的插件. 我们:
- 在 `app/src/main/AndroidManifest.xml` 暴露 `renderer=VMCR:libGL.so:libEGL.so` meta-data
- `libGL.so` 和 `libEGL.so` 现在是**完整 EGL+GLES 实现**, 不再转发
- FCL 加载后, MC 的 LWJGL3 调我们的 EGL/GL 入口, 我们自己完成渲染

---

## 4. 关键技术决策

### 4.1 为什么不用 Mesa Zink?

| 维度 | Zink | 我们自写 |
|------|------|----------|
| 代码量 | ~50K LOC | 估 ~10-15K LOC (Phase 1) |
| 上游维护 | Mesa 社区活跃 | 我们自维护 |
| Bug 修复 | 社区贡献 | 靠自己 |
| 完整性 | GL 4.6 完整 | GLES 3.2 核心 |
| 上手时间 | 2-4 周熟悉代码 | 0 (我们自写) |
| 风险 | 集成冲突多 | 范围可控 |

**结论**: Zink 太重, 我们目标是 GLES 3.2 而非 GL 4.6, 自写更合适.

### 4.2 为什么不用 gl4es?

gl4es 是 **GL (桌面) → GLES** 翻译. 我们需要 **GLES → Vulkan** 翻译. 方向相反.

但 gl4es 的 1.x 版本正在添加 GLES 3.x 完整支持. 可借鉴 API 设计, 不能直接用.

### 4.3 Shader 编译: glslang vs shaderc

| 工具 | 用途 | 体积 |
|------|------|------|
| glslang | GLSL → SPIR-V (完整) | ~5 MB 编译后 |
| shaderc (Vulkan SDK) | GLSL/HLSL → SPIR-V (Google) | ~500 KB 编译后 |
| SPIRV-Tools | SPIR-V 优化 / 反射 | ~1 MB |

**选 glslang**: 完整支持 GLSL ES 3.20 + GL_KHR extensions. shaderc 是 glslang 的薄封装.

### 4.4 资源上传策略

**Staging buffer + copy** (Phase 1 必选):
```cpp
vkCreateBuffer(staging, HOST_VISIBLE);
memcpy(staging, client_data, size);
vkCmdCopyBufferToImage(cmd, staging, image);
```

**直接映射 (Phase 2)**:
- 需 Adreno 支持 `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT` + DEVICE_LOCAL
- 大幅降低 staging 拷贝

### 4.5 同步模型

- Phase 1: 简单 `vkQueueWaitIdle` 同步
- Phase 2: 引入 fence + semaphore, 多 frame in flight

---

## 5. 目录结构

```
src/main/cpp/
├── gles/                      # 替换现有 stub
│   ├── egl/                   # EGL 1.5 实现
│   │   ├── egl_display.cpp
│   │   ├── egl_context.cpp
│   │   ├── egl_surface.cpp
│   │   └── egl_config.cpp
│   ├── gles/                  # GLES 3.2 实现
│   │   ├── gl_state.cpp
│   │   ├── gl_buffer.cpp
│   │   ├── gl_texture.cpp
│   │   ├── gl_framebuffer.cpp
│   │   ├── gl_shader.cpp
│   │   ├── gl_program.cpp
│   │   ├── gl_draw.cpp
│   │   └── gl_queries.cpp
│   ├── translate/             # GLES → Vulkan 翻译
│   │   ├── to_vk_buffer.cpp
│   │   ├── to_vk_texture.cpp
│   │   ├── to_vk_pipeline.cpp
│   │   └── to_vk_descriptor.cpp
│   ├── shader/                # GLSL 编译
│   │   ├── glsl_compiler.cpp  # glslang 封装
│   │   ├── spirv_cache.cpp
│   │   └── builtin_inject.cpp # 注入 gl_FragColor 等
│   ├── vk_backend/            # 复用现有 vulkan 后端
│   │   ├── vk_command_buffer.cpp
│   │   ├── vk_render_pass.cpp
│   │   └── vk_pipeline_cache.cpp
│   └── entry/                 # libGL.so 入口
│       ├── gl_entry.cpp       # 改为真实实现
│       └── egl_entry.cpp      # 改为真实实现
├── core/                      # 现有 — 跨平台工具
├── vulkan/                    # 现有 — Vulkan 后端基础设施
├── loader/                    # 移除 (v1 已废)
├── jni/                       # 保留 — JNI bridge
└── platform/                  # 现有 — Android logger
```

**移除的文件**:
- `src/main/cpp/loader/*` (dlsym_vendor, gl_entry 转发逻辑)
- `src/main/cpp/gles/gles_renderer_stub.cpp` (替换为真实实现)

**保留**:
- `core/`, `vulkan/`, `jni/`, `platform/`

---

## 6. 阶段路线图

### Phase 0: 基础 (1-2 周)

- 移除 `loader/`, 调整 CMakeLists
- 重新组织 `gles/`, `vk_backend/` 目录
- 加入 glslang 子模块
- 写最小 EGL 实现: display + context (固定 config)
- 写最小 GLES Context: 只有空函数, 不实现具体绘制

**验证**: FCL 启动 MC, 创建一个空 EGL 上下文, 不渲染. FCL 报告 "EGL initialized" 即可.

### Phase 1: 1.7.10 原版 (4-8 周)

按优先级:
1. **纹理上传** (必需, MC 1.7.10 大量贴图)
2. **简单 shader 编译** (mc 用 GLSL 1.00)
3. **VBO + 顶点属性** (必需, 块和实体)
4. **FBO + Renderbuffer** (必需, MC 的后处理基本不用但初始 GUI 用)
5. **glClear + glDrawArrays/Elements** (必需)
6. **状态机: blend, depth, stencil** (必需)
7. **GL_QUADS 兼容** (1.7.10 还在用)

**验证**:
- FCL 启动 1.7.10 原版
- 看到主菜单背景
- 进入世界, 看见方块
- **不**期望: 光影, 大多数 mod, 光照正确

### Phase 2: 1.16+ (4-8 周)

- 完整 GLES 3.0 (UBO, VAO 完整语义)
- 完整 GLSL ES 3.00 (in/out blocks)
- sRGB framebuffer / texture
- Array texture
- Sampler object 独立
- Sync object (fence, semaphore)

**验证**: FCL 启动 1.20.1, 看见方块和天空, 简单光影可加载.

### Phase 3: 1.20+ 光影 (可选, 4-8 周)

- GLSL ES 3.20 完整
- 资源绑定优化 (descriptor set pooling)
- 性能优化 (pipeline cache, render pass merge)
- 大纹理流式 (>= 2048)

**验证**: 加载 BSL / SEUS / Complementary 之一, 跑 30+ fps.

### Phase 4: 完善 (持续)

- 几何/曲面细分 shader (GLES 3.2)
- 计算 shader
- AEP / EGL 扩展
- dEQP 测试套件通过率

---

## 7. 测试策略

### 7.1 单元测试 (主机端)

CMake `BUILD_TESTING=ON` 时启用. 测试:
- EGL 入口 (mock Vulkan, 只测 EGL 状态机)
- GLES state tracker
- GLSL 编译 (用内置 GLSL 1.00/3.00 测试片段)

### 7.2 集成测试 (Android)

`gradle connectedCheck`:
- 启动 MC 1.7.10 原版, 进入世界, 截图
- 与 Adreno 真实 GLES 3.2 渲染对比
- dEQP - GLES3 subset

### 7.3 性能基准

- 1.7.10 主菜单 FPS
- 1.7.10 单一世界 FPS
- 1.20.1 主世界 FPS (无光影)
- 1.20.1 + 简单光影 FPS

目标: >= 30 FPS, 与 Adreno 原生差距 < 30%.

---

## 8. 风险与缓解

| 风险 | 影响 | 缓解 |
|------|------|------|
| 1.7.10 用了 GL_QUADS 等固定管线语义 | 高 — 必须模拟 | Phase 1 第一周调研; 用 GLES 3.0 的 glDrawArrays + 索引模拟 |
| glslang 编译后体积 ~5 MB | 中 — APK 增大 | 可选: 上游 glslang-static 用 LTO 缩到 ~2 MB |
| Vulkan 后端调试复杂 | 中 | 复用现有 vmcr_vulkan.cpp 基础设施; 增量写 |
| GLSL ES 3.20 翻译精度损失 | 中 | 用 SPIRV-Cross 验证; 与 native Adreno 输出 diff |
| 1.20.1 光影 shader 极复杂 | 高 | Phase 3 才做, 失败也接受 |
| 用户设备差异 (Adreno/Mali/PowerVR) | 中 | Phase 1 只目标 Adreno 735, 后续扩展 |

### 8.1 范围蔓延控制

**强约束**:
- Phase 1 不做 1.16+ 兼容
- Phase 2 不做光影
- Phase 3 不做 Mod 兼容
- 永远不碰 desktop GL (那是 Zink 的活)

**验收标准**:
- Phase 1: MC 1.7.10 原版能进世界
- Phase 2: MC 1.20.1 原版能进世界
- Phase 3: 1.20.1 加载 BSL 跑出 30 fps

---

## 9. 依赖与子模块

需要新加的 git submodule:
- `third_party/glslang/` — GLSL → SPIR-V
- `third_party/SPIRV-Tools/` — SPIR-V 优化 (可选)

**必须**: glslang (因为它是开源 GLSL 编译的事实标准, GLES 3.20 完整支持)

**可选**:
- vma (Vulkan Memory Allocator) — 已有
- SPIRV-Cross — 反射用, 可选

---

## 10. 实施前 TODO 清单

按顺序:

1. **删除 v1 loader**:
   - `rm -rf src/main/cpp/loader/`
   - 更新 `CMakeLists.txt` 移除 `VMCR_ENABLE_LOADER` 选项
   - 更新 `app/build.gradle.kts` 移除 `vmcr_experimentalModules`

2. **重新组织 CMake**:
   - 新加 `VMCR_ENABLE_VENDOR_GLES` 选项 (默认 OFF — 因为 v1 不用 vendor)
   - 强制 `gles/` 不再是 stub, 而是核心实现

3. **加入 glslang 子模块**:
   - `git submodule add https://github.com/KhronosGroup/glslang.git third_party/glslang`
   - 写 `third_party/CMakeLists.txt` 集成

4. **写最小 EGL**:
   - `egl_display.cpp`: `eglGetDisplay` + `eglInitialize` + `eglTerminate`
   - `egl_config.cpp`: 写死 1 个 config (RGBA8 + 24/8 D/S + ES3)
   - `egl_context.cpp`: 创建 GLES Context (空实现)
   - `egl_surface.cpp`: `ANativeWindow` 包装

5. **写最小 GLES Context**:
   - `gl_state.cpp`: state 结构
   - `gl_clear.cpp`: `glClear` 实际清屏
   - `gl_viewport.cpp`: `glViewport` 记录

6. **连接现有 Vulkan 后端**:
   - `vk_command_buffer.cpp`: 单 command buffer 录制
   - `vk_swapchain.cpp`: 创建 swapchain + image view

7. **集成测试**:
   - 编译 APK
   - FCL 启动 MC 1.7.10
   - **期望**: 至少能进主菜单背景 (可能是黑色, 但 EGL initialized)

8. **开始 Phase 1 真正实现**:
   - 纹理上传 (Texture/SubImage2D)
   - Shader 编译
   - VBO/IBO + 顶点属性
   - draw call

---

## 11. 文档结构 (docs/ 目录)

- `docs/VMCR_V2_GLES32.md` — 本文档
- `docs/EGL_API_SUBSET.md` — 我们要实现的 EGL 函数清单
- `docs/GLES_API_SUBSET.md` — 我们要实现的 GLES 函数清单
- `docs/GLES_TO_VK_MAPPING.md` — 详细翻译表
- `docs/GLSL_ES_3.20_NOTES.md` — glslang 编译常见问题
- `docs/PERFORMANCE_TARGETS.md` — Phase 1 性能基线
- `docs/FAQ.md` — 常见问题

---

## 12. 决策时间表

**今天 (2026-06-06)**:
- 写本文档 ✓
- 删除 v1 loader
- 加入 glslang 子模块

**明天 (2026-06-07)**:
- 写最小 EGL
- 写最小 GLES Context
- 写最小 Vulkan command buffer
- 集成测试: 至少能进 MC 1.7.10 主菜单 (黑屏也行)

**下周**:
- 纹理上传
- Shader 编译
- VBO
- 1.7.10 进世界

**两周后**:
- 完整 Phase 1
- 1.7.10 完整可玩

---

## 附录 A: v1 失败总结 (给未来的自己/读者)

**v1 尝试的方案**: 提供 libGL.so + libEGL.so shim, 转发到厂商 libGLESv2.so + libEGL.so. 在 shim 里加白名单过滤、GL_MAX_TEXTURE_SIZE 兜底等.

**v1 失败的关键证据**:
1. libGL.so / libEGL.so 的 `.init_array` 构造器从未执行
2. `JNI_OnLoad` 从未调用
3. logcat 无任何 VMCR-Core 输出
4. `/data/local/tmp/vmcr_*` 文件全 0 字节
5. 即便改了包名匹配 FCL 约定 (`com.mio.plugin.renderer.vmcr`), 仍未加载

**v1 失败的根因**: FCL 内部有自己**完整**的 EGL 桥 (PojavLauncher 继承的代码), 它直接 dlopen 系统 `libGLESv2.so`, 完全跳过我们的 shim. 我们的 shim 在 FCL 进程里**根本不存在**.

**v1 留下的可复用资产**:
- `core/` (跨平台工具)
- `vulkan/` (Vulkan 后端基础设施)
- `jni/` (JNI bridge)
- `platform/` (Android logger)
- `include/vmcr/` 通用头
- CI 流程 (4 ABI, 4 workflow)

**v1 完全废弃**:
- `loader/` 全部 (dlsym_vendor, gl_entry 转发, egl_entry 转发)
- `gles_renderer_stub.cpp`
- v1 全部 shim 逻辑

---

> 文档结束. 实施开始.
