# ===========================================================================
# src/main/cpp/vulkan/CMakeLists.txt
# ===========================================================================
set(_vmcr_vulkan_src
    vmcr_probe.cpp)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    list(APPEND _vmcr_vulkan_src vk_renderer.cpp)
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

# 仅在 Android 上链接 android log
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(vmcr_vulkan PUBLIC android log)
endif()

# 隐藏内部符号
set_target_properties(vmcr_vulkan PROPERTIES
    C_VISIBILITY_PRESET   hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)

# 暴露入口符号
set_source_files_properties(vmcr_probe.cpp PROPERTIES
    COMPILE_DEFINITIONS VMCR_EXPORTS=1)
