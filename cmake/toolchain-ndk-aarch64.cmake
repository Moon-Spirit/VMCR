# ===========================================================================
# NDK Toolchain for Android (arm64-v8a)
# 兼容 NDK r25 ~ r27
# ===========================================================================

if(NOT DEFINED ANDROID_NDK_HOME)
    if(DEFINED ENV{ANDROID_NDK_HOME})
        set(ANDROID_NDK_HOME $ENV{ANDROID_NDK_HOME})
    else()
        message(FATAL_ERROR "ANDROID_NDK_HOME is not set")
    endif()
endif()

set(ANDROID_ABI            "arm64-v8a")
set(ANDROID_PLATFORM       "android-26")
set(ANDROID_STL            "c++_static")
set(ANDROID_ARM_MODE       "arm64-v8a")

set(_TOOLCHAIN_ROOT
    "${ANDROID_NDK_HOME}/toolchains/llvm/prebuilt/${CMAKE_HOST_SYSTEM_NAME}")

if(NOT EXISTS "${_TOOLCHAIN_ROOT}/bin/clang++")
    message(FATAL_ERROR
        "NDK toolchain not found at: ${_TOOLCHAIN_ROOT}\n"
        "Please verify ANDROID_NDK_HOME=${ANDROID_NDK_HOME}")
endif()

# --- 编译器 --------------------------------------------------------------
set(CMAKE_SYSTEM_NAME       Android)
set(CMAKE_SYSTEM_PROCESSOR  aarch64)
set(CMAKE_C_COMPILER        "${_TOOLCHAIN_ROOT}/bin/clang")
set(CMAKE_CXX_COMPILER      "${_TOOLCHAIN_ROOT}/bin/clang++")
set(CMAKE_AR                "${_TOOLCHAIN_ROOT}/bin/llvm-ar")
set(CMAKE_RANLIB            "${_TOOLCHAIN_ROOT}/bin/llvm-ranlib")
set(CMAKE_STRIP             "${_TOOLCHAIN_ROOT}/bin/llvm-strip")
set(CMAKE_LINKER            "${_TOOLCHAIN_ROOT}/bin/ld.lld")
set(CMAKE_NM                "${_TOOLCHAIN_ROOT}/bin/llvm-nm")
set(CMAKE_OBJCOPY           "${_TOOLCHAIN_ROOT}/bin/llvm-objcopy")
set(CMAKE_OBJDUMP           "${_TOOLCHAIN_ROOT}/bin/llvm-objdump")

# --- 查找 Root -----------------------------------------------------------
list(APPEND CMAKE_FIND_ROOT_PATH
    "${_TOOLCHAIN_ROOT}/sysroot"
    "${ANDROID_NDK_HOME}/sysroot")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# --- 平台宏 --------------------------------------------------------------
add_compile_definitions(
    VK_USE_PLATFORM_ANDROID_KHR=1
    _FORTIFY_SOURCE=2
    __ANDROID_API__=${ANDROID_PLATFORM}
    _GNU_SOURCE
    _REENTRANT)

# --- 链接器开关 ----------------------------------------------------------
add_link_options(
    "-Wl,--build-id=sha1"
    "-Wl,-z,noexecstack"
    "-Wl,-z,relro"
    "-Wl,-z,now"
    "-Wl,--gc-sections"
    "-Wl,--exclude-libs,ALL")

message(STATUS "VMCR NDK Toolchain: ${ANDROID_NDK_HOME}")
message(STATUS "  ABI=${ANDROID_ABI}  API=${ANDROID_PLATFORM}  STL=${ANDROID_STL}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER}")
