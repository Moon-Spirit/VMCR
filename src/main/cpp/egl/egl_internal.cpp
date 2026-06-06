// ===========================================================================
// src/main/cpp/egl/egl_internal.cpp
// Phase 0 句柄表实现
// ===========================================================================
#include "egl_internal.h"

namespace vmcr::egl {

EglRegistry& EglRegistry::instance() noexcept {
    static EglRegistry r;
    return r;
}

DisplayImpl* EglRegistry::create_display(EGLDisplay_t h) {
    std::lock_guard<std::mutex> lk(mu_);
    auto d = std::make_unique<DisplayImpl>();
    d->id = next_id_++;
    displays_[to_key<DisplayImpl>(h)] = std::move(d);
    return displays_[to_key<DisplayImpl>(h)].get();
}

DisplayImpl* EglRegistry::get_display(EGLDisplay_t h) {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = displays_.find(to_key<DisplayImpl>(h));
    return it != displays_.end() ? it->second.get() : nullptr;
}

void EglRegistry::destroy_display(EGLDisplay_t h) {
    std::lock_guard<std::mutex> lk(mu_);
    displays_.erase(to_key<DisplayImpl>(h));
}

ContextImpl* EglRegistry::create_context(EGLContext_t h, DisplayImpl* dpy) {
    std::lock_guard<std::mutex> lk(mu_);
    auto c = std::make_unique<ContextImpl>();
    c->id = next_id_++;
    c->display = dpy;
    contexts_[to_key<ContextImpl>(h)] = std::move(c);
    return contexts_[to_key<ContextImpl>(h)].get();
}

ContextImpl* EglRegistry::get_context(EGLContext_t h) {
    std::lock_guard<std::mutex> lk(mu_);
    if (!h) return nullptr;
    auto it = contexts_.find(to_key<ContextImpl>(h));
    return it != contexts_.end() ? it->second.get() : nullptr;
}

void EglRegistry::destroy_context(EGLContext_t h) {
    std::lock_guard<std::mutex> lk(mu_);
    contexts_.erase(to_key<ContextImpl>(h));
}

SurfaceImpl* EglRegistry::create_surface(EGLSurface_t h, DisplayImpl* dpy) {
    std::lock_guard<std::mutex> lk(mu_);
    auto s = std::make_unique<SurfaceImpl>();
    s->id = next_id_++;
    s->display = dpy;
    surfaces_[to_key<SurfaceImpl>(h)] = std::move(s);
    return surfaces_[to_key<SurfaceImpl>(h)].get();
}

SurfaceImpl* EglRegistry::get_surface(EGLSurface_t h) {
    std::lock_guard<std::mutex> lk(mu_);
    if (!h) return nullptr;
    auto it = surfaces_.find(to_key<SurfaceImpl>(h));
    return it != surfaces_.end() ? it->second.get() : nullptr;
}

void EglRegistry::destroy_surface(EGLSurface_t h) {
    std::lock_guard<std::mutex> lk(mu_);
    surfaces_.erase(to_key<SurfaceImpl>(h));
}

ConfigImpl* EglRegistry::create_config(EGLConfig_t h) {
    std::lock_guard<std::mutex> lk(mu_);
    auto c = std::make_unique<ConfigImpl>();
    c->id = next_id_++;
    configs_[to_key<ConfigImpl>(h)] = std::move(c);
    return configs_[to_key<ConfigImpl>(h)].get();
}

ConfigImpl* EglRegistry::get_config(EGLConfig_t h) {
    std::lock_guard<std::mutex> lk(mu_);
    if (!h) return nullptr;
    auto it = configs_.find(to_key<ConfigImpl>(h));
    return it != configs_.end() ? it->second.get() : nullptr;
}

void EglRegistry::set_error(int err) noexcept {
    std::lock_guard<std::mutex> lk(mu_);
    if (err != egl_err::SUCCESS) {
        last_error_ = err;
    }
}

int EglRegistry::get_error() noexcept {
    std::lock_guard<std::mutex> lk(mu_);
    int e = last_error_;
    last_error_ = egl_err::SUCCESS;
    return e;
}

void EglRegistry::clear_error() noexcept {
    std::lock_guard<std::mutex> lk(mu_);
    last_error_ = egl_err::SUCCESS;
}

}  // namespace vmcr::egl
