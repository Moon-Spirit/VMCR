# ===========================================================================
# src/main/cpp/core/CMakeLists.txt
# ===========================================================================
add_library(vmcr_core STATIC
    backend_common.cpp
    backend_factory.cpp
    backend_router.cpp)

target_compile_features(vmcr_core PUBLIC cxx_std_20)
target_include_directories(vmcr_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(vmcr_core PUBLIC log)
endif()
