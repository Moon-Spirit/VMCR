// ===========================================================================
// include/vmcr/dl.h - 平台无关的动态库加载
// Android:    dlopen/dlsym/dlclose (libc)
// Windows:    LoadLibraryA/GetProcAddress/FreeLibrary (kernel32)
// ===========================================================================
#pragma once

#include <cstdint>

#if defined(_WIN32) && !defined(__ANDROID__)
#include <windows.h>
#define VMCR_DL_HANDLE  HMODULE
#define VMCR_DL_OPEN(n) LoadLibraryA(n)
#define VMCR_DL_SYM(h, s) reinterpret_cast<void*>(GetProcAddress(h, s))
#define VMCR_DL_CLOSE(h) FreeLibrary(h)
#define VMCR_DL_ERROR()  ""
#else
#include <dlfcn.h>
#define VMCR_DL_HANDLE  void*
#define VMCR_DL_OPEN(n) ::dlopen(n, RTLD_NOW | RTLD_LOCAL)
#define VMCR_DL_SYM(h, s) ::dlsym(h, s)
#define VMCR_DL_CLOSE(h) ::dlclose(h)
#define VMCR_DL_ERROR()  ::dlerror()
#endif
