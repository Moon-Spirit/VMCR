// ===========================================================================
// tests/unit/test_backend.cpp
// ===========================================================================
#include <gtest/gtest.h>
#include "vmcr/backend.h"

using namespace vmcr;

TEST(TierString, RoundTrip) {
    EXPECT_STREQ(tier_to_string(RendererTier::Invalid),       "Invalid");
    EXPECT_STREQ(tier_to_string(RendererTier::VulkanFull),    "VulkanFull");
    EXPECT_STREQ(tier_to_string(RendererTier::VulkanLimited), "VulkanLimited");
    EXPECT_STREQ(tier_to_string(RendererTier::GLES32),        "GLES32");
}

TEST(TierParse, Valid) {
    EXPECT_EQ(string_to_tier("VulkanFull"),    RendererTier::VulkanFull);
    EXPECT_EQ(string_to_tier("VulkanLimited"), RendererTier::VulkanLimited);
    EXPECT_EQ(string_to_tier("GLES32"),        RendererTier::GLES32);
    EXPECT_EQ(string_to_tier("auto"),          RendererTier::Invalid);
}

TEST(TierParse, Invalid) {
    EXPECT_EQ(string_to_tier(nullptr),     RendererTier::Invalid);
    EXPECT_EQ(string_to_tier(""),          RendererTier::Invalid);
    EXPECT_EQ(string_to_tier("unknown"),   RendererTier::Invalid);
}
