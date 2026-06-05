set(_vmcr_vulkan_src
    vmcr_probe.cpp)

# 检测 Vulkan-Hpp 可用性 (子模块)
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/Vulkan-Hpp/vk_sdk/include/vulkan/vulkan.hpp")
    list(APPEND _vmcr_vulkan_src vk_renderer_hpp.cpp)
    message(STATUS "vmcr_vulkan: Vulkan-Hpp detected, building vk_renderer_hpp.cpp")
else()
    list(APPEND _vmcr_vulkan_src vk_renderer.cpp)
    message(STATUS "vmcr_vulkan: Vulkan-Hpp NOT found, using legacy vk_renderer.cpp")
endif()

# Android: SHARED (dlopen); 主机: STATIC (避免 .lib 缺失)
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_library(vmcr_vulkan SHARED ${_vmcr_vulkan_src})
else()
    add_library(vmcr_vulkan STATIC ${_vmcr_vulkan_src})
endif()

target_compile_features(vmcr_vulkan PUBLIC cxx_std_20)
target_link_libraries(vmcr_vulkan
    PRIVATE  vmcr_core
    PUBLIC   Vulkan::Headers)

# 链接 Vulkan loader
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(vmcr_vulkan PRIVATE vulkan)
else()
    if(TARGET Vulkan::Vulkan)
        target_link_libraries(vmcr_vulkan PRIVATE Vulkan::Vulkan)
    elseif(VULKAN_LIB)
        target_link_libraries(vmcr_vulkan PRIVATE "${VULKAN_LIB}")
    endif()
endif()

# Vulkan 平台宏 (Android 必须)
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_compile_definitions(vmcr_vulkan PRIVATE
        VK_USE_PLATFORM_ANDROID_KHR=1)
endif()

# Vulkan-Hpp include (子模块存在时)
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/Vulkan-Hpp/vk_sdk/include")
    target_include_directories(vmcr_vulkan PRIVATE
        "${CMAKE_SOURCE_DIR}/third_party/Vulkan-Hpp/vk_sdk/include")
endif()

# VMA include (子模块存在时)
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/VMA/include")
    target_include_directories(vmcr_vulkan PRIVATE
        "${CMAKE_SOURCE_DIR}/third_party/VMA/include")
endif()

# vkbootstrap include (子模块存在时)
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/vkbootstrap/include")
    target_include_directories(vmcr_vulkan PRIVATE
        "${CMAKE_SOURCE_DIR}/third_party/vkbootstrap/include")
endif()

# 仅在 Android 上链接 android log
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(vmcr_vulkan PUBLIC android log)
endif()
