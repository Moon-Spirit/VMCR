# Phase 0 实施清单 (本周末目标)

> 目标: 让 MC 1.7.10 至少能完成 EGL 初始化, 崩在 GL 命令 (而非 EGL 错)
> 时间: 1 周 (2026-06-06 ~ 2026-06-13)

---

## Day 1 (今天): 清理 + 骨架

### 上午: 删 v1 死代码

- [ ] `rm -rf src/main/cpp/loader/`
- [ ] `rm -rf src/main/cpp/gles/`
- [ ] `rm -f include/vmcr/{vendor_gl,vendor_egl,gl_safe,dl,backend,backend_router}.h`
- [ ] `rm -f src/main/cpp/vulkan/vmcr_probe.cpp`
- [ ] 检查: `find src/main/cpp include/vmcr -name '*.cpp' -o -name '*.h' | sort`
- [ ] 验证删除后还能 `git status` 看到

### 下午: 简化 CMakeLists

- [ ] 打开 `CMakeLists.txt`, 移除:
  - `add_subdirectory(third_party EXCLUDE_FROM_ALL)` (Phase 0 暂时不需要第三方)
  - `add_subdirectory(src/main/cpp/core)` (保留 — 通用工具)
  - `add_subdirectory(src/main/cpp/vulkan)` (保留 — Phase 1 用)
  - `add_subdirectory(src/main/cpp/loader)` (移除)
  - `add_subdirectory(src/main/cpp/gles)` (移除 — 重建)
  - `add_subdirectory(src/main/cpp/platform)` (保留)
  - `add_subdirectory(src/main/cpp/jni)` (Phase 0 移除 — 简单)
- [ ] 打开 `cmake/Options.cmake`:
  - 移除 `VMCR_ENABLE_LOADER` 选项
  - 移除 `VMCR_USE_VULKAN_HPP`, `VMCR_USE_VMA` 等 (Phase 0 用最小 Vulkan)
  - 保留 `VMCR_ENABLE_VULKAN`, `VMCR_ENABLE_VERBOSE`
- [ ] 打开 `app/build.gradle.kts`:
  - 移除 CMake flags: `VMCR_USE_GLSLANG=ON`, `VMCR_USE_VMA=ON` 等
  - 只留 `-DVMCR_ENABLE_VULKAN=ON`

### 验证: 本地 gradle 还能编 (即使空)

- [ ] `cd app; gradle assembleRelease`
- [ ] 看到 BUILD SUCCESSFUL
- [ ] APK 生成: `app/build/outputs/apk/release/app-release.apk`
- [ ] commit + push: "Phase 0: 删 v1, 简化 CMake"

---

## Day 2: 写 EGL 内部表

### 文件: `src/main/cpp/egl/egl_internal.h`

定义:
```cpp
namespace vmcr::egl {

struct DisplayImpl { ... };  // VkInstance, VkPhysicalDevice, VkDevice
struct ContextImpl { ... };  // VkCommandPool, 关联 GlesContext
struct SurfaceImpl { ... };  // ANativeWindow, VkSurfaceKHR, VkSwapchainKHR
struct ConfigImpl { ... };   // EGLConfig 数据

class EglRegistry {
public:
    static EglRegistry& instance();
    DisplayImpl* get_display(EGLDisplay_t h);
    ContextImpl* get_context(EGLContext_t h);
    // ...
};

}
```

### 文件: `src/main/cpp/egl/egl_internal.cpp`

实现句柄表 (thread-safe, 用 std::mutex).

### 验证

- [ ] `cd app; gradle assembleRelease` 通过
- [ ] 编译无错

---

## Day 3: 写 25 个 EGL 入口

### 文件: `src/main/cpp/egl/egl_api.cpp`

每个 EGL 入口都是:
```cpp
extern "C" EGLBoolean eglInitialize(EGLDisplay dpy, EGLint* major, EGLint* minor) {
    auto* d = EglRegistry::instance().get_display(dpy);
    if (!d) return EGL_FALSE;  // 设置 eglGetError
    *major = 1; *minor = 5;
    return EGL_TRUE;
}
```

注意: Phase 0 **不**翻译到 Vulkan, 仅返回 SUCCESS, 记录句柄.

### 验证

- [ ] gradle 编译通过
- [ ] APK 包含 `libEGL.so` 含 25 个 egl* 符号
- [ ] `llvm-readelf --dyn-syms build/.../libEGL.so | grep egl` 应看到全部 25 个

---

## Day 4: 写 GL 状态 + 130 个 GL 入口 (占位)

### 文件: `src/main/cpp/gles/gl_state.h`

```cpp
namespace vmcr::gles {
struct GlState {
    bool initialized = false;
    void* current_context = nullptr;
    std::array<int, 4> viewport = {0,0,0,0};
    int gl_error = 0;
    uint64_t draw_calls = 0;
};
GlState& state();
}
```

### 文件: `src/main/cpp/gles/gles_entry.cpp`

130 个 gl* 入口, 每个都是:
```cpp
extern "C" void glViewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    auto& s = gles::state();
    s.viewport = {x, y, w, h};
}
```

或者完全 no-op:
```cpp
extern "C" void glDepthMask(GLboolean flag) {
    // Phase 0: ignore
}
```

为了 1.7.10 至少能进 GL 初始化, **所有 GL 命令** 都该安全地 no-op 或记到 state.

### 验证

- [ ] gradle 编译通过
- [ ] APK 包含 `libGL.so` 含 130+ gl* 符号

---

## Day 5: 写 Vulkan 加载器 (按需初始化)

### 文件: `src/main/cpp/vk/vk_loader.cpp`

```cpp
namespace vmcr::vk {
bool ensure_vk_instance(DisplayImpl* dpy);
bool ensure_vk_device(DisplayImpl* dpy);
}
```

Phase 0: 仅 stub, 返回 false. Phase 1 才真正创建.

实际上, **Phase 0 不需要 Vulkan**. 因为 Phase 0 不渲染, 只需 EGL/GL 接口. Vulkan 在 Phase 1 引入.

### 验证

- [ ] gradle 编译通过
- [ ] libGL.so + libEGL.so 都能在 dlsym 找到对应符号

---

## Day 6-7: 集成测试

### 步骤

1. [ ] commit + push 到 main
2. [ ] CI 跑通
3. [ ] 用户下载新 APK
4. [ ] FCL 启动 MC 1.7.10
5. [ ] **期望**: 不再崩在 EGL init (之前是 EGL_BAD_DISPLAY 之类)
6. [ ] **期望**: 崩在 GL 命令 (因为我们的 GL 是占位, MC 会发现没渲染)
7. [ ] **期望**: `adb logcat -s VMCR-Core:V` 看到 EGL 日志
8. [ ] 把 FCL 日志 + logcat 给开发者, 决定 Phase 1 优先项

### Phase 0 验收标准

- [ ] APK 安装成功
- [ ] FCL 识别 "Renderer: VMCR" (已有)
- [ ] FCL 启动 MC, **不**因 EGL 问题崩
- [ ] logcat 出现 VMCR-Core 日志
- [ ] 启动崩溃位置 **比之前更深** (从 EGL 进到 GL)

---

## Day 8+: 进入 Phase 1 (M1 纹理)

Phase 0 完成后, 用户给反馈, 我们按反馈进 M1 (纹理).

如果 Phase 0 顺利, M1 估 1 周.
