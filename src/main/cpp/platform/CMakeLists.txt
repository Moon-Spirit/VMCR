# ===========================================================================
# src/main/cpp/platform/CMakeLists.txt
# ===========================================================================
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_library(vmcr_platform STATIC
        android_logger.cpp)
    target_link_libraries(vmcr_platform PUBLIC log)
else()
    add_library(vmcr_platform INTERFACE)  # �? 主机构建跳过
endif()