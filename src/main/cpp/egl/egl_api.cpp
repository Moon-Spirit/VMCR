// ===========================================================================
// src/main/cpp/egl/egl_api.cpp
// libEGL.so 入口: 25 个 EGL 1.5 符号
// Phase 0: 完整状态机可跑, 翻译到 Vulkan 在 Phase 1+
// ===========================================================================
#include "vmcr/log.h"
#include "vmcr/export.h"
#include "vmcr/egl_types.h"
#include "egl_internal.h"

namespace vmcr::egl {
namespace {
constexpr const char* kTag = "VMCR-EGL";
inline int ok() { EglRegistry::instance().clear_error(); return egl_err::SUCCESS; }
inline int err(int e) { EglRegistry::instance().set_error(e); return e; }
}  // namespace
}  // namespace vmcr::egl

using namespace vmcr::egl;

extern "C" {

VMCR_EXPORT EGLDisplay EGLAPIENTRY eglGetDisplay(void* native_display) {
    LOG_I(kTag, "eglGetDisplay(%p)", native_display);
    if (native_display == egl_default_display() || native_display == nullptr) {
        EglRegistry::instance().create_display(egl_default_display());
        return ok(), egl_default_display();
    }
    return err(egl_err::BAD_PARAMETER), egl_no_display();
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay dpy, EGLint* major, EGLint* minor) {
    auto* d = EglRegistry::instance().get_display(dpy);
    if (!d) return err(egl_err::BAD_DISPLAY), EGL_FALSE;
    if (d->initialized) {
        if (major) *major = d->major;
        if (minor) *minor = d->minor;
        return ok(), EGL_TRUE;
    }
    d->initialized = true;
    d->major = 1;
    d->minor = 5;
    if (major) *major = 1;
    if (minor) *minor = 5;
    LOG_I(kTag, "EGL %d.%d initialized (VMCR v2 Phase 0)", 1, 5);
    return ok(), EGL_TRUE;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay dpy) {
    auto* d = EglRegistry::instance().get_display(dpy);
    if (!d) return err(egl_err::BAD_DISPLAY), EGL_FALSE;
    d->initialized = false;
    return ok(), EGL_TRUE;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglBindAPI(EGLenum api) {
    if (api == 0x30A0 || api == 0x30A1 || api == 0x30A2) {
        return ok(), EGL_TRUE;
    }
    return err(egl_err::BAD_PARAMETER), EGL_FALSE;
}

VMCR_EXPORT EGLDisplay EGLAPIENTRY eglGetCurrentDisplay(void) {
    return egl_default_display();
}

VMCR_EXPORT EGLContext EGLAPIENTRY eglGetCurrentContext(void) {
    static EGLContext current = egl_no_context();
    return current;
}

VMCR_EXPORT EGLSurface EGLAPIENTRY eglGetCurrentSurface(EGLint which) {
    (void)which;
    return egl_no_surface();
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay dpy, EGLSurface draw,
                                                  EGLSurface read, EGLContext ctx) {
    if (dpy != egl_default_display() && dpy != egl_no_display()) {
        auto* d = EglRegistry::instance().get_display(dpy);
        if (!d) return err(egl_err::BAD_DISPLAY), EGL_FALSE;
        if (!d->initialized) return err(egl_err::NOT_INITIALIZED), EGL_FALSE;
    }
    if (draw != read) {
        if (draw != egl_no_surface() && read != egl_no_surface() && draw != read) {
            return err(egl_err::BAD_MATCH), EGL_FALSE;
        }
    }
    auto* c = EglRegistry::instance().get_context(ctx);
    if (ctx != egl_no_context() && !c) {
        return err(egl_err::BAD_CONTEXT), EGL_FALSE;
    }
    return ok(), EGL_TRUE;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    (void)dpy; (void)surf;
    return ok(), EGL_TRUE;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglSwapInterval(EGLDisplay dpy, EGLint interval) {
    (void)dpy; (void)interval;
    return ok(), EGL_TRUE;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay dpy, const EGLint* attrib_list,
                                                    EGLConfig* configs, EGLint config_size,
                                                    EGLint* num_config) {
    (void)dpy; (void)attrib_list;
    if (num_config) *num_config = 1;
    if (configs && config_size > 0) {
        static EGLConfig cfg_handle = (EGLConfig)(uintptr_t)0x1001;
        if (!EglRegistry::instance().get_config(cfg_handle)) {
            EglRegistry::instance().create_config(cfg_handle);
        }
        configs[0] = cfg_handle;
        return ok(), EGL_TRUE;
    }
    return ok(), EGL_TRUE;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
                                                     EGLint attribute, EGLint* value) {
    (void)dpy;
    auto* c = EglRegistry::instance().get_config(config);
    if (!c) return err(egl_err::BAD_CONFIG), EGL_FALSE;
    if (!value) return err(egl_err::BAD_PARAMETER), EGL_FALSE;
    switch (attribute) {
        case 0x3024: *value = c->red_bits; break;
        case 0x3025: *value = c->green_bits; break;
        case 0x3026: *value = c->blue_bits; break;
        case 0x3027: *value = c->alpha_bits; break;
        case 0x3028: *value = c->depth_bits; break;
        case 0x3029: *value = c->stencil_bits; break;
        case 0x3040: *value = c->renderable_type; break;
        case 0x3033: *value = c->surface_type; break;
        default: return err(egl_err::BAD_ATTRIBUTE), EGL_FALSE;
    }
    return ok(), EGL_TRUE;
}

VMCR_EXPORT EGLSurface EGLAPIENTRY eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
                                                         void* native_window, const EGLint* attrib_list) {
    (void)attrib_list;
    auto* d = EglRegistry::instance().get_display(dpy);
    if (!d) return err(egl_err::BAD_DISPLAY), egl_no_surface();
    if (!native_window) return err(egl_err::BAD_NATIVE_WINDOW), egl_no_surface();
    static EGLSurface surf_handle = (EGLSurface)(uintptr_t)0x2001;
    auto* s = EglRegistry::instance().create_surface(surf_handle, d);
    s->native_window = native_window;
    return ok(), surf_handle;
}

VMCR_EXPORT EGLSurface EGLAPIENTRY eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config,
                                                            const EGLint* attrib_list) {
    (void)attrib_list;
    auto* d = EglRegistry::instance().get_display(dpy);
    if (!d) return err(egl_err::BAD_DISPLAY), egl_no_surface();
    static EGLSurface surf_handle = (EGLSurface)(uintptr_t)0x2002;
    EglRegistry::instance().create_surface(surf_handle, d);
    return ok(), surf_handle;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay dpy, EGLSurface surf) {
    (void)dpy;
    if (surf == egl_no_surface()) return ok(), EGL_TRUE;
    EglRegistry::instance().destroy_surface(surf);
    return ok(), EGL_TRUE;
}

VMCR_EXPORT EGLContext EGLAPIENTRY eglCreateContext(EGLDisplay dpy, EGLConfig config,
                                                      EGLContext share, const EGLint* attrib_list) {
    (void)config; (void)share;
    auto* d = EglRegistry::instance().get_display(dpy);
    if (!d) return err(egl_err::BAD_DISPLAY), egl_no_context();
    int client_version = 3;
    if (attrib_list) {
        for (int i = 0; attrib_list[i] != 0x3038; i += 2) {
            if (attrib_list[i] == 0x3098) {
                client_version = attrib_list[i + 1];
            }
        }
    }
    static EGLContext ctx_handle = (EGLContext)(uintptr_t)0x3001;
    auto* c = EglRegistry::instance().create_context(ctx_handle, d);
    c->es_version = client_version;
    return ok(), ctx_handle;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglDestroyContext(EGLDisplay dpy, EGLContext ctx) {
    (void)dpy;
    if (ctx == egl_no_context()) return ok(), EGL_TRUE;
    EglRegistry::instance().destroy_context(ctx);
    return ok(), EGL_TRUE;
}

VMCR_EXPORT EGLint EGLAPIENTRY eglGetError(void) {
    return EglRegistry::instance().get_error();
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglQueryContext(EGLDisplay dpy, EGLContext ctx,
                                                    EGLint attribute, EGLint* value) {
    (void)dpy;
    auto* c = EglRegistry::instance().get_context(ctx);
    if (!c) return err(egl_err::BAD_CONTEXT), EGL_FALSE;
    if (!value) return err(egl_err::BAD_PARAMETER), EGL_FALSE;
    switch (attribute) {
        case 0x3098: *value = c->es_version; break;
        case 0x3086: *value = 0x3086; break;
        default: return err(egl_err::BAD_ATTRIBUTE), EGL_FALSE;
    }
    return ok(), EGL_TRUE;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglQuerySurface(EGLDisplay dpy, EGLSurface surf,
                                                    EGLint attribute, EGLint* value) {
    (void)dpy;
    auto* s = EglRegistry::instance().get_surface(surf);
    if (!s) return err(egl_err::BAD_SURFACE), EGL_FALSE;
    if (!value) return err(egl_err::BAD_PARAMETER), EGL_FALSE;
    switch (attribute) {
        case 0x3056: *value = s->width ? s->width : 1080; break;
        case 0x3057: *value = s->height ? s->height : 2400; break;
        case 0x3086: *value = 0x3086; break;
        default: return err(egl_err::BAD_ATTRIBUTE), EGL_FALSE;
    }
    return ok(), EGL_TRUE;
}

VMCR_EXPORT const char* EGLAPIENTRY eglQueryString(EGLDisplay dpy, EGLenum name) {
    static const char* vendor = "VMCR";
    static const char* version = "1.5 VMCR";
    static const char* apis = "OpenGL_ES";
    static const char* exts = "EGL_KHR_create_context EGL_KHR_surfaceless_context";
    auto* d = EglRegistry::instance().get_display(dpy);
    if (!d) { err(egl_err::BAD_DISPLAY); return nullptr; }
    switch (name) {
        case 0x3055: return vendor;
        case 0x3054: return version;
        case 0x3058: return apis;
        case 0x3057: return exts;
        default: err(egl_err::BAD_PARAMETER); return nullptr;
    }
}

VMCR_EXPORT EGLint EGLAPIENTRY eglWaitClient(void) {
    return ok(), EGL_TRUE;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglWaitNative(EGLint engine) {
    (void)engine;
    return ok(), EGL_TRUE;
}

VMCR_EXPORT EGLBoolean EGLAPIENTRY eglReleaseThread(void) {
    return ok(), EGL_TRUE;
}

VMCR_EXPORT __eglMustCastToProperFunctionPointerType EGLAPIENTRY
eglGetProcAddress(const char* name) {
    if (!name) {
        err(egl_err::BAD_PARAMETER);
        return nullptr;
    }
    (void)name;
    return nullptr;
}

}  // extern "C"
