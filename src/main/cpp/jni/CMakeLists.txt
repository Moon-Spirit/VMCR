# ===========================================================================
# src/main/cpp/jni/CMakeLists.txt
# JNI �?- Phase 3 实现完整功能
# ===========================================================================
add_library(vmcr_jni SHARED
    jni_bridge.cpp
    jni_chunk_stream.cpp)

target_compile_features(vmcr_jni PUBLIC cxx_std_20)
target_link_libraries(vmcr_jni
    PRIVATE  vmcr_core
    PUBLIC   android log)

# JNI 符号必须 default 可见
set_target_properties(vmcr_jni PROPERTIES
    C_VISIBILITY_PRESET   default
    CXX_VISIBILITY_PRESET default)
