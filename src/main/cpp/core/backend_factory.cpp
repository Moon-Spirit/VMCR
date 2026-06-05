// ===========================================================================
// src/main/cpp/core/backend_factory.cpp
//
// 通过 dlopen 加载后端 .so. 当前实现:
//   1) 先尝试 dlopen("libvmcr_vk.so"), 找到 vmcr_vk_create 符号
//   2) 再尝试 dlopen("libvmcr_gles.so"), 找到 vmcr_gles_create 符号
//   3) 失败则返回 nullptr
// ===========================================================================
#include "vmcr/backend.h"
#include "vmcr/log.h"

#include <dlfcn.h>
#include <cstring>
#include <mutex>
#include <vector>

namespace vmcr {

namespace {

struct StaticEntry {
    const char*  so_name;
    RendererTier tier;
    RendererPtr  (*create)();
};

std::vector<StaticEntry>& registry() {
    static std::vector<StaticEntry> r;
    return r;
}

std::mutex g_create_mtx;

}  // namespace

void BackendFactory::register_static(const char* so_name,
                                      RendererTier tier,
                                      RendererPtr (*create)()) noexcept {
    registry().push_back({ so_name, tier, create });
}

IRenderer* BackendFactory::create(RendererTier force_tier) noexcept {
    std::lock_guard<std::mutex> lk(g_create_mtx);

    const char* tag = log::kTagCore;

    for (const auto& e : registry()) {
        if (force_tier != RendererTier::Invalid && e.tier != force_tier) {
            continue;
        }

        LOG_I(tag, "Trying backend: %s (tier=%s)",
              e.so_name, tier_to_string(e.tier));

        void* handle = ::dlopen(e.so_name, RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            LOG_W(tag, "  dlopen(%s) failed: %s", e.so_name, ::dlerror());
            continue;
        }

        using CreateFn = RendererPtr (*)();
        auto fn = reinterpret_cast<CreateFn>(::dlsym(handle, "vmcr_renderer_create"));
        if (!fn) {
            LOG_W(tag, "  dlsym(vmcr_renderer_create) failed in %s: %s",
                  e.so_name, ::dlerror());
            ::dlclose(handle);
            continue;
        }

        RendererPtr r;
        try {
            r = fn();
        } catch (const std::exception& ex) {
            LOG_E(tag, "  backend create threw: %s", ex.what());
            ::dlclose(handle);
            continue;
        }

        if (!r) {
            LOG_W(tag, "  backend create returned nullptr");
            ::dlclose(handle);
            continue;
        }

        LOG_I(tag, "Loaded backend: %s -> %s",
              e.so_name, r->name());
        return r.release();
    }

    LOG_E(tag, "No backend available (force_tier=%s)",
          tier_to_string(force_tier));
    return nullptr;
}

}  // namespace vmcr
