#!/usr/bin/env python3
# ---------------------------------------------------------------------------
# 解析 vulkaninfo JSON, 提取关键信息
# 用法: python3 scripts/parse_vkjson.py out/vulkaninfo_<device>.json
# ---------------------------------------------------------------------------
import json
import sys
from pathlib import Path


def score_from_features(features13: dict) -> int:
    s = 0
    weights = {
        "dynamicRendering": 2,
        "timelineSemaphore": 2,
        "synchronization2": 1,
        "maintenance4": 1,
        "bufferDeviceAddress": 1,
        "shaderIntegerDotProduct": 1,
        "subgroupSizeControl": 1,
        "separateDepthStencilLayouts": 1,
        "scalarBlockLayout": 1,
        "pipelineRobustness": 1,
    }
    for k, w in weights.items():
        if features13.get(k, False):
            s += w
    return s


def score_from_extensions(exts: list) -> int:
    s = 0
    wanted = [
        "VK_KHR_push_descriptor",
        "VK_KHR_draw_indirect_count",
        "VK_EXT_descriptor_buffer",
        "VK_ANDROID_external_memory_android_hardware_buffer",
        "VK_KHR_external_memory_fd",
        "VK_KHR_swapchain",
        "VK_KHR_create_renderpass2",
    ]
    have = {e["extensionName"] for e in exts}
    for w in wanted:
        if w in have:
            s += 1
    return s


def tier_for(score: int) -> str:
    if score >= 8:
        return "VulkanFull"
    if score >= 4:
        return "VulkanLimited"
    return "GLES32"


def main(argv: list) -> int:
    if len(argv) < 2:
        print("usage: parse_vkjson.py <vulkaninfo.json>", file=sys.stderr)
        return 1

    p = Path(argv[1])
    if not p.exists():
        print(f"file not found: {p}", file=sys.stderr)
        return 2

    data = json.loads(p.read_text())

    devs = data.get("devices", [])
    if not devs:
        print("no devices")
        return 3

    for d in devs:
        props = d.get("properties", {})
        feats13 = d.get("features", {}).get("VkPhysicalDeviceVulkan13Features", {})
        exts    = d.get("extensions", [])

        score13 = score_from_features(feats13)
        scoreExt = score_from_extensions(exts)
        total   = score13 + scoreExt

        api = props.get("apiVersion", 0)
        api_str = f"{(api >> 22) & 0x7f}.{(api >> 12) & 0x3ff}.{api & 0xfff}"

        print(f"=== {props.get('deviceName', '?')} ===")
        print(f"  apiVersion     : {api_str}")
        print(f"  vendorID       : {props.get('vendorID', 0):#x}")
        print(f"  driverVersion  : {props.get('driverVersion', 0):#x}")
        print(f"  features13     : {sum(1 for v in feats13.values() if v)}/{len(feats13)}")
        print(f"  extensions     : {len(exts)}")
        print(f"  score(13+ext)  : {score13} + {scoreExt} = {total}")
        print(f"  recommended    : {tier_for(total)}")
        print()

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
