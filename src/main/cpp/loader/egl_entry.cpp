// ===========================================================================
// src/main/cpp/loader/egl_entry.cpp
//
// libEGL.so 入口 - 劫持 EGL 关键函数
// Phase 0/1: 全部转发到 vendor
// Phase 2: eglSwapBuffers / eglMakeCurrent 拦截
// ===========================================================================
#include "vmcr/log.h"
#include "vmcr/vendor_egl.h"
#include "vmcr/backend_router.h"
#include "vmcr/export.h"

#include <cstring>

extern "C" {

// =====================================================================
// EGL 入口 (按需劫持, 默认全部 forward 到 vendor)
// =====================================================================

VMCR_EXPORT EGLDisplay EGLAPIENTRY eglGetDisplay(void* native_display) {
    auto& e = vmcr::vendor::egl();
    if (e.eglGetDisplay) return e.eglGetDisplay(native_display);
    return nullptr;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay dpy, EGLint* major, EGLint* minor) {
    auto& e = vmcr::vendor::egl();
    if (e.eglInitialize) return e.eglInitialize(dpy, major, minor);
    return 0;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay dpy) {
    auto& e = vmcr::vendor::egl();
    if (e.eglTerminate) return e.eglTerminate(dpy);
    return 0;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglBindAPI(EGLenum api) {
    auto& e = vmcr::vendor::egl();
    if (e.eglBindAPI) return e.eglBindAPI(api);
    return 0;
}

VMCR_EXPORT EGLDisplay EGLAPIENTRY eglGetCurrentDisplay(void) {
    auto& e = vmcr::vendor::egl();
    if (e.eglGetCurrentDisplay) return e.eglGetCurrentDisplay();
    return nullptr;
}

VMCR_EXPORT EGLContext EGLAPIENTRY eglGetCurrentContext(void) {
    auto& e = vmcr::vendor::egl();
    if (e.eglGetCurrentContext) return e.eglGetCurrentContext();
    return nullptr;
}

VMCR_EXPORT EGLSurface EGLAPIENTRY eglGetCurrentSurface(EGLint which) {
    auto& e = vmcr::vendor::egl();
    if (e.eglGetCurrentSurface) return e.eglGetCurrentSurface(which);
    return nullptr;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay dpy, EGLSurface draw,
                                                  EGLSurface read, EGLContext ctx) {
    auto& e = vmcr::vendor::egl();
    EGLBoolean r = e.eglMakeCurrent ? e.eglMakeCurrent(dpy, draw, read, ctx) : 0;
    if (r) {
        // 当 MC 完成 eglMakeCurrent 时, 更新 backend router 的 surface
        auto& router = vmcr::BackendRouter::instance();
        if (auto* r = router.current()) {
            // TODO: 将 EGL surface 关联到 ANativeWindow (Phase 2)
        }
    }
    return r;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    auto& router = vmcr::BackendRouter::instance();
    auto& e = vmcr::vendor::egl();

    auto tier = router.tier();
    if (tier == vmcr::RendererTier::VulkanFull ||
        tier == vmcr::RendererTier::VulkanLimited) {
        // Phase 2: 触发 Vulkan 路径的 end_frame + present
        if (auto* r = router.current()) {
            r->end_frame();
            r->begin_frame();
        }
        return EGL_TRUE;
    }

    // GLES32 路径: 直接 forward
    return e.eglSwapBuffers ? e.eglSwapBuffers(dpy, surf) : 0;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglSwapInterval(EGLDisplay dpy, EGLint interval) {
    auto& e = vmcr::vendor::egl();
    if (e.eglSwapInterval) return e.eglSwapInterval(dpy, interval);
    return 0;
}

VMCR_EXPORT __eglMustCastToProperFunctionPointerType EGLAPIENTRY
eglGetProcAddress(const char* name) {
    auto& e = vmcr::vendor::egl();
    // vendor 返回 void*, EGL 规范要求函数指针. reinterpret_cast 转换
    void* p = e.eglGetProcAddress ? e.eglGetProcAddress(name) : nullptr;
    return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(p);
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay dpy, const EGLint* attrib_list,
                                                    EGLConfig* configs, EGLint config_size,
                                                    EGLint* num_config) {
    auto& e = vmcr::vendor::egl();
    if (e.eglChooseConfig) return e.eglChooseConfig(dpy, attrib_list, configs, config_size, num_config);
    return 0;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
                                                     EGLint attribute, EGLint* value) {
    auto& e = vmcr::vendor::egl();
    if (e.eglGetConfigAttrib) return e.eglGetConfigAttrib(dpy, config, attribute, value);
    return 0;
}

VMCR_EXPORT EGLSurface EGLAPIENTRY eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
                                                         void* native_window, const EGLint* attrib_list) {
    auto& e = vmcr::vendor::egl();
    EGLSurface s = e.eglCreateWindowSurface ?
                   e.eglCreateWindowSurface(dpy, config, native_window, attrib_list) : nullptr;
    if (s) {
        // TODO Phase 2: 将 native_window 传给 backend
    }
    return s;
}

VMCR_EXPORT EGLSurface EGLAPIENTRY eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config,
                                                            const EGLint* attrib_list) {
    auto& e = vmcr::vendor::egl();
    return e.eglCreatePbufferSurface ? e.eglCreatePbufferSurface(dpy, config, attrib_list) : nullptr;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay dpy, EGLSurface surf) {
    auto& e = vmcr::vendor::egl();
    return e.eglDestroySurface ? e.eglDestroySurface(dpy, surf) : 0;
}

VMCR_EXPORT EGLContext EGLAPIENTRY eglCreateContext(EGLDisplay dpy, EGLConfig config,
                                                      EGLContext share, const EGLint* attrib_list) {
    auto& e = vmcr::vendor::egl();
    return e.eglCreateContext ? e.eglCreateContext(dpy, config, share, attrib_list) : nullptr;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglDestroyContext(EGLDisplay dpy, EGLContext ctx) {
    auto& e = vmcr::vendor::egl();
    return e.eglDestroyContext ? e.eglDestroyContext(dpy, ctx) : 0;
}

VMCR_EXPORT EGLint EGLAPIENTRY eglGetError(void) {
    auto& e = vmcr::vendor::egl();
    return e.eglGetError ? e.eglGetError() : 0;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglQueryContext(EGLDisplay dpy, EGLContext ctx,
                                                    EGLint attribute, EGLint* value) {
    auto& e = vmcr::vendor::egl();
    return e.eglQueryContext ? e.eglQueryContext(dpy, ctx, attribute, value) : 0;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglQuerySurface(EGLDisplay dpy, EGLSurface surf,
                                                    EGLint attribute, EGLint* value) {
    auto& e = vmcr::vendor::egl();
    return e.eglQuerySurface ? e.eglQuerySurface(dpy, surf, attribute, value) : 0;
}

VMCR_EXPORT const char* EGLAPIENTRY eglQueryString(EGLDisplay dpy, EGLenum name) {
    auto& e = vmcr::vendor::egl();
    return e.eglQueryString ? e.eglQueryString(dpy, name) : nullptr;
}

VMCR_EXPORT EGLint EGLAPIENTRY eglWaitClient(void) {
    auto& e = vmcr::vendor::egl();
    return e.eglWaitClient ? e.eglWaitClient() : 0;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglWaitNative(EGLint engine) {
    auto& e = vmcr::vendor::egl();
    return e.eglWaitNative ? e.eglWaitNative(engine) : 0;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglReleaseThread(void) {
    auto& e = vmcr::vendor::egl();
    return e.eglReleaseThread ? e.eglReleaseThread() : 0;
}

}  // extern "C"
