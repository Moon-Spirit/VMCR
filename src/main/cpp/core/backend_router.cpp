// ===========================================================================
// src/main/cpp/core/backend_router.cpp
// ===========================================================================
#include "vmcr/backend_router.h"
#include "vmcr/log.h"

#include <cstring>

namespace vmcr {

BackendRouter& BackendRouter::instance() noexcept {
    static BackendRouter r;
    return r;
}

const char* BackendRouter::tier_name() const noexcept {
    return tier_to_string(tier_.load(std::memory_order_acquire));
}

bool BackendRouter::initialize(RendererTier force_tier) noexcept {
    std::lock_guard<std::mutex> lk(switch_mtx_);

    if (current_.load() != nullptr) {
        LOG_W(log::kTagCore, "BackendRouter already initialized");
        return true;
    }

    IRenderer* r = BackendFactory::create(force_tier);
    if (!r) {
        LOG_E(log::kTagCore, "Failed to create any backend");
        return false;
    }

    // 用一个 dummy params init, 真正的 params 在 eglMakeCurrent 时设置
    InitParams dummy{};
    if (!r->init(dummy)) {
        LOG_E(log::kTagCore, "Backend %s init() failed", r->name());
        delete r;
        return false;
    }

    current_.store(r, std::memory_order_release);
    tier_.store(r->tier(), std::memory_order_release);
    LOG_I(log::kTagCore, "BackendRouter initialized: %s (tier=%s)",
          r->name(), tier_to_string(r->tier()));
    return true;
}

void BackendRouter::set_backend(RendererPtr b) noexcept {
    std::lock_guard<std::mutex> lk(switch_mtx_);

    if (auto* old = current_.exchange(nullptr)) {
        old->destroy();
        delete old;
    }

    if (b) {
        current_.store(b.release(), std::memory_order_release);
        tier_.store(current_.load()->tier(), std::memory_order_release);
        LOG_I(log::kTagCore, "Backend switched to %s (tier=%s)",
              current_.load()->name(), tier_name());
    } else {
        tier_.store(RendererTier::Invalid, std::memory_order_release);
        LOG_W(log::kTagCore, "Backend cleared");
    }
}

void BackendRouter::shutdown() noexcept {
    set_backend(nullptr);
}

}  // namespace vmcr
