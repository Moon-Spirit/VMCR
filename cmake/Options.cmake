# ===========================================================================
# VMCR 全局编译开关
# ===========================================================================

# ---- 功能开关 ------------------------------------------------------------
option(VMCR_ENABLE_VULKAN          "Build Vulkan backend"                 ON)
option(VMCR_ENABLE_GLES            "Build GLES 3.2 fallback backend"      ON)
option(VMCR_ENABLE_JNI             "Build JNI bridge"                     ON)
option(VMCR_ENABLE_LOADER          "Build libGL.so / libEGL.so loader"    ON)
option(VMCR_ENABLE_UNIT_TEST       "Build host unit tests"                OFF)
option(VMCR_ENABLE_VERBOSE         "Verbose logging"                      OFF)
option(VMCR_ENABLE_ASAN            "AddressSanitizer"                     OFF)
option(VMCR_ENABLE_TSAN            "ThreadSanitizer"                      OFF)
option(VMCR_ENABLE_NEON            "ARM NEON intrinsics"                  ON)
option(VMCR_ENABLE_FP16            "FP16 shader paths"                    ON)
option(VMCR_ENABLE_SHADER_HOT_RELOAD "Hot reload SPIR-V"                  OFF)
option(VMCR_ENABLE_DESC_BUFFER     "Use VK_EXT_descriptor_buffer"         ON)
option(VMCR_ENABLE_DRAW_INDIRECT   "Use VK_KHR_draw_indirect_count"        ON)
option(VMCR_USE_VMA                "Use VulkanMemoryAllocator"            ON)
option(VMCR_ENABLE_PIPELINE_CACHE  "Use persistent pipeline cache"        ON)

# 主机侧构建时关闭 Android-only 依赖
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(VMCR_ENABLE_LOADER OFF CACHE BOOL "Build loader" FORCE)
    set(VMCR_ENABLE_JNI    OFF CACHE BOOL "Build JNI"    FORCE)
    set(VMCR_ENABLE_NEON   OFF CACHE BOOL "NEON"         FORCE)
endif()

# ---- 渲染档位 ------------------------------------------------------------
set(VMCR_RENDER_TIER "auto"
    CACHE STRING "Renderer tier: auto|VulkanFull|VulkanLimited|GLES32")

# ---- 日志等级 (0=err 1=warn 2=info 3=trace) ------------------------------
if(VMCR_ENABLE_VERBOSE)
    set(_VMCR_LOG_LEVEL 3)
else()
    set(_VMCR_LOG_LEVEL 1)
endif()

# ---- 平台宏 --------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_compile_definitions(VMCR_PLATFORM_ANDROID=1)
    add_compile_definitions(_GNU_SOURCE _REENTRANT)
endif()

add_compile_definitions(
    VMCR_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    VMCR_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    VMCR_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    VMCR_RENDER_TIER_${VMCR_RENDER_TIER}=1
    VMCR_LOG_LEVEL=${_VMCR_LOG_LEVEL}
)

# ---- 通用警告与优化 -------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(
        -Wall -Wextra
        -Wno-unused-parameter
        -Wno-deprecated-declarations
        -ffunction-sections -fdata-sections
        -fvisibility=hidden
        -fvisibility-inlines-hidden)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(
        /W4
        /wd4068           # unknown pragma
        /wd4244           # conversion
        /wd4267           # size_t conversion
        /wd4996           # deprecated
        /utf-8)
endif()

# Config-specific optimization (直接 set, 不依赖 generator expression)
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        add_compile_options(-O3)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/O2)
    endif()
    add_compile_definitions(NDEBUG)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        add_compile_options(-O0 -g3)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/Od /Zi)
    endif()
endif()

# ---- ARM 专属 ------------------------------------------------------------
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
    if(VMCR_ENABLE_NEON)
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
            add_compile_options(-mcpu=cortex-a75)
        else()
            add_compile_options(-mfpu=neon -mfloat-abi=softfp)
        endif()
    endif()
endif()

# ---- Sanitizers ----------------------------------------------------------
if(VMCR_ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(VMCR_ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
    add_link_options(-fsanitize=thread)
endif()
