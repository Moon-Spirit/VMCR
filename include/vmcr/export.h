// ===========================================================================
// include/vmcr/export.h - 跨平台符号导出宏
// ===========================================================================
#pragma once

#if defined(_WIN32)
#define VMCR_EXPORT __declspec(dllexport)
#else
#define VMCR_EXPORT __attribute__((visibility("default")))
#endif

// EGLAPIENTRY: EGL 规范要求, 在 Android / Linux 上为空, Windows 上是 __stdcall
#if defined(_WIN32) && !defined(EGLAPIENTRY)
#define EGLAPIENTRY __stdcall
#elif !defined(EGLAPIENTRY)
#define EGLAPIENTRY
#endif
