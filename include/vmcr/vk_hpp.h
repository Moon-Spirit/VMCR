// ===========================================================================
// include/vmcr/vk_hpp.h - Vulkan-Hpp 头文件检测与封装
//
// 若 third_party/Vulkan-Hpp 存在, 引入 Vulkan-Hpp 头 (C++ 风格 RAII)
// 否则降级到原生 vulkan.h
//
// 使用示例:
//   #include "vmcr/vk_hpp.h"
//   vk::Instance instance = vk::raii::Context().createInstance(...);
// ===========================================================================
#pragma once

#include <vulkan/vulkan.h>

#ifdef VULKAN_HPP_INCLUDED
// Vulkan-Hpp 已包含, 直接使用
#else

// 检查子模块是否拉取
#if __has_include(<vulkan/vulkan.hpp>)
    #include <vulkan/vulkan.hpp>
    #ifndef VULKAN_HPP_INCLUDED
        #define VULKAN_HPP_INCLUDED
    #endif
#endif

#endif  // VULKAN_HPP_INCLUDED

#ifdef VULKAN_HPP_INCLUDED
    #define VMCR_HAS_VULKAN_HPP 1
    namespace vmcr::vk {
        using Instance = ::vk::Instance;
        using PhysicalDevice = ::vk::PhysicalDevice;
        using Device = ::vk::Device;
        using Queue = ::vk::Queue;
        using SurfaceKHR = ::vk::SurfaceKHR;
        using SwapchainKHR = ::vk::SwapchainKHR;
    }
#else
    #define VMCR_HAS_VULKAN_HPP 0
    // 降级到原生 Vulkan 句柄
    namespace vmcr::vk {
        using Instance = VkInstance;
        using PhysicalDevice = VkPhysicalDevice;
        using Device = VkDevice;
        using Queue = VkQueue;
        using SurfaceKHR = VkSurfaceKHR;
        using SwapchainKHR = VkSwapchainKHR;
    }
#endif

// 自动检测其他第三方库
#ifdef __has_include
    #if __has_include(<vk_mem_alloc.h>)
        #define VMCR_HAS_VMA 1
    #else
        #define VMCR_HAS_VMA 0
    #endif
    #if __has_include(<vkbootstrap/VkBootstrap.h>)
        #define VMCR_HAS_VKBOOTSTRAP 1
    #else
        #define VMCR_HAS_VKBOOTSTRAP 0
    #endif
    #if __has_include(<glm/glm.hpp>)
        #define VMCR_HAS_GLM 1
    #else
        #define VMCR_HAS_GLM 0
    #endif
    #if __has_include(<spdlog/spdlog.h>)
        #define VMCR_HAS_SPDLOG 1
    #else
        #define VMCR_HAS_SPDLOG 0
    #endif
#else
    #define VMCR_HAS_VMA 0
    #define VMCR_HAS_VKBOOTSTRAP 0
    #define VMCR_HAS_GLM 0
    #define VMCR_HAS_SPDLOG 0
#endif
