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
#include <cstdio>
#include <string>
#include <dlfcn.h>

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
    if (!name) return nullptr;

    // ---- 关键: 优先返回我们自己 libGL.so 的函数指针 ----
    // FCL/Pojav 的 EGL 桥通过 eglGetProcAddress 获取 GL 函数, 拿到的是
    // vendor (Adreno) 的真实函数指针, 我们的 shim 被绕过.
    // 这里强制先从我们的 libGL.so 解析, 命中即返回 shim 函数.
    //
    // 因为我们的 libGL.so 没有 DT_SONAME, dlopen("libGL.so") 找不到.
    // 通过 dladdr 拿到 libEGL.so 的全路径, 把 "libEGL.so" 替换成 "libGL.so"
    // 即得到 libGL.so 的全路径. FCL 同一个 APK 把两个库放同目录.
    static void* s_our_libGL = nullptr;
    if (!s_our_libGL) {
        Dl_info info{};
        // 用本函数地址反查 libEGL.so 路径
        if (::dladdr(reinterpret_cast<const void*>(&eglGetProcAddress), &info)
            && info.dli_fname) {
            std::string path(info.dli_fname);
            const std::string from = "/libEGL.so";
            const std::string to   = "/libGL.so";
            auto pos = path.rfind(from);
            if (pos != std::string::npos) {
                path.replace(pos, from.size(), to);
                s_our_libGL = ::dlopen(path.c_str(), RTLD_NOW | RTLD_NOLOAD);
                if (s_our_libGL) {
                    std::fprintf(stderr,
                        "[VMCR-GL] eglGetProcAddress: locked our libGL.so at %s\n",
                        path.c_str());
                    std::fflush(stderr);
                    LOG_I(vmcr::log::kTagGL,
                          "eglGetProcAddress: locked our libGL.so at %s", path.c_str());
                } else {
                    std::fprintf(stderr,
                        "[VMCR-GL] eglGetProcAddress: dlopen(%s) failed: %s\n",
                        path.c_str(), ::dlerror());
                }
            }
        }
        if (!s_our_libGL) {
            LOG_W(vmcr::log::kTagGL, "eglGetProcAddress: could not find our libGL.so, falling back to vendor");
        }
    }
    if (s_our_libGL) {
        void* ours = ::dlsym(s_our_libGL, name);
        if (ours) {
            return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(ours);
        }
    }

    // ---- vendor (Adreno) 路径 ----
    auto& e = vmcr::vendor::egl();
    void* p = e.eglGetProcAddress ? e.eglGetProcAddress(name) : nullptr;
    if (p) return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(p);

    // 兜底: 试 libGLESv2.so
    if (auto* handle = ::dlopen("libGLESv2.so", RTLD_NOW | RTLD_NOLOAD)) {
        p = ::dlsym(handle, name);
        ::dlclose(handle);
    }
    if (!p) {
        if (auto* handle = ::dlopen("libEGL.so", RTLD_NOW | RTLD_NOLOAD)) {
            p = ::dlsym(handle, name);
            ::dlclose(handle);
        }
    }
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
