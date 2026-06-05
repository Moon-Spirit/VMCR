// ===========================================================================
// tests/unit/CMakeLists.txt
// 单元测试 (GTest, 仅主机侧)
// ===========================================================================
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(googletest)

add_executable(vmcr_unit_tests
    test_main.cpp
    test_backend.cpp
    test_vk_probe.cpp)

target_link_libraries(vmcr_unit_tests
    PRIVATE  vmcr_core
    GTest::gtest GTest::gtest_main)

if(VMCR_ENABLE_VULKAN)
    target_link_libraries(vmcr_unit_tests PRIVATE vmcr_vulkan)
endif()
if(VMCR_ENABLE_GLES)
    target_link_libraries(vmcr_unit_tests PRIVATE vmcr_gles)
endif()

include(GoogleTest)
gtest_discover_tests(vmcr_unit_tests)
