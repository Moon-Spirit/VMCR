# EGL 1.5 实现 (v2)
# 入口: libEGL.so, 导出 25 个 egl* 符号
# Phase 0: 仅占位入口 (返回 SUCCESS), 句柄表就绪
# Phase 1+: 翻译到 Vulkan (VkSurfaceKHR, VkSwapchainKHR 等)

# Phase 0 文件: egl_internal.{h,cpp} (句柄表) + egl_api.cpp (25 个 egl*)
# Phase 1+ 增: egl_loader.cpp (Vulkan 初始化), egl_surface_vk.cpp (VkSurfaceKHR)
set(_vmcr_egl_src
    egl_internal.cpp
    egl_api.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_library(vmcr_egl SHARED ${_vmcr_egl_src})
    set_target_properties(vmcr_egl PROPERTIES
        OUTPUT_NAME "EGL"
        PREFIX "lib"
        C_VISIBILITY_PRESET   default
        CXX_VISIBILITY_PRESET default)
else()
    add_library(vmcr_egl STATIC ${_vmcr_egl_src})
endif()

target_compile_features(vmcr_egl PUBLIC cxx_std_20)
target_include_directories(vmcr_egl PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(vmcr_egl PUBLIC android log)
endif()
