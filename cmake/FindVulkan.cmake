# ===========================================================================
# 简化版 Vulkan 查找
# 优先使用 NDK 自带的 vulkan_core.h (1.3.x)
# ===========================================================================

find_path(VULKAN_HEADER_DIR
    NAMES vulkan/vulkan.h
    HINTS
        ${ANDROID_NDK_HOME}/sources/third_party/vulkan/include
        ${CMAKE_SOURCE_DIR}/third_party/Vulkan-Headers/include
    DOC "Vulkan header location"
    REQUIRED)

if(NOT VULKAN_HEADER_DIR)
    message(FATAL_ERROR "Vulkan headers not found")
endif()

# 校验版本
if(EXISTS "${VULKAN_HEADER_DIR}/vulkan/vulkan_core.h")
    file(STRINGS "${VULKAN_HEADER_DIR}/vulkan/vulkan_core.h"
         _vk_version_line
         REGEX "#define VK_API_VERSION_1_3 VK_MAKE_VERSION")
    message(STATUS "Found Vulkan headers in: ${VULKAN_HEADER_DIR}")
endif()

add_library(Vulkan::Headers INTERFACE IMPORTED)
set_target_properties(Vulkan::Headers PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_HEADER_DIR}")
