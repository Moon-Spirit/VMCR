// ===========================================================================
// src/main/cpp/gles/CMakeLists.txt
// ===========================================================================
add_library(vmcr_gles SHARED
    gles_renderer.cpp)

target_compile_features(vmcr_gles PUBLIC cxx_std_20)
target_link_libraries(vmcr_gles
    PRIVATE  vmcr_core
    PUBLIC   android log)

set_source_files_properties(gles_renderer.cpp PROPERTIES
    COMPILE_DEFINITIONS VMCR_EXPORTS=1)
