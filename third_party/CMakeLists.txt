# ===========================================================================
# third_party/CMakeLists.txt - 第三方依赖统一管理
# 任何子模块不存在时, 优雅降级而不是 fatal
# ===========================================================================
cmake_minimum_required(VERSION 3.22.1)
project(vmcr_third_party LANGUAGES C CXX)

set(VMCR_THIRD_PARTY_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# ---- 工具: 检查目录是否非空 (有源码) ----------------------------
function(vmcr_third_party_check name path)
    if(EXISTS "${path}/CMakeLists.txt")
        set(${name}_FOUND TRUE PARENT_SCOPE)
    else()
        set(${name}_FOUND FALSE PARENT_SCOPE)
    endif()
endfunction()

# ---- 详细 add_subdirectory 包装 (子模块不存在时跳过) --------------
function(vmcr_add_submodule name path)
    vmcr_third_party_check(${name} "${path}")
    if(${name}_FOUND)
        message(STATUS "[third_party] ${name}: using ${path}")
        add_subdirectory(${path} ${name}_binary EXCLUDE_FROM_ALL)
        set(${name}_FOUND TRUE PARENT_SCOPE)
    else()
        message(WARNING "[third_party] ${name} not found at ${path}")
        message(WARNING "            run: git submodule update --init --recursive")
        set(${name}_FOUND FALSE PARENT_SCOPE)
    endif()
endfunction()

# ---- 默认选项 ----------------------------------------------------
option(VMCR_USE_VULKAN_HPP    "Use Vulkan-Hpp C++ bindings"    ON)
option(VMCR_USE_VMA          "Use Vulkan Memory Allocator"    ON)
option(VMCR_USE_VKBOOTSTRAP  "Use vk-bootstrap"               ON)
option(VMCR_USE_GLSLANG      "Use glslang (GLSL -> SPIR-V)"   ON)
option(VMCR_USE_SPIRV_CROSS  "Use SPIRV-Cross (reflection)"   ON)
option(VMCR_USE_GLM          "Use GLM math library"           ON)
option(VMCR_USE_SPDLOG       "Use spdlog logging"             ON)
option(VMCR_USE_DOBBY        "Use Dobby runtime hooking"      OFF)  # Phase 4+
