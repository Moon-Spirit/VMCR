// ===========================================================================
// src/main/cpp/vulkan/CMakeLists.txt
// ===========================================================================
add_library(vmcr_vulkan SHARED
    vmcr_probe.cpp
    vk_renderer.cpp)

target_compile_features(vmcr_vulkan PUBLIC cxx_std_20)
target_link_libraries(vmcr_vulkan
    PRIVATE  vmcr_core
    PUBLIC   Vulkan::Headers android log)

# 隐藏内部符号
set_target_properties(vmcr_vulkan PROPERTIES
    C_VISIBILITY_PRESET   hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)

# 暴露入口符号
set_source_files_properties(vmcr_probe.cpp PROPERTIES
    COMPILE_DEFINITIONS VMCR_EXPORTS=1)
