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
// ===========================================================================
#pragma once

#include <android/log.h>
#include <cstdarg>
#include <cstdio>

namespace vmcr::log {

constexpr const char* kTagCore = "VMCR-Core";
constexpr const char* kTagVK   = "VMCR-VK";
constexpr const char* kTagGL   = "VMCR-GL";
constexpr const char* kTagJNI  = "VMCR-JNI";

#ifndef VMCR_LOG_LEVEL
#define VMCR_LOG_LEVEL 1
#endif

namespace detail {
inline void write(int prio, const char* tag, const char* fmt, ...) noexcept {
    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(prio, tag, fmt, ap);
    va_end(ap);
}
}  // namespace detail

}  // namespace vmcr::log

#if VMCR_LOG_LEVEL >= 3
#define LOG_V(tag, ...) ::vmcr::log::detail::write(ANDROID_LOG_VERBOSE, tag, __VA_ARGS__)
#else
#define LOG_V(tag, ...) ((void)0)
#endif

#if VMCR_LOG_LEVEL >= 2
#define LOG_D(tag, ...) ::vmcr::log::detail::write(ANDROID_LOG_DEBUG, tag, __VA_ARGS__)
#else
#define LOG_D(tag, ...) ((void)0)
#endif

#define LOG_I(tag, ...) ::vmcr::log::detail::write(ANDROID_LOG_INFO,  tag, __VA_ARGS__)
#define LOG_W(tag, ...) ::vmcr::log::detail::write(ANDROID_LOG_WARN,  tag, __VA_ARGS__)
#define LOG_E(tag, ...) ::vmcr::log::detail::write(ANDROID_LOG_ERROR, tag, __VA_ARGS__)
#define LOG_F(tag, ...) ::vmcr::log::detail::write(ANDROID_LOG_FATAL, tag, __VA_ARGS__)

// 断言 - Debug 抛异常, Release 编译为 if (!cond) return
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
