cmake_minimum_required(VERSION 3.22.1)

project(vmcr
    VERSION 0.1.0
    DESCRIPTION "Vulkan Mixed Compatibility Renderer for FCL"
    LANGUAGES C CXX
)

# ===========================================================================
# 0. 全局约定
# ===========================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ===========================================================================
# 1. 编译选项
# ===========================================================================
include(${CMAKE_SOURCE_DIR}/cmake/Options.cmake)

# ===========================================================================
# 2. Vulkan 头文件 / Loader
# ===========================================================================
include(${CMAKE_SOURCE_DIR}/cmake/FindVulkan.cmake)

# ===========================================================================
# 3. 公共 include
# ===========================================================================
include_directories(${CMAKE_SOURCE_DIR}/include)

# ===========================================================================
# 4. 子模块
# ===========================================================================
if(VMCR_ENABLE_VULKAN OR VMCR_ENABLE_GLES)
    add_subdirectory(src/main/cpp/core)
endif()

if(VMCR_ENABLE_VULKAN)
    add_subdirectory(src/main/cpp/vulkan)
    add_subdirectory(tools/vmcr_probe_tool)
endif()

if(VMCR_ENABLE_GLES)
    add_subdirectory(src/main/cpp/gles)
endif()

add_subdirectory(src/main/cpp/loader)
add_subdirectory(src/main/cpp/platform)

if(VMCR_ENABLE_JNI)
    add_subdirectory(src/main/cpp/jni)
endif()

# ===========================================================================
# 5. 单元测试 (主机侧)
# ===========================================================================
if(VMCR_ENABLE_UNIT_TEST)
    enable_testing()
    add_subdirectory(tests/unit)
endif()

# ===========================================================================
# 6. 安装规则
# ===========================================================================
include(CMakePackageConfigHelpers)
install(TARGETS vmcr_loader
    RUNTIME DESTINATION .
    LIBRARY DESTINATION .)
install(DIRECTORY configs/ DESTINATION configs/)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/ DESTINATION scripts/)
