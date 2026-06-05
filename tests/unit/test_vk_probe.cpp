// ===========================================================================
// tests/unit/test_vk_probe.cpp
// 注意: 在没有 Vulkan 驱动的 CI 上, 此测试会跳过
// ===========================================================================
#include <gtest/gtest.h>
#include "vmcr/vk_probe.h"

using namespace vmcr::vk;

TEST(VkProbe, NoCrash) {
    ProbeResult r = probe_tier();
    // 任意环境, 探测不应崩溃
    EXPECT_NE(r.tier, RendererTier::Invalid);
    // 必然是三档之一
    EXPECT_TRUE(r.tier == RendererTier::VulkanFull ||
                r.tier == RendererTier::VulkanLimited ||
                r.tier == RendererTier::GLES32);
}

TEST(VkProbe, Sm8635) {
    if (std::getenv("VMCR_TEST_SM8635") == nullptr) {
        GTEST_SKIP() << "set VMCR_TEST_SM8635=1 to run";
    }
    ProbeResult r = probe_tier({.verbose = true});
    EXPECT_EQ(r.tier, RendererTier::VulkanFull);
    EXPECT_GE(r.score, 17u);
}
