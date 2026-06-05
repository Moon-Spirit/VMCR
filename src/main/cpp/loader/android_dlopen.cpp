// ===========================================================================
// src/main/cpp/loader/android_dlopen.cpp
// dlopen 包装与依赖管理
// ===========================================================================
#include "vmcr/log.h"
#include <dlfcn.h>
#include <string>

namespace vmcr::loader {

void* safe_dlopen(const char* name, int flags) noexcept {
    void* h = ::dlopen(name, flags);
    if (!h) {
        LOG_W(vmcr::log::kTagCore, "dlopen(%s) failed: %s", name, ::dlerror());
    } else {
        LOG_D(vmcr::log::kTagCore, "dlopen(%s) = %p", name, h);
    }
    return h;
}

}  // namespace vmcr::loader
