# ===========================================================================
# src/main/cpp/loader/CMakeLists.txt
#
# 产物: libGL.so (�?FCL 当作 OpenGL ES 库加�?
# 暴露: glClear, glDrawElements, ... 全部 GLES 3.2 入口
#       JNI_OnLoad, JNI_OnUnload
# ===========================================================================
add_library(vmcr_loader SHARED
    gl_entry.cpp
    vmcr_init.cpp
    dlsym_vendor.cpp
    android_dlopen.cpp)

target_compile_features(vmcr_loader PUBLIC cxx_std_20)
target_include_directories(vmcr_loader PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(vmcr_loader
    PRIVATE  vmcr_core
    PUBLIC   android log)

# 关键: 输出名必须是 libGL.so
set_target_properties(vmcr_loader PROPERTIES
    OUTPUT_NAME "GL"
    PREFIX "lib"
    C_VISIBILITY_PRESET   default
    CXX_VISIBILITY_PRESET default)

# 导出 GL 符号
target_compile_definitions(vmcr_loader PRIVATE VMCR_EXPORTS_GLSYMS=1)
