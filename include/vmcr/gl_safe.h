// ===========================================================================
// include/vmcr/gl_safe.h
//
// libGL.so 转发器: glGetIntegerv / glGetString 的安全默认值
//
// 问题: 1.7.10 + LWJGL3 + FCL + Adreno 735 组合下, 当 MC 调用
//        glGetIntegerv(GL_MAX_TEXTURE_SIZE, ...) 时, vendor 返回 0
//        (可能因为 context 状态, 也可能 Adreno 驱动未正确初始化),
//        导致 TextureAtlas.maxTextureSize = 0,
//        之后加载任意贴图 (e.g. lava_flow 32x32) 时抛出
//        "Unable to fit: lava_flow - size: 32x32 - Maybe try a lower
//        resolution resourcepack?"
//
// 1.20.1 + LWJGL3 + FCL 在 1.20.1 渲染循环中调用
//        glGetIntegerv(0x8064, ...), vendor (Adreno) 报 GL_INVALID_ENUM
//        'texture target 32868 is an invalid enum', 错误日志刷屏导致
//        游戏卡死.
//
// 策略: 用白名单 (whitelist) 判断 pname 是否属于 GLES 3.2.
//       - 白名单内: 转发到 vendor, 之后对已知关键 pname 做最小值兜底
//         防止 MC 把 0 误判为有效上限而崩溃.
//       - 白名单外: 直接 *params = 0, 避免触发 GL_INVALID_ENUM
//         (LWJGL3 探测 desktop-only pname 时会被此分支静默掉).
// ===========================================================================
#pragma once

#include "vmcr/vendor_gl.h"
#include <cstdint>
#include <cstring>

namespace vmcr::gl_safe {

// ---------------------------------------------------------------------------
// 已知 desktop-only / MC 探测用的 pname 安全默认值
// 来自 GLES 3.2 规范 + Adreno 735 实测值
// ---------------------------------------------------------------------------
constexpr GLint kAdrenoMaxTextureSize       = 16384;   // GL_MAX_TEXTURE_SIZE
constexpr GLint kSafeMaxTextureSize         = 4096;    // 兜底最小 (足以装 MC 32x 材质)
constexpr GLint kSafeMaxRenderbufferSize    = 4096;
constexpr GLint kSafeMaxViewportDims        = 4096;
constexpr GLint kSafeMaxVertexAttribs       = 16;      // GLES 3.0+ 规范要求 ≥ 16
constexpr GLint kSafeMaxVertexUniformVectors = 256;    // GLES 3.0 规范要求 ≥ 256
constexpr GLint kSafeMaxVaryingVectors      = 15;      // GLES 3.0 规范要求 ≥ 15
constexpr GLint kSafeMaxFragmentUniformVectors = 224;  // GLES 3.0 规范要求 ≥ 224
constexpr GLint kSafeMaxVertexTextureImageUnits = 16;
constexpr GLint kSafeMaxCombinedTextureImageUnits = 32;
constexpr GLint kSafeMaxTextureImageUnits  = 16;      // fragment stage
constexpr GLint kSafeMaxSamples             = 4;       // MSAA 上限
constexpr GLint kSafeMaxElementsVertices    = 0;       // GLES 不支持
constexpr GLint kSafeMaxElementsIndices     = 0;       // GLES 不支持
constexpr GLint kSafeMaxTextureUnits        = 8;       // 兼容固定管线 (老旧 MC 代码可能探测)

// 颜色 / 深度 / 模板缓冲位深 - 用 vendor 真实值, 0 时兜底
constexpr GLint kSafeColorBits = 8;        // RGB 各通道
constexpr GLint kSafeAlphaBits = 8;
constexpr GLint kSafeDepthBits = 24;
constexpr GLint kSafeStencilBits = 8;
constexpr GLint kSafeSubpixelBits = 4;

// GL_DOUBLEBUFFER / GL_STEREO / GL_AUX_BUFFERS - desktop only
constexpr GLint kSafeDoubleBuffer = 1;     // ES 总是双缓冲
constexpr GLint kSafeStereo = 0;
constexpr GLint kSafeAuxBuffers = 0;
constexpr GLint kSafeSampleBuffers = 0;    // 0 unless MSAA enabled
constexpr GLint kSafeSamples = 0;          // 0 unless MSAA enabled

// ---------------------------------------------------------------------------
// 已知的 GL 枚举常量 (避免引入 <GLES*/gl.h> 头)
// ---------------------------------------------------------------------------
namespace pname {
    constexpr GLenum MAX_TEXTURE_SIZE                = 0x0D33;
    constexpr GLenum MAX_RENDERBUFFER_SIZE           = 0x84E8;
    constexpr GLenum MAX_VIEWPORT_DIMS               = 0x0D3A;
    constexpr GLenum MAX_VERTEX_ATTRIBS              = 0x8869;
    constexpr GLenum MAX_VERTEX_UNIFORM_VECTORS      = 0x8DFB;
    constexpr GLenum MAX_VARYING_VECTORS             = 0x8DFC;
    constexpr GLenum MAX_FRAGMENT_UNIFORM_VECTORS    = 0x8DFD;
    constexpr GLenum MAX_VERTEX_TEXTURE_IMAGE_UNITS  = 0x8B4B;
    constexpr GLenum MAX_COMBINED_TEXTURE_IMAGE_UNITS = 0x8B4D;
    constexpr GLenum MAX_TEXTURE_IMAGE_UNITS         = 0x8B4C;  // ES fragment stage
    constexpr GLenum MAX_TEXTURE_UNITS               = 0x84E2;  // desktop
    constexpr GLenum MAX_SAMPLES                     = 0x8D57;
    constexpr GLenum MAX_ELEMENTS_VERTICES           = 0x80E8;
    constexpr GLenum MAX_ELEMENTS_INDICES            = 0x80E9;
    constexpr GLenum NUM_COMPRESSED_TEXTURE_FORMATS  = 0x86A2;
    constexpr GLenum COMPRESSED_TEXTURE_FORMATS      = 0x86A3;

    // 颜色 / 深度 / 模板
    constexpr GLenum RED_BITS                        = 0x0D52;
    constexpr GLenum GREEN_BITS                      = 0x0D53;
    constexpr GLenum BLUE_BITS                       = 0x0D54;
    constexpr GLenum ALPHA_BITS                      = 0x0D55;
    constexpr GLenum DEPTH_BITS                      = 0x0D56;
    constexpr GLenum STENCIL_BITS                    = 0x0D57;
    constexpr GLenum SUBPIXEL_BITS                   = 0x0D50;

    // Buffer
    constexpr GLenum DOUBLEBUFFER                    = 0x0C32;
    constexpr GLenum STEREO                          = 0x0C33;
    constexpr GLenum AUX_BUFFERS                     = 0x0C00;
    constexpr GLenum SAMPLE_BUFFERS                  = 0x80A8;
    constexpr GLenum SAMPLES                         = 0x80A9;

    // String enums (for glGetString)
    constexpr GLenum VERSION                         = 0x1F02;
    constexpr GLenum VENDOR                          = 0x1F00;
    constexpr GLenum RENDERER                        = 0x1F01;
    constexpr GLenum SHADING_LANGUAGE_VERSION        = 0x8B8C;
    constexpr GLenum EXTENSIONS                      = 0x1F03;

    // GL 3.0+ context
    constexpr GLenum MAJOR_VERSION                   = 0x821B;
    constexpr GLenum MINOR_VERSION                   = 0x821C;
    constexpr GLenum NUM_EXTENSIONS                  = 0x821D;
    constexpr GLenum CONTEXT_FLAGS                   = 0x821E;
}

// ---------------------------------------------------------------------------
// GLES 3.2 规范中 glGetIntegerv 接受的 pname 白名单
// 来自 OpenGL ES 3.2 (April 2015) 表格 6.5 / 6.6 / 6.7
//
// 不用巨型表格 (0x20000 元素, 128KB), 用 is_gles_pname() 函数精确判定.
// 列出所有 GLES 3.2 接受的 GLenum 值, 命中即返回 true.
// 命中失败 -> 视为 desktop-only, 返回 0 跳过 vendor 调用.
// ---------------------------------------------------------------------------
inline bool is_gles_pname(GLenum pname) {
    switch (pname) {
        // ---- Boolean ----
        case 0x0B44: case 0x0B45: case 0x0BE2: case 0x0BD0:    // CULL_FACE/MODE, BLEND, DITHER
        case 0x0B71: case 0x0B90: case 0x0C11: case 0x8037:    // DEPTH_TEST, STENCIL_TEST, SCISSOR_TEST, POLY_OFFSET_FILL
        case 0x8013:                                            // SAMPLE_COVERAGE_INVERT
        case 0x8C89:                                            // RASTERIZER_DISCARD
        case 0x8D69:                                            // PRIMITIVE_RESTART_FIXED_INDEX
        // ---- Integer single ----
        case 0x0D50: case 0x0D52: case 0x0D53: case 0x0D54:    // SUBPIXEL_BITS, RED/GREEN/BLUE_BITS
        case 0x0D55: case 0x0D56: case 0x0D57: case 0x0C32:    // ALPHA/DEPTH/STENCIL_BITS, DOUBLEBUFFER
        case 0x0C33: case 0x0C00: case 0x80A8: case 0x80A9:    // STEREO, AUX_BUFFERS, SAMPLE_BUFFERS, SAMPLES
        case 0x80A0:                                            // ... (扩展)
        case 0x0B21: case 0x0B46:                              // LINE_WIDTH, FRONT_FACE
        case 0x80E8: case 0x80E9:                              // MAX_ELEMENTS_VERTICES/INDICES
        case 0x846E: case 0x84E2:                              // ALIASED_LINE_WIDTH_RANGE, MAX_TEXTURE_UNITS
        case 0x84E8: case 0x84FF: case 0x0D33:                 // MAX_RENDERBUFFER_SIZE, MAX_TEXTURE_MAX_ANISOTROPY_EXT, MAX_TEXTURE_SIZE
        case 0x0D3A: case 0x8869:                              // MAX_VIEWPORT_DIMS, MAX_VERTEX_ATTRIBS
        case 0x8B4B: case 0x8B4C: case 0x8B4D:                 // MAX_VERTEX/MAX_TEXTURE/MAX_COMBINED_TEXTURE_IMAGE_UNITS
        case 0x8073: case 0x8CDF:                              // MAX_3D_TEXTURE_SIZE, MAX_COLOR_ATTACHMENTS
        case 0x8824: case 0x851C: case 0x88FF:                 // MAX_DRAW_BUFFERS, MAX_CUBE_MAP_TEXTURE_SIZE, MAX_ARRAY_TEXTURE_LAYERS
        case 0x8D57: case 0x0E45:                              // MAX_SAMPLES, MAX_TEXTURE_LOD_BIAS
        case 0x8DFB: case 0x8DFC: case 0x8DFD:                 // MAX_VERTEX/VARYING/FRAGMENT_UNIFORM_VECTORS
        case 0x8A2B: case 0x8A2D: case 0x8A2E:                 // MAX_VERTEX/FRAGMENT/COMBINED_UNIFORM_BLOCKS
        case 0x8A2F: case 0x8A30:                              // MAX_UNIFORM_BUFFER_BINDINGS, MAX_UNIFORM_BLOCK_SIZE
        case 0x8A31: case 0x8A33: case 0x8A49:                 // MAX_COMBINED_VERTEX/FRAGMENT_UNIFORM_COMPONENTS, MAX_*
        case 0x8C8A: case 0x8C8B: case 0x8C8C:                 // MAX_TRANSFORM_FEEDBACK_*
        case 0x8E59: case 0x9111: case 0x8905:                 // MAX_SAMPLE_MASK_WORDS, MAX_SERVER_WAIT_TIMEOUT, MAX_PROGRAM_TEXEL_OFFSET
        case 0x8904:                                            // MIN_PROGRAM_TEXEL_OFFSET
        case 0x9122: case 0x9125: case 0x0D32:                 // MAX_VERTEX_OUTPUT_COMPONENTS, MAX_FRAGMENT_INPUT_COMPONENTS, MAX_CLIP_DISTANCES
        // ---- Integer pair (clip) ----
        case 0x0B70: case 0x0C10:                             // DEPTH_RANGE, SCISSOR_BOX
        // ---- Integer 4-tuple ----
        case 0x0BA2:                                            // VIEWPORT
        case 0x0C22:                                            // COLOR_CLEAR_VALUE
        case 0x0C23:                                            // COLOR_WRITEMASK
        // ---- Integer pointer / buffer binding ----
        case 0x8069: case 0x806A: case 0x8514:                 // TEXTURE_BINDING_2D/3D/CUBE_MAP
        case 0x8C1D: case 0x8D67: case 0x8C2C:                 // TEXTURE_BINDING_2D_ARRAY/EXTERNAL_OES, TEXTURE_BINDING_BUFFER
        case 0x8895: case 0x88ED: case 0x88EF:                 // ELEMENT_ARRAY_BUFFER_BINDING, PIXEL_PACK/UNPACK_BUFFER_BINDING
        case 0x8B8D:                                            // CURRENT_PROGRAM
        case 0x8CA7:                                            // RENDERBUFFER_BINDING
        case 0x825A:                                            // PROGRAM_PIPELINE_BINDING
        case 0x80C6: case 0x80C8: case 0x80CA: case 0x80CB:    // BLEND_SRC_RGB/DST_RGB/SRC_ALPHA/DST_ALPHA
        case 0x8009: case 0x883D:                              // BLEND_EQUATION_RGB/ALPHA
        case 0x8005:                                            // BLEND_COLOR
        // ---- Draw buffer ----
        case 0x8825: case 0x8826: case 0x8827: case 0x8828:    // DRAW_BUFFER0..3
        case 0x8829: case 0x882A: case 0x882B: case 0x882C:
        case 0x882D: case 0x882E: case 0x882F: case 0x8830:
        case 0x8831: case 0x8832: case 0x8833: case 0x8834:
        // ---- 3.0+ context ----
        case 0x821B: case 0x821C:                              // MAJOR_VERSION, MINOR_VERSION
        case 0x821D: case 0x821E:                              // NUM_EXTENSIONS, CONTEXT_FLAGS
        case 0x87FE: case 0x87FF:                              // NUM_PROGRAM_BINARY_FORMATS, PROGRAM_BINARY_FORMATS
        case 0x8DF9: case 0x8DF8:                              // NUM_SHADER_BINARY_FORMATS, SHADER_BINARY_FORMATS
        case 0x8DFA:                                            // SHADER_COMPILER
        case 0x8B8B:                                            // FRAGMENT_SHADER_DERIVATIVE_HINT
        case 0x8192:                                            // GENERATE_MIPMAP_HINT
        case 0x0C01: case 0x0C02:                              // LINE_SMOOTH_HINT, READ_BUFFER (only the latter in ES)
        // ---- Pixel store ----
        case 0x0D05: case 0x0D02: case 0x0D03: case 0x0D04:    // PACK_*
        case 0x0CF5: case 0x0CF2: case 0x0CF3: case 0x0CF4:    // UNPACK_*
        case 0x806E: case 0x806D:                              // UNPACK_IMAGE_HEIGHT, UNPACK_SKIP_IMAGES
        // ---- Misc ----
        case 0x86A2: case 0x86A3:                              // NUM/COMPRESSED_TEXTURE_FORMATS
        case 0x8B9B: case 0x8B9A:                              // IMPLEMENTATION_COLOR_READ_FORMAT/TYPE
        case 0x0C90:                                            // HINT (?? not a pname, but in 1.20.1 probing)
        // ---- Depth/stencil funcs ----
        case 0x0B74: case 0x0B73:                              // DEPTH_FUNC, DEPTH_CLEAR_VALUE
        case 0x0B72:                                            // DEPTH_WRITEMASK
        case 0x0B91: case 0x0B92: case 0x0B93: case 0x0B94:    // STENCIL_CLEAR/FUNC/VALUE_MASK/FAIL
        case 0x0B95: case 0x0B96: case 0x0B97: case 0x0B98:    // STENCIL_PASS_DEPTH_FAIL/PASS/REF/WRITEMASK
        case 0x8CA3: case 0x8CA4: case 0x8CA5:                 // STENCIL_BACK_REF/VALUE_MASK/WRITEMASK
        case 0x8800: case 0x8801: case 0x8802: case 0x8803:    // STENCIL_BACK_FUNC/FAIL/PASS_DEPTH_FAIL/PASS_DEPTH_PASS
        // ---- Polygon offset ----
        case 0x8038: case 0x2A00:                              // POLYGON_OFFSET_FACTOR, POLYGON_OFFSET_UNITS
            return true;
        default:
            return false;
    }
}

// ---------------------------------------------------------------------------
// 兜底: 给定 pname, 返回 vendor 调用后可能需要的最小安全值
// 返回 0 表示: 该 pname 不需要兜底, 直接用 vendor 值
// ---------------------------------------------------------------------------
inline GLint safe_min_for_pname(GLenum pname) {
    switch (pname) {
        case pname::MAX_TEXTURE_SIZE:                  return kSafeMaxTextureSize;
        case pname::MAX_RENDERBUFFER_SIZE:             return kSafeMaxRenderbufferSize;
        case pname::MAX_VIEWPORT_DIMS:                 return kSafeMaxViewportDims;
        case pname::MAX_VERTEX_ATTRIBS:                return kSafeMaxVertexAttribs;
        case pname::MAX_VERTEX_UNIFORM_VECTORS:        return kSafeMaxVertexUniformVectors;
        case pname::MAX_VARYING_VECTORS:               return kSafeMaxVaryingVectors;
        case pname::MAX_FRAGMENT_UNIFORM_VECTORS:      return kSafeMaxFragmentUniformVectors;
        case pname::MAX_VERTEX_TEXTURE_IMAGE_UNITS:    return kSafeMaxVertexTextureImageUnits;
        case pname::MAX_COMBINED_TEXTURE_IMAGE_UNITS:  return kSafeMaxCombinedTextureImageUnits;
        case pname::MAX_TEXTURE_IMAGE_UNITS:           return kSafeMaxTextureImageUnits;
        case pname::MAX_TEXTURE_UNITS:                 return kSafeMaxTextureUnits;
        case pname::MAX_ELEMENTS_VERTICES:             return kSafeMaxElementsVertices;
        case pname::MAX_ELEMENTS_INDICES:              return kSafeMaxElementsIndices;
        case pname::RED_BITS:                          return kSafeColorBits;
        case pname::GREEN_BITS:                        return kSafeColorBits;
        case pname::BLUE_BITS:                         return kSafeColorBits;
        case pname::ALPHA_BITS:                        return kSafeAlphaBits;
        case pname::DEPTH_BITS:                        return kSafeDepthBits;
        case pname::STENCIL_BITS:                      return kSafeStencilBits;
        case pname::SUBPIXEL_BITS:                     return kSafeSubpixelBits;
        case pname::DOUBLEBUFFER:                      return kSafeDoubleBuffer;
        case pname::STEREO:                            return kSafeStereo;
        case pname::AUX_BUFFERS:                       return kSafeAuxBuffers;
        case pname::SAMPLE_BUFFERS:                    return kSafeSampleBuffers;
        case pname::SAMPLES:                           return kSafeSamples;
        default:                                       return 0;  // 不兜底
    }
}

// 是否需要为该 pname 提供兜底
inline bool needs_safe_fallback(GLenum pname) {
    return safe_min_for_pname(pname) > 0 || pname == pname::MAX_TEXTURE_SIZE;
}

// ---------------------------------------------------------------------------
// 兜底: 已知 GL_STRING 缺失时返回的安全值
// 返回 nullptr 表示: 该 string 不兜底, 直接用 vendor 值
// ---------------------------------------------------------------------------
inline const char* safe_string_for(GLenum name) {
    switch (name) {
        case pname::VENDOR:                   return "VMCR/Adreno";
        case pname::RENDERER:                 return "Adreno (TM) 735 VMCR";
        case pname::VERSION:                  return "OpenGL ES 3.2 V@0762.39 (VMCR)";
        case pname::SHADING_LANGUAGE_VERSION: return "OpenGL ES GLSL ES 3.20";
        default:                              return nullptr;
    }
}

}  // namespace vmcr::gl_safe
