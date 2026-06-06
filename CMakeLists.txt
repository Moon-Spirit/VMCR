cmake_minimum_required(VERSION 3.22.1)

project(vmcr
    VERSION 0.2.0
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
# include(${CMAKE_SOURCE_DIR}/cmake/FindVulkan.cmake)  # Phase 0 暂不引

# ===========================================================================
# 3. 第三方依赖 (Phase 0: 暂不引入, Phase 1+ 引入 glslang + VMA)
# ===========================================================================
# if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/CMakeLists.txt")
#     add_subdirectory(third_party EXCLUDE_FROM_ALL)
# endif()

# ===========================================================================
# 4. 公共 include
# ===========================================================================
include_directories(${CMAKE_SOURCE_DIR}/include)

# ===========================================================================
# 5. 子模块
#   v2 路线: 自写 EGL + GLES (Phase 0 占位, Phase 1+ 真实 Vulkan 翻译)
#   - egl/    EGL 1.5 实现
#   - gles/   GLES 2.0+ 实现
#   - platform/ Android logger
#   - jni/    JNI bridge (可选, 默认 OFF)
#   - vulkan/ Vulkan 后端 (Phase 1+ 引入)
# ===========================================================================
if(VMCR_ENABLE_EGL)
    add_subdirectory(src/main/cpp/egl)
endif()

if(VMCR_ENABLE_GLES)
    add_subdirectory(src/main/cpp/gles)
endif()

add_subdirectory(src/main/cpp/platform)

if(VMCR_ENABLE_VULKAN)
    # Phase 1+ 引入
    # add_subdirectory(src/main/cpp/vulkan)
    message(STATUS "VMCR Phase 0: Vulkan backend not yet implemented, will add in Phase 1")
endif()

if(VMCR_ENABLE_JNI)
    add_subdirectory(src/main/cpp/jni)
endif()

# ===========================================================================
# 6. 单元测试 (主机侧)
# ===========================================================================
if(VMCR_ENABLE_UNIT_TEST)
    enable_testing()
    add_subdirectory(tests/unit)
endif()

# ===========================================================================
# 7. 安装规则
# ===========================================================================
install(DIRECTORY configs/ DESTINATION configs/)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/ DESTINATION scripts/)
