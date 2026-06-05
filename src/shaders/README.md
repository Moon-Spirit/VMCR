# ===========================================================================
# SPIR-V 着色器源 (Phase 2 起填充)
#
# chunk.vert  - 顶点着色器
# chunk.frag  - 片段着色器
# post/       - 后处理 (Phase 4)
#
# 编译方式:
#   glslangValidator -V --target-env vulkan1.3 chunk.vert -o chunk.vert.spv
# ===========================================================================

此目录当前为空, Phase 2 起会包含:
- chunk.vert
- chunk.frag
- chunk_cutout.frag
- chunk_transparent.frag
- sky.vert / sky.frag
- post/tonemap.comp
- post/fxaa.comp

详细 shader 设计见 ARCHITECTURE.md § 7.
