// ===========================================================================
// src/main/cpp/core/backend_common.cpp
// 通用辅助: tier 字符串转换
// ===========================================================================
#include "vmcr/backend.h"
#include "vmcr/log.h"

namespace vmcr {

const char* tier_to_string(RendererTier t) noexcept {
    switch (t) {
        case RendererTier::Invalid:        return "Invalid";
        case RendererTier::VulkanFull:     return "VulkanFull";
        case RendererTier::VulkanLimited:  return "VulkanLimited";
        case RendererTier::GLES32:         return "GLES32";
    }
    return "Unknown";
}

RendererTier string_to_tier(const char* s) noexcept {
    if (!s) return RendererTier::Invalid;
    if (std::strcmp(s, "VulkanFull")    == 0) return RendererTier::VulkanFull;
    if (std::strcmp(s, "VulkanLimited") == 0) return RendererTier::VulkanLimited;
    if (std::strcmp(s, "GLES32")        == 0) return RendererTier::GLES32;
    if (std::strcmp(s, "auto")          == 0) return RendererTier::Invalid;  // auto -> 探测
    return RendererTier::Invalid;
}

}  // namespace vmcr
