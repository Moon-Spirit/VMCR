0a39ef6 → 793c64b: Host Build + NDK Build CI 通过 (28 commits)

下一步: MobileGlues 风格的混合渲染器深度重构

【已做】
- third_party/ 目录 + .gitmodules (Vulkan-Hpp/VMA/vkbootstrap/glslang/SPIRV-Cross/GLM/spdlog)
- app/ FCL 插件 APK 模块
  * AndroidManifest.xml 含 fclPlugin/renderer/boatEnv/pojavEnv/minMCVer meta-data
  * minMCVer="1.7.10" 覆盖 GT: New Horizons
  * 渲染器标识: "VMCR:libGL.so:libEGL.so"
  * VMCRPluginActivity 占位 (NoDisplay theme)
  * build.gradle.kts 用 AGP 8.5, NDK 27.3.13750724
  * settings.gradle.kts 根配置

【待做】
- 重构 vk_renderer 使用 Vulkan-Hpp + VMA + vkbootstrap + spdlog
- 集成 glslang / SPIRV-Cross 编译管线 (支持 Iris 兼容)
- 加 GLM 数学库 (替换手写矩阵)
- 实装 GLES state cache (Phase 1 优化)
- 实装 JNI chunk stream (Phase 3)
- 实现 eglSwapBuffers 真实 Vulkan present
- 实际 NDK APK 验证打包

请继续按 MobileGlues-plugin 的架构完善
