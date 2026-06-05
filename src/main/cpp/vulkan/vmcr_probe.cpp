// ===========================================================================
// src/main/cpp/vulkan/vmcr_probe.cpp
//
// 设备特性探测实现. 详见 ARCHITECTURE.md § 4.
//
// 兼容性:
//   - 1.3 内核特性: 通过 VkPhysicalDeviceVulkan13Features
//   - 1.4 扩展特性 (timelineSemaphore, bufferDeviceAddress 等):
//     在 1.4 SDK 中已从 1.3 features 移到 1.4 features
//   - 使用宏检测 API 版本
// ===========================================================================
#include "vmcr/vk_probe.h"
#include "vmcr/log.h"
#include "vmcr/dl.h"

#include <vulkan/vulkan.h>

#ifdef __ANDROID__
#include <vulkan/vulkan_android.h>
#endif

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// 检测 1.4 features struct 是否存在
#ifdef VK_API_VERSION_1_4
#define VMCR_HAS_VK14_FEATURES 1
#else
#define VMCR_HAS_VK14_FEATURES 0
#endif

namespace vmcr::vk {

namespace {

constexpr const char* kTag = log::kTagVK;

struct LoaderProcs {
    PFN_vkGetInstanceProcAddr GetInstanceProcAddr = nullptr;
    PFN_vkCreateInstance      CreateInstance       = nullptr;
    PFN_vkEnumerateInstanceLayerProperties EnumerateInstanceLayerProperties = nullptr;
    VMCR_DL_HANDLE            lib_handle = nullptr;
};

bool load_loader(LoaderProcs& p) noexcept {
    p.lib_handle = VMCR_DL_OPEN("libvulkan.so");
    if (!p.lib_handle) {
#ifdef _WIN32
        p.lib_handle = VMCR_DL_OPEN("vulkan-1.dll");
#endif
    }
    if (!p.lib_handle) {
        LOG_W(kTag, "libvulkan not found: %s", VMCR_DL_ERROR());
        return false;
    }
    p.GetInstanceProcAddr =
        reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            VMCR_DL_SYM(p.lib_handle, "vkGetInstanceProcAddr"));
    p.CreateInstance =
        reinterpret_cast<PFN_vkCreateInstance>(
            p.GetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance"));
    p.EnumerateInstanceLayerProperties =
        reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(
            p.GetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties"));

    if (!p.GetInstanceProcAddr || !p.CreateInstance) {
        LOG_E(kTag, "loader missing core procs");
        return false;
    }
    return true;
}

bool try_create_instance(LoaderProcs& p, VkInstance& out, uint32_t api_ver) noexcept {
    VkApplicationInfo app{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "VMCR",
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName = "VMCR",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion = api_ver,
    };

    std::array<const char*, 1> layers{};
    uint32_t layer_count = 0;
    if (p.EnumerateInstanceLayerProperties) {
        uint32_t lc = 0;
        p.EnumerateInstanceLayerProperties(&lc, nullptr);
        std::vector<VkLayerProperties> lp(lc);
        p.EnumerateInstanceLayerProperties(&lc, lp.data());
        for (auto& l : lp) {
            if (std::strcmp(l.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
                layers[layer_count++] = "VK_LAYER_KHRONOS_validation";
                break;
            }
        }
    }

    VkInstanceCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
        .enabledLayerCount = layer_count,
        .ppEnabledLayerNames = layers.data(),
    };

    VkResult r = p.CreateInstance(&ci, nullptr, &out);
    if (r != VK_SUCCESS) {
        LOG_W(kTag, "vkCreateInstance(0x%x) failed: %d", api_ver, r);
        return false;
    }
    return true;
}

bool has_extension(const std::vector<VkExtensionProperties>& exts, const char* name) noexcept {
    return std::any_of(exts.begin(), exts.end(),
        [name](const VkExtensionProperties& e) { return std::strcmp(e.extensionName, name) == 0; });
}

// 安全获取 feature 字段
struct FeatureBits {
    bool dynamic_rendering = false;
    bool timeline_semaphore = false;
    bool synchronization2 = false;
    bool maintenance4 = false;
    bool buffer_device_address = false;
    bool shader_integer_dot_product = false;
    bool subgroup_size_control = false;
    bool push_descriptor = false;
    bool pipeline_robustness = false;
    bool scalar_block_layout = false;
    bool separate_depth_stencil_layouts = false;
};

FeatureBits read_features(VkInstance inst, VkPhysicalDevice dev) noexcept {
    FeatureBits fb;

    auto pGF2 = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
        vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceFeatures2"));
    if (!pGF2) return fb;

    // ---- 1.2 features (timelineSemaphore, bufferDeviceAddress 等) -------
    VkPhysicalDeviceVulkan12Features f12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    };
    VkPhysicalDeviceFeatures2 f2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &f12,
    };
    pGF2(dev, &f2);

    fb.timeline_semaphore             = f12.timelineSemaphore != 0;
    fb.buffer_device_address          = f12.bufferDeviceAddress != 0;
    fb.scalar_block_layout            = f12.scalarBlockLayout != 0;
    fb.separate_depth_stencil_layouts = f12.separateDepthStencilLayouts != 0;

    // ---- 1.3 features ---------------------------------------------------
    VkPhysicalDeviceVulkan13Features f13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    };
    f12.pNext = &f13;
    pGF2(dev, &f2);

    fb.dynamic_rendering          = f13.dynamicRendering != 0;
    fb.synchronization2           = f13.synchronization2 != 0;
    fb.maintenance4               = f13.maintenance4 != 0;
    fb.shader_integer_dot_product = f13.shaderIntegerDotProduct != 0;
    fb.subgroup_size_control      = f13.subgroupSizeControl != 0;

    // ---- 1.4 features (pushDescriptor, pipelineRobustness) --------------
#if VMCR_HAS_VK14_FEATURES
    VkPhysicalDeviceVulkan14Features f14{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
    };
    f13.pNext = &f14;
    pGF2(dev, &f2);

    fb.push_descriptor      = f14.pushDescriptor != 0;
    fb.pipeline_robustness  = f14.pipelineRobustness != 0;
#endif

    return fb;
}

int score_features(const FeatureBits& fb) noexcept {
    int s = 0;
    if (fb.dynamic_rendering)         s += 2;
    if (fb.timeline_semaphore)        s += 2;
    if (fb.synchronization2)          s += 1;
    if (fb.maintenance4)              s += 1;
    if (fb.buffer_device_address)     s += 1;
    if (fb.shader_integer_dot_product) s += 1;
    if (fb.subgroup_size_control)     s += 1;
    if (fb.push_descriptor)           s += 1;
    if (fb.pipeline_robustness)       s += 1;
    if (fb.scalar_block_layout)       s += 1;
    if (fb.separate_depth_stencil_layouts) s += 1;
    return s;
}

int score_extensions(const std::vector<VkExtensionProperties>& exts) noexcept {
    int s = 0;
    if (has_extension(exts, "VK_KHR_push_descriptor"))                          s += 1;
    if (has_extension(exts, "VK_KHR_draw_indirect_count"))                      s += 1;
    if (has_extension(exts, "VK_EXT_descriptor_buffer"))                        s += 1;
    if (has_extension(exts, "VK_ANDROID_external_memory_android_hardware_buffer")) s += 1;
    if (has_extension(exts, "VK_KHR_external_memory_fd"))                       s += 1;
    if (has_extension(exts, "VK_KHR_swapchain"))                                s += 1;
    if (has_extension(exts, "VK_KHR_create_renderpass2"))                       s += 1;
    return s;
}

void fill_result(VkInstance inst, VkPhysicalDevice dev, int score,
                 const FeatureBits& fb, ProbeResult& r) noexcept {
    auto pGP = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
        vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceProperties"));
    if (!pGP) return;
    VkPhysicalDeviceProperties props;
    pGP(dev, &props);
    r.api_version    = props.apiVersion;
    r.driver_version = props.driverVersion;
    r.score          = static_cast<uint32_t>(score);
    std::strncpy(r.device_name,  props.deviceName,  sizeof(r.device_name)  - 1);

    // vendorID -> 字符串 (避免 SDK 1.4 移除 vendorName 字段的问题)
    static const char* vendor_names[] = {
        "VENDOR", "AMD", "ARM", "Imagination", "Intel",
        "NVIDIA", "Qualcomm", "Unknown"
    };
    uint32_t vid = props.vendorID;
    if (vid == 0x1002)      std::strncpy(r.vendor_name, vendor_names[1], 63);
    else if (vid == 0x13B5) std::strncpy(r.vendor_name, vendor_names[2], 63);
    else if (vid == 0x1010) std::strncpy(r.vendor_name, vendor_names[3], 63);
    else if (vid == 0x8086) std::strncpy(r.vendor_name, vendor_names[4], 63);
    else if (vid == 0x10DE) std::strncpy(r.vendor_name, vendor_names[5], 63);
    else if (vid == 0x5143) std::strncpy(r.vendor_name, vendor_names[6], 63);
    else                    std::snprintf(r.vendor_name, sizeof(r.vendor_name), "0x%04x", vid);

    r.has_dynamic_rendering     = fb.dynamic_rendering;
    r.has_timeline_semaphore    = fb.timeline_semaphore;
    r.has_synchronization2      = fb.synchronization2;
    r.has_maintenance4          = fb.maintenance4;
    r.has_buffer_device_address = fb.buffer_device_address;

    auto pEep = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
        vkGetInstanceProcAddr(inst, "vkEnumerateDeviceExtensionProperties"));
    if (pEep) {
        uint32_t ec = 0;
        pEep(dev, nullptr, &ec, nullptr);
        std::vector<VkExtensionProperties> exts(ec);
        pEep(dev, nullptr, &ec, exts.data());
        r.has_push_descriptor        = has_extension(exts, "VK_KHR_push_descriptor");
        r.has_draw_indirect_count    = has_extension(exts, "VK_KHR_draw_indirect_count");
        r.has_descriptor_buffer      = has_extension(exts, "VK_EXT_descriptor_buffer");
        r.has_ahb_external_memory    = has_extension(exts, "VK_ANDROID_external_memory_android_hardware_buffer");
        r.has_external_memory_fd     = has_extension(exts, "VK_KHR_external_memory_fd");
        r.has_swapchain              = has_extension(exts, "VK_KHR_swapchain");
        r.has_create_renderpass2     = has_extension(exts, "VK_KHR_create_renderpass2");
    }

    r.max_storage_buffer_range = props.limits.maxStorageBufferRange;
}

}  // namespace

ProbeResult probe_tier(const ProbeOptions& opt) noexcept {
    ProbeResult r;
    const char* tag = kTag;

    LoaderProcs lp;
    if (!load_loader(lp)) {
        r.tier = RendererTier::GLES32;
        return r;
    }

    VkInstance inst = VK_NULL_HANDLE;
    if (!try_create_instance(lp, inst, VK_API_VERSION_1_3)) {
        if (!try_create_instance(lp, inst, VK_API_VERSION_1_1)) {
            VMCR_DL_CLOSE(lp.lib_handle);
            r.tier = RendererTier::GLES32;
            return r;
        }
    }

    auto pEpd = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        lp.GetInstanceProcAddr(inst, "vkEnumeratePhysicalDevices"));
    if (!pEpd) {
        vkDestroyInstance(inst, nullptr);
        VMCR_DL_CLOSE(lp.lib_handle);
        r.tier = RendererTier::GLES32;
        return r;
    }

    uint32_t dc = 0;
    pEpd(inst, &dc, nullptr);
    if (dc == 0) {
        LOG_W(tag, "no Vulkan device");
        vkDestroyInstance(inst, nullptr);
        VMCR_DL_CLOSE(lp.lib_handle);
        r.tier = RendererTier::GLES32;
        return r;
    }

    std::vector<VkPhysicalDevice> devs(dc);
    pEpd(inst, &dc, devs.data());

    int best = -1;
    VkPhysicalDevice best_dev = VK_NULL_HANDLE;
    for (auto& d : devs) {
        FeatureBits fb = read_features(inst, d);
        auto pEep = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
            vkGetInstanceProcAddr(inst, "vkEnumerateDeviceExtensionProperties"));
        uint32_t ec = 0;
        pEep(d, nullptr, &ec, nullptr);
        std::vector<VkExtensionProperties> exts(ec);
        pEep(d, nullptr, &ec, exts.data());
        int s = score_features(fb) + score_extensions(exts);

        if (opt.verbose) {
            VkPhysicalDeviceProperties p;
            reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
                lp.GetInstanceProcAddr(inst, "vkGetPhysicalDeviceProperties"))(d, &p);
            LOG_I(tag, "  Device: %s api=0x%x score=%d", p.deviceName, p.apiVersion, s);
        }
        if (s > best) {
            best = s;
            best_dev = d;
        }
    }

    if (best < 0) best = 0;

    if (best >= 8) {
        r.tier = RendererTier::VulkanFull;
    } else if (best >= 4) {
        r.tier = RendererTier::VulkanLimited;
    } else {
        r.tier = RendererTier::GLES32;
    }

    if (best_dev != VK_NULL_HANDLE) {
        FeatureBits fb = read_features(inst, best_dev);
        fill_result(inst, best_dev, best, fb, r);
    }

    LOG_I(kTag, "[PROBE] tier=%s score=%d api=0x%x vendor=%s device=%s",
          tier_to_string(r.tier), r.score, r.api_version,
          r.vendor_name, r.device_name);

    vkDestroyInstance(inst, nullptr);
    VMCR_DL_CLOSE(lp.lib_handle);
    return r;
}

}  // namespace vmcr::vk
