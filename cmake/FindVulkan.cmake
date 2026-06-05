# ===========================================================================
# 简化版 Vulkan 查找
# 优先使用 NDK 自带的 vulkan_core.h (1.3.x)
# 主机侧: 尝试系统路径 (VULKAN_SDK / find_package)
# ===========================================================================

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    # ---- Android: 强制从 NDK 找 ------------------------------------------
    find_path(VULKAN_HEADER_DIR
        NAMES vulkan/vulkan.h
        HINTS
            ${ANDROID_NDK_HOME}/sources/third_party/vulkan/include
            ${CMAKE_SOURCE_DIR}/third_party/Vulkan-Headers/include
        DOC "Vulkan header location"
        REQUIRED)

    add_library(Vulkan::Headers INTERFACE IMPORTED)
    set_target_properties(Vulkan::Headers PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_HEADER_DIR}")
else()
    # ---- 主机侧: 尝试 find_package(Vulkan) -----------------------------
    find_package(Vulkan QUIET)
    if(Vulkan_FOUND)
        # Vulkan::Vulkan (lib) 与 Vulkan::Headers 都已自动创建
        message(STATUS "Vulkan SDK found: ${Vulkan_VERSION}")
    else()
        # ---- fallback: 手动找头文件 + 库 --------------------------------
        set(_vk_sdk_hints
            "$ENV{VULKAN_SDK}/include"
            "$ENV{VK_SDK_PATH}/include"
            "C:/VulkanSDK/1.4.350.0/Include"
            "C:/VulkanSDK/1.3.250.0/Include"
            "C:/VulkanSDK/1.3.290.0/Include"
            "/usr/include"
            "/usr/local/include"
            "/opt/vulkan-sdk/include")

        find_path(VULKAN_HEADER_DIR
            NAMES vulkan/vulkan.h
            HINTS ${_vk_sdk_hints}
            DOC "Vulkan header location")

        if(NOT VULKAN_HEADER_DIR)
            message(WARNING "Vulkan headers not found on host. "
                    "libvmcr_vulkan will not build. "
                    "Install Vulkan SDK or libvulkan-dev.")
            set(VULKAN_HEADER_DIR "NOTFOUND")
        else()
            add_library(Vulkan::Headers INTERFACE IMPORTED)
            set_target_properties(Vulkan::Headers PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_HEADER_DIR}")

            # 手动找 vulkan-1.lib
            find_library(VULKAN_LIB
                NAMES vulkan-1
                HINTS
                    "$ENV{VULKAN_SDK}/Lib"
                    "C:/VulkanSDK/1.4.350.0/Lib"
                    "/usr/lib"
                    "/usr/lib/x86_64-linux-gnu"
                DOC "Vulkan library location")
            if(VULKAN_LIB)
                add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
                set_target_properties(Vulkan::Vulkan PROPERTIES
                    IMPORTED_LOCATION "${VULKAN_LIB}")
                message(STATUS "Vulkan library: ${VULKAN_LIB}")
            else()
                message(WARNING "Vulkan library not found")
            endif()
        endif()
    endif()
endif()

message(STATUS "Vulkan headers: ${VULKAN_HEADER_DIR}")
