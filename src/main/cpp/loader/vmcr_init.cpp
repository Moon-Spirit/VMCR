// ===========================================================================
// src/main/cpp/loader/vmcr_init.cpp
// libGL.so 启动入口 (JNI_OnLoad + .init_array constructor)
//
// 架构诊断: FCL 的 MioLibPatcher 打了 org.lwjgl.system.Library 类,
// LWJGL3 的 dlsym(libGL.so, "glGetIntegerv") 被重定向到 libGLESv2.so
// (Adreno), 我们的 shim 被绕过.
//
// 验证方式:
//   1. .init_array constructor 立即写 stderr "VMCR libGL.so LOADED"
//   2. JNI_OnLoad 写 stderr "VMCR libGL.so JNI_OnLoad"
//   3. glGetIntegerv 入口加 atomic 计数器, 每次调用写 stderr
//   4. libGL.so 卸载时 dump 计数器
// 期望: 如果 [VMCR-GL] 行出现, shim 被调用; 反之 shim 被绕过.
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
#include <atomic>
#include <unistd.h>

namespace {

constexpr const char* kTag = vmcr::log::kTagCore;

// ---- 诊断计数器 ----------------------------------------------------------
// 写入 stderr 供 FCL 日志查看. atomic 保证多线程安全.
std::atomic<uint64_t> g_count_glGetIntegerv{0};
std::atomic<uint64_t> g_count_glGetString{0};
std::atomic<uint64_t> g_count_glGetStringi{0};
std::atomic<uint64_t> g_count_glActiveTexture{0};
std::atomic<uint64_t> g_count_libGL_loaded{0};

void dump_counters(const char* prefix) {
    std::fprintf(stderr,
        "[VMCR-GL] %s: glGetIntegerv=%llu glGetString=%llu "
        "glGetStringi=%llu glActiveTexture=%llu\n",
        prefix,
        (unsigned long long)g_count_glGetIntegerv.load(),
        (unsigned long long)g_count_glGetString.load(),
        (unsigned long long)g_count_glGetStringi.load(),
        (unsigned long long)g_count_glActiveTexture.load());
}

}  // namespace

// ---- 暴露给 gl_entry.cpp 的计数器接口 ------------------------------------
extern "C" {
    void vmcr_loader_bump_get_integerv() noexcept {
        g_count_glGetIntegerv.fetch_add(1, std::memory_order_relaxed);
    }
    void vmcr_loader_bump_get_string() noexcept {
        g_count_glGetString.fetch_add(1, std::memory_order_relaxed);
    }
    void vmcr_loader_bump_get_stringi() noexcept {
        g_count_glGetStringi.fetch_add(1, std::memory_order_relaxed);
    }
    void vmcr_loader_bump_active_texture() noexcept {
        g_count_glActiveTexture.fetch_add(1, std::memory_order_relaxed);
    }
}

namespace {

vmcr::RendererTier parse_force_tier() noexcept {
    const char* env = std::getenv("VMCR_FORCE_TIER");
    if (!env) return vmcr::RendererTier::Invalid;
    if (std::strcmp(env, "VulkanFull")    == 0) return vmcr::RendererTier::VulkanFull;
    if (std::strcmp(env, "VulkanLimited") == 0) return vmcr::RendererTier::VulkanLimited;
    if (std::strcmp(env, "GLES32")        == 0) return vmcr::RendererTier::GLES32;
    return vmcr::RendererTier::Invalid;
}

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

// ---- .init_array constructor ----------------------------------------------
// 一旦 libGL.so 被 dlopen, 立即执行. 哪怕 LWJGL3 不调用 JNI_OnLoad 路径
// (因为 MioLibPatcher 提前 patch 了 Library 类), 构造器也会跑.
extern "C" __attribute__((constructor))
void vmcr_loader_ctor() {
    g_count_libGL_loaded.fetch_add(1, std::memory_order_relaxed);
    std::fprintf(stderr,
        "[VMCR-GL] libGL.so CONSTRUCTOR (pid=%d, tid=%d) -- shim is now in this process\n",
        (int)getpid(), (int)gettid());
    std::fflush(stderr);
    LOG_I(kTag, "VMCR libGL.so constructor fired (pid=%d)", (int)getpid());
}

// ---- JNI_OnLoad: System.loadLibrary 路径 ---------------------------------
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    std::fprintf(stderr,
        "[VMCR-GL] libGL.so JNI_OnLoad (this is the System.load path)\n");
    std::fflush(stderr);

    LOG_I(kTag, "===========================================");
    LOG_I(kTag, "  VMCR libGL.so loading via JNI_OnLoad");
    LOG_I(kTag, "  Version: %d.%d.%d",
         VMCR_VERSION_MAJOR, VMCR_VERSION_MINOR, VMCR_VERSION_PATCH);
    LOG_I(kTag, "===========================================");

    vmcr::RendererTier forced = parse_force_tier();
    vmcr::RendererTier prop_forced = parse_prop_force_tier();
    if (prop_forced != vmcr::RendererTier::Invalid) forced = prop_forced;
    if (forced != vmcr::RendererTier::Invalid) {
        LOG_I(kTag, "Force tier: %s", vmcr::tier_to_string(forced));
    } else {
        LOG_I(kTag, "Force tier: auto (will probe)");
    }

    if (!vmcr::vendor::load_vendor()) {
        LOG_E(kTag, "Failed to load vendor GLES / EGL");
        return JNI_ERR;
    }

    if (!vmcr::BackendRouter::instance().initialize(forced)) {
        LOG_W(kTag, "BackendRouter init failed, GLES forwarder will still work");
    }

    LOG_I(kTag, "  [BOOT] libGL.so loaded, tier=%s", vmcr::BackendRouter::instance().tier_name());
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* /*vm*/, void* /*reserved*/) {
    dump_counters("JNI_OnUnload");
    LOG_I(kTag, "VMCR unloading");
    vmcr::BackendRouter::instance().shutdown();
    vmcr::vendor::unload_vendor();
}

// libGL.so 进程退出时再 dump 一次 (覆盖 dlclose 不调用的场景)
extern "C" __attribute__((destructor))
void vmcr_loader_dtor() {
    dump_counters("DESTRUCTOR (process exit)");
    std::fprintf(stderr,
        "[VMCR-GL] libGL.so was loaded %llu times during process lifetime\n",
        (unsigned long long)g_count_libGL_loaded.load());
    std::fflush(stderr);
}
