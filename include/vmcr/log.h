// ===========================================================================
// include/vmcr/log.h - 统一日志接口
//
// 日志 TAG:
//   VMCR-Core   - 核心路由 / 探测
//   VMCR-VK     - Vulkan 后端
//   VMCR-GL     - GLES 后端
//   VMCR-JNI    - JNI 桥
//
// 过滤示例:
//   adb logcat -s VMCR-Core:V VMCR-VK:V VMCR-GL:V VMCR-JNI:V
//
// 主机侧: 自动 fallback 到 fprintf(stderr, ...)
// ===========================================================================
#pragma once

#include <cstdarg>
#include <cstdio>

#ifdef __ANDROID__
#include <android/log.h>
#define VMCR_HAS_ANDROID_LOG 1
#else
#define VMCR_HAS_ANDROID_LOG 0
#endif

namespace vmcr::log {

constexpr const char* kTagCore = "VMCR-Core";
constexpr const char* kTagVK   = "VMCR-VK";
constexpr const char* kTagGL   = "VMCR-GL";
constexpr const char* kTagJNI  = "VMCR-JNI";

#ifndef VMCR_LOG_LEVEL
#define VMCR_LOG_LEVEL 1
#endif

namespace detail {

#if VMCR_HAS_ANDROID_LOG
inline void host_write(int prio, const char* tag, const char* fmt, ...) noexcept {
    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(prio, tag, fmt, ap);
    va_end(ap);
}
#else
inline void host_write(int prio, const char* tag, const char* fmt, ...) noexcept {
    static const char* levels = "VDIWEFF";
    char c = (prio >= 0 && prio < 7) ? levels[prio] : '?';
    va_list ap;
    va_start(ap, fmt);
    std::fprintf(stderr, "[%c/%s] ", c, tag);
    std::vfprintf(stderr, fmt, ap);
    std::fputc('\n', stderr);
    va_end(ap);
}
#endif

}  // namespace detail
}  // namespace vmcr::log

#if VMCR_HAS_ANDROID_LOG
#  define VMCR_LOG_IMPL(prio, tag, ...) \
       ::vmcr::log::detail::host_write((prio), (tag), __VA_ARGS__)
#  define VMCR_LOG_V_DISABLED 0
#  define VMCR_LOG_D_DISABLED 0
#else
#  define VMCR_LOG_IMPL(prio, tag, ...) \
       ::vmcr::log::detail::host_write((prio), (tag), __VA_ARGS__)
#  define VMCR_LOG_V_DISABLED 0
#  define VMCR_LOG_D_DISABLED 0
#endif

#if VMCR_LOG_LEVEL >= 3
#define LOG_V(tag, ...) VMCR_LOG_IMPL(0, tag, __VA_ARGS__)
#else
#define LOG_V(tag, ...) ((void)0)
#endif

#if VMCR_LOG_LEVEL >= 2
#define LOG_D(tag, ...) VMCR_LOG_IMPL(1, tag, __VA_ARGS__)
#else
#define LOG_D(tag, ...) ((void)0)
#endif

#define LOG_I(tag, ...) VMCR_LOG_IMPL(2, tag, __VA_ARGS__)
#define LOG_W(tag, ...) VMCR_LOG_IMPL(3, tag, __VA_ARGS__)
#define LOG_E(tag, ...) VMCR_LOG_IMPL(4, tag, __VA_ARGS__)
#define LOG_F(tag, ...) VMCR_LOG_IMPL(5, tag, __VA_ARGS__)

// 断言
#ifdef NDEBUG
#define VMCR_ASSERT(cond, msg) \
    do { if (!(cond)) { LOG_E(::vmcr::log::kTagCore, "ASSERT failed: %s (%s:%d) %s", #cond, __FILE__, __LINE__, msg); return; } } while (0)
#define VMCR_ASSERT_RET(cond, msg, ret) \
    do { if (!(cond)) { LOG_E(::vmcr::log::kTagCore, "ASSERT failed: %s (%s:%d) %s", #cond, __FILE__, __LINE__, msg); return ret; } } while (0)
#else
#include <cassert>
#define VMCR_ASSERT(cond, msg)       assert((cond) && (msg))
#define VMCR_ASSERT_RET(cond, msg, ret) assert((cond) && (msg))
#endif
