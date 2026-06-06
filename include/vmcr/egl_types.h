// ===========================================================================
// include/vmcr/egl_types.h
// EGL 类型定义 (与 EGL 1.5 spec 兼容, 但避免引入 <EGL/egl.h> 头)
// Phase 0 用
// ===========================================================================
#pragma once

#include <cstdint>

// EGL 标准类型
typedef std::int32_t   EGLint;
typedef std::int32_t   EGLBoolean;
typedef std::uint32_t  EGLenum;
typedef void*           EGLDisplay;
typedef void*           EGLContext;
typedef void*           EGLSurface;
typedef void*           EGLConfig;
typedef void*           EGLNativeDisplayType;
typedef void*           EGLNativeWindowType;
typedef void*           EGLNativePixmapType;

// EGL 函数指针类型
typedef void           (*__eglMustCastToProperFunctionPointerType)(void);

// EGL 常量
constexpr EGLBoolean EGL_FALSE = 0;
constexpr EGLBoolean EGL_TRUE  = 1;

// EGL 错误码
constexpr EGLint EGL_SUCCESS             = 0x3000;
constexpr EGLint EGL_NOT_INITIALIZED    = 0x3001;
constexpr EGLint EGL_BAD_ACCESS         = 0x3002;
constexpr EGLint EGL_BAD_ALLOC          = 0x3003;
constexpr EGLint EGL_BAD_ATTRIBUTE      = 0x3004;
constexpr EGLint EGL_BAD_CONFIG         = 0x3005;
constexpr EGLint EGL_BAD_CONTEXT        = 0x3006;
constexpr EGLint EGL_BAD_CURRENT_SURFACE = 0x3007;
constexpr EGLint EGL_BAD_DISPLAY        = 0x3008;
constexpr EGLint EGL_BAD_MATCH          = 0x3009;
constexpr EGLint EGL_BAD_NATIVE_PIXMAP  = 0x300A;
constexpr EGLint EGL_BAD_NATIVE_WINDOW  = 0x300B;
constexpr EGLint EGL_BAD_PARAMETER      = 0x300C;
constexpr EGLint EGL_BAD_SURFACE        = 0x300D;
constexpr EGLint EGL_CONTEXT_LOST       = 0x300E;

// EGL 属性
constexpr EGLint EGL_RED_SIZE          = 0x3024;
constexpr EGLint EGL_GREEN_SIZE        = 0x3025;
constexpr EGLint EGL_BLUE_SIZE         = 0x3026;
constexpr EGLint EGL_ALPHA_SIZE        = 0x3027;
constexpr EGLint EGL_DEPTH_SIZE        = 0x3028;
constexpr EGLint EGL_STENCIL_SIZE      = 0x3029;
constexpr EGLint EGL_WIDTH             = 0x3056;
constexpr EGLint EGL_HEIGHT            = 0x3057;
constexpr EGLint EGL_RENDER_BUFFER     = 0x3086;
constexpr EGLint EGL_VENDOR            = 0x3055;
constexpr EGLint EGL_VERSION           = 0x3054;
constexpr EGLint EGL_CLIENT_APIS       = 0x3058;
constexpr EGLint EGL_EXTENSIONS        = 0x3057;
constexpr EGLint EGL_SURFACE_TYPE      = 0x3033;
constexpr EGLint EGL_RENDERABLE_TYPE   = 0x3040;
constexpr EGLint EGL_OPENGL_ES3_BIT   = 0x0040;
constexpr EGLint EGL_WINDOW_BIT        = 0x0004;
constexpr EGLint EGL_NONE              = 0x3038;
constexpr EGLint EGL_CONTEXT_CLIENT_VERSION = 0x3098;
