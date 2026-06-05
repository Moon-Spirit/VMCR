// ===========================================================================
// include/vmcr/vendor_egl.h - 原厂 EGL 入口 (dlsym)
// ===========================================================================
#pragma once

#include <cstdint>

// EGL 类型 (避免引入 EGL 头)
using EGLBoolean = uint32_t;
using EGLDisplay = void*;
using EGLContext = void*;
using EGLSurface = void*;
using EGLConfig  = void*;
using EGLint     = int32_t;
using EGLenum    = uint32_t;
using __eglMustCastToProperFunctionPointerType = void(*)();

// EGL 常量
#ifndef EGL_TRUE
#define EGL_TRUE  1
#endif
#ifndef EGL_FALSE
#define EGL_FALSE 0
#endif
#ifndef EGL_NONE
#define EGL_NONE  0
#endif

namespace vmcr::vendor {

struct EglFunctionTable {
    bool loaded = false;

    EGLBoolean (*eglInitialize)(EGLDisplay, EGLint*, EGLint*) = nullptr;
    EGLBoolean (*eglTerminate)(EGLDisplay) = nullptr;
    EGLBoolean (*eglBindAPI)(EGLenum) = nullptr;
    EGLDisplay (*eglGetDisplay)(void*) = nullptr;
    EGLDisplay (*eglGetCurrentDisplay)(void) = nullptr;
    EGLContext (*eglGetCurrentContext)(void) = nullptr;
    EGLSurface (*eglGetCurrentSurface)(EGLint) = nullptr;
    EGLBoolean (*eglMakeCurrent)(EGLDisplay, EGLSurface, EGLSurface, EGLContext) = nullptr;
    EGLBoolean (*eglSwapBuffers)(EGLDisplay, EGLSurface) = nullptr;
    EGLBoolean (*eglSwapInterval)(EGLDisplay, EGLint) = nullptr;
    void*      (*eglGetProcAddress)(const char*) = nullptr;
    EGLBoolean (*eglChooseConfig)(EGLDisplay, const EGLint*, EGLConfig*, EGLint, EGLint*) = nullptr;
    EGLBoolean (*eglGetConfigAttrib)(EGLDisplay, EGLConfig, EGLint, EGLint*) = nullptr;
    EGLSurface (*eglCreateWindowSurface)(EGLDisplay, EGLConfig, void*, const EGLint*) = nullptr;
    EGLSurface (*eglCreatePbufferSurface)(EGLDisplay, EGLConfig, const EGLint*) = nullptr;
    EGLBoolean (*eglDestroySurface)(EGLDisplay, EGLSurface) = nullptr;
    EGLContext (*eglCreateContext)(EGLDisplay, EGLConfig, EGLContext, const EGLint*) = nullptr;
    EGLBoolean (*eglDestroyContext)(EGLDisplay, EGLContext) = nullptr;
    EGLint     (*eglGetError)(void) = nullptr;
    EGLBoolean (*eglQueryContext)(EGLDisplay, EGLContext, EGLint, EGLint*) = nullptr;
    EGLBoolean (*eglQuerySurface)(EGLDisplay, EGLSurface, EGLint, EGLint*) = nullptr;
    const char* (*eglQueryString)(EGLDisplay, EGLenum) = nullptr;
    EGLint     (*eglWaitClient)(void) = nullptr;
    EGLBoolean (*eglWaitNative)(EGLint) = nullptr;
    EGLBoolean (*eglReleaseThread)(void) = nullptr;
    EGLBoolean (*eglWaitGL)(void) = nullptr;
    EGLBoolean (*eglWaitClientNOK)(void) = nullptr;
    void*     (*eglGetProcAddress_raw)(const char*) = nullptr;  // raw void*
};

EglFunctionTable& egl() noexcept;
bool load_egl() noexcept;
void unload_egl() noexcept;

}  // namespace vmcr::vendor
