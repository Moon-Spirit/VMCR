// ===========================================================================
// src/main/cpp/vulkan/vmcr_probe.cpp
//
// 设备特性探测实现. 详见 ARCHITECTURE.md § 4.
// ===========================================================================
#include "vmcr/vk_probe.h"
#include "vmcr/log.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <vector>

namespace vmcr::vk {

namespace {

constexpr const char* kTag = log::kTagVK;

struct LoaderProcs {
    PFN_vkGetInstanceProcAddr GetInstanceProcAddr = nullptr;
    PFN_vkCreateInstance      CreateInstance       = nullptr;
    PFN_vkEnumerateInstanceLayerProperties EnumerateInstanceLayerProperties = nullptr;
    void*                     lib_handle = nullptr;
};

bool load_loader(LoaderProcs& p) noexcept {
    p.lib_handle = ::dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!p.lib_handle) {
        LOG_W(kTag, "libvulkan.so not found: %s", ::dlerror());
        return false;
    }
    p.GetInstanceProcAddr =
        reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            ::dlsym(p.lib_handle, "vkGetInstanceProcAddr"));
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
        .sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "VMCR",
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName      = "VMCR",
        .engineVersion    = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion       = api_ver,
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
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &app,
        .enabledLayerCount       = layer_count,
        .ppEnabledLayerNames     = layers.data(),
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

int score_device(VkInstance inst, VkPhysicalDevice dev) noexcept {
    int s = 0;

    // ---- 1.3 内核特性 ---------------------------------------------------
    VkPhysicalDeviceVulkan13Features f13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    };
    VkPhysicalDeviceFeatures2 f2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &f13,
    };
    auto pGF2 = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
        vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceFeatures2"));
    if (!pGF2) return 0;
    pGF2(dev, &f2);

    if (f13.dynamicRendering)            s += 2;
    if (f13.timelineSemaphore)           s += 2;
    if (f13.synchronization2)            s += 1;
    if (f13.maintenance4)                s += 1;
    if (f13.bufferDeviceAddress)         s += 1;
    if (f13.shaderIntegerDotProduct)     s += 1;
    if (f13.subgroupSizeControl)         s += 1;
    if (f13.separateDepthStencilLayouts) s += 1;
    if (f13.scalarBlockLayout)           s += 1;
    if (f13.pipelineRobustness)          s += 1;

    // ---- 关键扩展 --------------------------------------------------------
    auto pEep = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
        vkGetInstanceProcAddr(inst, "vkEnumerateDeviceExtensionProperties"));
    if (!pEep) return s;

    uint32_t ec = 0;
    pEep(dev, nullptr, &ec, nullptr);
    std::vector<VkExtensionProperties> exts(ec);
    pEep(dev, nullptr, &ec, exts.data());

    if (has_extension(exts, "VK_KHR_push_descriptor"))                          s += 1;
    if (has_extension(exts, "VK_KHR_draw_indirect_count"))                      s += 1;
    if (has_extension(exts, "VK_EXT_descriptor_buffer"))                        s += 1;
    if (has_extension(exts, "VK_ANDROID_external_memory_android_hardware_buffer")) s += 1;
    if (has_extension(exts, "VK_KHR_external_memory_fd"))                       s += 1;
    if (has_extension(exts, "VK_KHR_swapchain"))                                s += 1;
    if (has_extension(exts, "VK_KHR_create_renderpass2"))                       s += 1;

    return s;
}

void fill_result(VkInstance inst, VkPhysicalDevice dev, int score, ProbeResult& r) noexcept {
    auto pGP = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
        vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceProperties"));
    if (!pGP) return;
    VkPhysicalDeviceProperties props;
    pGP(dev, &props);
    r.api_version    = props.apiVersion;
    r.driver_version = props.driverVersion;
    r.score          = static_cast<uint32_t>(score);
    std::strncpy(r.vendor_name,  props.vendorName,  sizeof(r.vendor_name)  - 1);
    std::strncpy(r.device_name,  props.deviceName,  sizeof(r.device_name)  - 1);

    VkPhysicalDeviceVulkan13Features f13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    };
    VkPhysicalDeviceFeatures2 f2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &f13,
    };
    reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
        vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceFeatures2"))(dev, &f2);

    r.has_dynamic_rendering     = f13.dynamicRendering;
    r.has_timeline_semaphore    = f13.timelineSemaphore;
    r.has_synchronization2      = f13.synchronization2;
    r.has_maintenance4          = f13.maintenance4;
    r.has_buffer_device_address = f13.bufferDeviceAddress;

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

    // 能力
    VkPhysicalDeviceProperties p2;
    pGP(dev, &p2);
    r.max_memory_allocation    = p2.limits.maxMemoryAllocationCount * 0;  // 实际为 maxStorageBufferRange 等
    r.max_storage_buffer_range = p2.limits.maxStorageBufferRange;
    r.max_memory_allocation    = static_cast<uint64_t>(p2.limits.maxMemoryAllocationCount) * 1024ULL;  // 粗略
    // 真实 max alloc 来自 vkGetPhysicalDeviceMemoryProperties, 此处保守
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
            ::dlclose(lp.lib_handle);
            r.tier = RendererTier::GLES32;
            return r;
        }
    }

    auto pEpd = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        lp.GetInstanceProcAddr(inst, "vkEnumeratePhysicalDevices"));
    if (!pEpd) {
        vkDestroyInstance(inst, nullptr);
        ::dlclose(lp.lib_handle);
        r.tier = RendererTier::GLES32;
        return r;
    }

    uint32_t dc = 0;
    pEpd(inst, &dc, nullptr);
    if (dc == 0) {
        LOG_W(tag, "no Vulkan device");
        vkDestroyInstance(inst, nullptr);
        ::dlclose(lp.lib_handle);
        r.tier = RendererTier::GLES32;
        return r;
    }

    std::vector<VkPhysicalDevice> devs(dc);
    pEpd(inst, &dc, devs.data());

    int best = -1;
    VkPhysicalDevice best_dev = VK_NULL_HANDLE;
    for (auto& d : devs) {
        int s = score_device(inst, d);
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
        fill_result(inst, best_dev, best, r);
    }

    LOG_I(tag, "[PROBE] tier=%s score=%d api=0x%x vendor=%s device=%s",
          tier_to_string(r.tier), r.score, r.api_version,
          r.vendor_name, r.device_name);

    vkDestroyInstance(inst, nullptr);
    ::dlclose(lp.lib_handle);
    return r;
}

}  // namespace vmcr::vk
