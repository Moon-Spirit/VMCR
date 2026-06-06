// ===========================================================================
// src/main/cpp/egl/egl_internal.h
// Phase 0 最小句柄表定义
// ===========================================================================
#pragma once

#include <cstdint>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "vmcr/egl_types.h"

namespace vmcr::egl {

// EGL 句柄: 用指针作 opaque handle
using EGLDisplay_t = void*;
using EGLContext_t = void*;
using EGLSurface_t = void*;
using EGLConfig_t  = void*;

inline EGLDisplay_t egl_default_display() { return (EGLDisplay_t)(uintptr_t)0x1; }
inline EGLDisplay_t egl_no_display()      { return nullptr; }
inline EGLContext_t egl_no_context()      { return nullptr; }
inline EGLSurface_t egl_no_surface()      { return nullptr; }

namespace egl_err {
    constexpr int SUCCESS             = 0x3000;
    constexpr int NOT_INITIALIZED    = 0x3001;
    constexpr int BAD_ACCESS         = 0x3002;
    constexpr int BAD_ALLOC          = 0x3003;
    constexpr int BAD_ATTRIBUTE      = 0x3004;
    constexpr int BAD_CONFIG         = 0x3005;
    constexpr int BAD_CONTEXT        = 0x3006;
    constexpr int BAD_CURRENT_SURFACE = 0x3007;
    constexpr int BAD_DISPLAY        = 0x3008;
    constexpr int BAD_MATCH          = 0x3009;
    constexpr int BAD_NATIVE_PIXMAP  = 0x300A;
    constexpr int BAD_NATIVE_WINDOW  = 0x300B;
    constexpr int BAD_PARAMETER      = 0x300C;
    constexpr int BAD_SURFACE        = 0x300D;
    constexpr int CONTEXT_LOST       = 0x300E;
}

// Phase 0 占位 Display / Context / Surface
struct DisplayImpl {
    std::uint32_t id = 0;
    bool initialized = false;
    int major = 0;
    int minor = 0;
};

struct ContextImpl {
    std::uint32_t id = 0;
    DisplayImpl* display = nullptr;
    int es_version = 3;
};

struct SurfaceImpl {
    std::uint32_t id = 0;
    DisplayImpl* display = nullptr;
    int width = 0;
    int height = 0;
    void* native_window = nullptr;
};

struct ConfigImpl {
    std::uint32_t id = 0;
    int red_bits = 8;
    int green_bits = 8;
    int blue_bits = 8;
    int alpha_bits = 8;
    int depth_bits = 24;
    int stencil_bits = 8;
    int renderable_type = 0x0040;  // EGL_OPENGL_ES3_BIT
    int surface_type = 0x0004;     // EGL_WINDOW_BIT
};

class EglRegistry {
public:
    static EglRegistry& instance() noexcept;

    DisplayImpl* create_display(EGLDisplay_t h);
    DisplayImpl* get_display(EGLDisplay_t h);
    void destroy_display(EGLDisplay_t h);

    ContextImpl* create_context(EGLContext_t h, DisplayImpl* dpy);
    ContextImpl* get_context(EGLContext_t h);
    void destroy_context(EGLContext_t h);

    SurfaceImpl* create_surface(EGLSurface_t h, DisplayImpl* dpy);
    SurfaceImpl* get_surface(EGLSurface_t h);
    void destroy_surface(EGLSurface_t h);

    ConfigImpl* create_config(EGLConfig_t h);
    ConfigImpl* get_config(EGLConfig_t h);

    void set_error(int err) noexcept;
    int  get_error() noexcept;
    void clear_error() noexcept;

private:
    EglRegistry() = default;
    std::mutex mu_;
    std::unordered_map<std::uintptr_t, std::unique_ptr<DisplayImpl>> displays_;
    std::unordered_map<std::uintptr_t, std::unique_ptr<ContextImpl>> contexts_;
    std::unordered_map<std::uintptr_t, std::unique_ptr<SurfaceImpl>> surfaces_;
    std::unordered_map<std::uintptr_t, std::unique_ptr<ConfigImpl>>  configs_;
    std::uint32_t next_id_ = 1;
    int last_error_ = egl_err::SUCCESS;

    template <typename T>
    std::uintptr_t to_key(void* h) const {
        return reinterpret_cast<std::uintptr_t>(h);
    }
};

}  // namespace vmcr::egl
