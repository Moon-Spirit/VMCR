# VMCR v1 失败总结 (2026-06-06 封存)

## 路线

提供 `libGL.so` + `libEGL.so` 两个 shim, 转发到厂商 `libGLESv2.so` + `libEGL.so`. 在 shim 里:
- `glGetIntegerv` 白名单过滤 + 安全最小值兜底
- `glGetString` / `glGetStringi` 字符串兜底
- `eglGetProcAddress` 优先返回 shim 函数

## 失败的铁证 (5 个独立信号全部指向同一根因)

1. **`.init_array` 构造器从未执行**:
   libGL.so 和 libEGL.so 都加了 `__attribute__((constructor))`, 在 dlopen 立即跑. 用户在 logcat 没看到任何 `VMCR-Core` 日志.

2. **`JNI_OnLoad` 从未调用**:
   libGL.so 里有 `JNI_OnLoad`, 在 `System.load` 路径触发. 没跑.

3. **logcat 静默**:
   `adb logcat -s VMCR-Core:V` 完全没有 VMCR-Core 输出.

4. **文件标记全 0**:
   构造器会写 `/data/local/tmp/vmcr_loaded` + `vmcr_egl_loaded`. 用户 `ls` 0 个文件. 即使 `/data/local/tmp/` 通常可写, scoped storage 限制不到.

5. **改了包名到 `com.mio.plugin.renderer.vmcr` (匹配 FCL 约定), 仍无效果**.

## 失败的根因

FCL 内部有**完整**的 EGL 桥 (从 PojavLauncher 继承):
- `egl_loader.c` 直接 `dlopen("libEGL.so")` 拿厂商
- `egl_bridge.c` 用 EGL 函数建上下文
- 我们的 shim **永远不被 dlopen**

用户报 "Renderer: VMCR" 是 FCL 读了我们 AndroidManifest meta-data 仅作 UI 显示, **实际渲染走 FCL 自己的 EGL 桥**.

## 教训

1. **不要相信 meta-data 检测** = 实际加载. Android 上 FCL/Pojav 的 plugin 系统 ≠ 实际 native 库加载.
2. **`.init_array` 是最可靠的下游检测手段** — 任何 dlopen 都会触发. 比检查符号导出、logcat、文件都更直接.
3. **shim 路线在 Android 上对启动器基本失效** — Android 的 EGL 桥太深, 任何 c++ shim 都需要 dlopen 链支持, 而 FCL 这种基于 Pojav 的启动器都有自己 EGL 桥.

## 留下的可复用代码

- `core/` (跨平台工具)
- `vulkan/` (Vulkan 后端基础设施, 写得很扎实)
- `jni/` (JNI bridge)
- `platform/` (Android logger)
- `include/vmcr/` (GL/Vulkan/EGL 类型定义, 头)
- CI 流程 (4 ABI 构建, 4 workflow)

## 完全废弃

- `src/main/cpp/loader/*` (dlsym_vendor, gl_entry 转发, egl_entry 转发)
- `gles_renderer_stub.cpp`
- 全部 v1 shim 逻辑 (glGetIntegerv 白名单, eglGetProcAddress 改写等)

## v2 方向

写**完整 EGL + GLES 3.2 实现**, 不依赖厂商 .so. 走 Zink/ANGLE 路线但是自己写一个最小版本.
详见 `docs/VMCR_V2_GLES32.md`.
