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
// 策略: 转发到 vendor, 之后对已知关键 pname 做最小值兜底, 防止 MC
//       把 0 误判为有效上限而崩溃. 对于 desktop-only 的 pname (LWJGL3
//       在初始化时探测), 也提供安全默认值, 消除 "1280: Invalid enum"
//       错误 (Pre startup / Startup).
//
// 设计原则:
//  - 不破坏 vendor 的真实返回值 (只在 vendor 返回 0 或错误时兜底)
//  - 优先信任 vendor 报告的值, 仅当小于已知安全下限时才覆盖
//  - 未知 pname 不做处理, 完全 forward, 避免引入新 bug
// ===========================================================================
#pragma once

#include "vmcr/vendor_gl.h"

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
