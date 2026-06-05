// ===========================================================================
// src/main/cpp/loader/vmcr_init.cpp
// JNI_OnLoad: VMCR 启动入口. 当 libGL.so 被 MC 加载时调用.
// ===========================================================================
#include "vmcr/log.h"
#include "vmcr/backend.h"
#include "vmcr/backend_router.h"
#include "vmcr/vendor_gl.h"
#include "vmcr/vendor_egl.h"

#include <jni.h>
#include <android/log.h>
#include <sys/system_properties.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>

namespace {

constexpr const char* kTag = vmcr::log::kTagCore;

// 解析环境变量 VMCR_FORCE_TIER
vmcr::RendererTier parse_force_tier() noexcept {
    const char* env = std::getenv("VMCR_FORCE_TIER");
    if (!env) return vmcr::RendererTier::Invalid;
    if (std::strcmp(env, "VulkanFull")    == 0) return vmcr::RendererTier::VulkanFull;
    if (std::strcmp(env, "VulkanLimited") == 0) return vmcr::RendererTier::VulkanLimited;
    if (std::strcmp(env, "GLES32")        == 0) return vmcr::RendererTier::GLES32;
    return vmcr::RendererTier::Invalid;  // auto
}

// 解析 debug.vmcr.forced 属性
vmcr::RendererTier parse_prop_force_tier() noexcept {
    char value[PROP_VALUE_MAX] = {0};
    if (__system_property_get("debug.vmcr.forced", value) <= 0) {
        return vmcr::RendererTier::Invalid;
    }
    if (std::strcmp(value, "vk_full")     == 0) return vmcr::RendererTier::VulkanFull;
    if (std::strcmp(value, "vk_limited")  == 0) return vmcr::RendererTier::VulkanLimited;
    if (std::strcmp(value, "gles32")      == 0) return vmcr::RendererTier::GLES32;
    return vmcr::RendererTier::Invalid;
}

}  // namespace

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    LOG_I(kTag, "===========================================");
    LOG_I(kTag, "  VMCR libGL.so loading");
    LOG_I(kTag, "  Version: %d.%d.%d",
         VMCR_VERSION_MAJOR, VMCR_VERSION_MINOR, VMCR_VERSION_PATCH);
    LOG_I(kTag, "===========================================");

    // 1. 解析强制档位
    vmcr::RendererTier forced = parse_force_tier();
    vmcr::RendererTier prop_forced = parse_prop_force_tier();
    if (prop_forced != vmcr::RendererTier::Invalid) forced = prop_forced;
    if (forced != vmcr::RendererTier::Invalid) {
        LOG_I(kTag, "Force tier: %s", vmcr::tier_to_string(forced));
    } else {
        LOG_I(kTag, "Force tier: auto (will probe)");
    }

    // 2. 加载原厂 GLES + EGL 入口
    if (!vmcr::vendor::load_vendor()) {
        LOG_E(kTag, "Failed to load vendor GLES / EGL");
        return JNI_ERR;
    }

    // 3. 初始化 BackendRouter (auto 探测 + 加载后端)
    if (!vmcr::BackendRouter::instance().initialize(forced)) {
        LOG_W(kTag, "BackendRouter init failed, GLES forwarder will still work");
    }

    LOG_I(kTag, "  [BOOT] libGL.so loaded, tier=%s", vmcr::BackendRouter::instance().tier_name());
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* /*vm*/, void* /*reserved*/) {
    LOG_I(kTag, "VMCR unloading");
    vmcr::BackendRouter::instance().shutdown();
    vmcr::vendor::unload_vendor();
}
