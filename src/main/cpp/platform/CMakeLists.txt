// ===========================================================================
// src/main/cpp/platform/CMakeLists.txt
// ===========================================================================
add_library(vmcr_platform STATIC
    android_logger.cpp)

target_compile_features(vmcr_platform PUBLIC cxx_std_20)
target_link_libraries(vmcr_platform PUBLIC log)
