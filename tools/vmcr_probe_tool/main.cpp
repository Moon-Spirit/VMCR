// ===========================================================================
// tools/vmcr_probe_tool/main.cpp
//
// 独立探测工具: 不依赖 FCL/MC, 可在主机 / 设备上直接运行
// 用法:
//   vmcr_probe_tool                  (auto)
//   vmcr_probe_tool --verbose
//   vmcr_probe_tool --json
//   vmcr_probe_tool --force vk_full
// ===========================================================================
#include "vmcr/vk_probe.h"
#include "vmcr/log.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace {

void print_human(const vmcr::vk::ProbeResult& r, bool verbose) {
    const char* api = "Unknown";
    if (r.api_version > 0) {
        uint32_t major = (r.api_version >> 22) & 0x7f;
        uint32_t minor = (r.api_version >> 12) & 0x3ff;
        uint32_t patch = r.api_version & 0xfff;
        static char buf[32];
        std::snprintf(buf, sizeof(buf), "%u.%u.%u", major, minor, patch);
        api = buf;
    }
    std::printf("=== VMCR Vulkan Probe ===\n");
    std::printf("  device       : %s\n", r.device_name);
    std::printf("  vendor       : %s\n", r.vendor_name);
    std::printf("  api version  : %s\n", api);
    std::printf("  driver       : 0x%x\n", r.driver_version);
    std::printf("  score        : %u\n", r.score);
    std::printf("  tier         : %s\n", vmcr::tier_to_string(r.tier));
    if (verbose) {
        std::printf("  features 1.3 :\n");
        std::printf("    dynamic_rendering      = %d\n", (int)r.has_dynamic_rendering);
        std::printf("    timeline_semaphore     = %d\n", (int)r.has_timeline_semaphore);
        std::printf("    synchronization2       = %d\n", (int)r.has_synchronization2);
        std::printf("    maintenance4           = %d\n", (int)r.has_maintenance4);
        std::printf("    buffer_device_address  = %d\n", (int)r.has_buffer_device_address);
        std::printf("  extensions   :\n");
        std::printf("    push_descriptor        = %d\n", (int)r.has_push_descriptor);
        std::printf("    draw_indirect_count    = %d\n", (int)r.has_draw_indirect_count);
        std::printf("    descriptor_buffer      = %d\n", (int)r.has_descriptor_buffer);
        std::printf("    ahb_external_memory    = %d\n", (int)r.has_ahb_external_memory);
        std::printf("    external_memory_fd     = %d\n", (int)r.has_external_memory_fd);
        std::printf("    swapchain              = %d\n", (int)r.has_swapchain);
        std::printf("    create_renderpass2     = %d\n", (int)r.has_create_renderpass2);
        std::printf("  capabilities :\n");
        std::printf("    max_storage_buffer     = %llu MiB\n",
                    (unsigned long long)(r.max_storage_buffer_range / (1024 * 1024)));
    }
    std::printf("=========================\n");
}

void print_json(const vmcr::vk::ProbeResult& r) {
    std::printf("{\n");
    std::printf("  \"tier\": \"%s\",\n", vmcr::tier_to_string(r.tier));
    std::printf("  \"score\": %u,\n", r.score);
    std::printf("  \"device\": \"%s\",\n", r.device_name);
    std::printf("  \"vendor\": \"%s\",\n", r.vendor_name);
    std::printf("  \"api_version\": %u,\n", r.api_version);
    std::printf("  \"driver_version\": %u,\n", r.driver_version);
    std::printf("  \"features13\": {\n");
    std::printf("    \"dynamic_rendering\": %d,\n",      (int)r.has_dynamic_rendering);
    std::printf("    \"timeline_semaphore\": %d,\n",     (int)r.has_timeline_semaphore);
    std::printf("    \"synchronization2\": %d,\n",       (int)r.has_synchronization2);
    std::printf("    \"maintenance4\": %d,\n",           (int)r.has_maintenance4);
    std::printf("    \"buffer_device_address\": %d\n",   (int)r.has_buffer_device_address);
    std::printf("  },\n");
    std::printf("  \"extensions\": {\n");
    std::printf("    \"push_descriptor\": %d,\n",       (int)r.has_push_descriptor);
    std::printf("    \"draw_indirect_count\": %d,\n",   (int)r.has_draw_indirect_count);
    std::printf("    \"descriptor_buffer\": %d,\n",     (int)r.has_descriptor_buffer);
    std::printf("    \"ahb_external_memory\": %d,\n",   (int)r.has_ahb_external_memory);
    std::printf("    \"external_memory_fd\": %d,\n",    (int)r.has_external_memory_fd);
    std::printf("    \"swapchain\": %d,\n",             (int)r.has_swapchain);
    std::printf("    \"create_renderpass2\": %d\n",     (int)r.has_create_renderpass2);
    std::printf("  },\n");
    std::printf("  \"max_storage_buffer_range\": %llu\n",
                (unsigned long long)r.max_storage_buffer_range);
    std::printf("}\n");
}

}  // namespace

int main(int argc, char** argv) {
    bool verbose = false;
    bool json = false;
    bool help = false;
    vmcr::vk::ProbeOptions opt;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
            opt.verbose = true;
        } else if (std::strcmp(argv[i], "--json") == 0) {
            json = true;
        } else if (std::strcmp(argv[i], "--validation") == 0) {
            opt.enable_validation = true;
        } else if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            help = true;
        }
    }

    if (help) {
        std::printf("vmcr_probe_tool - VMCR Vulkan device probe\n");
        std::printf("Usage: vmcr_probe_tool [options]\n");
        std::printf("  -v, --verbose   verbose output\n");
        std::printf("  --json          output as JSON\n");
        std::printf("  --validation    enable Vulkan validation layer\n");
        return 0;
    }

    vmcr::vk::ProbeResult r = vmcr::vk::probe_tier(opt);

    if (json) {
        print_json(r);
    } else {
        print_human(r, verbose);
    }

    return r.tier == vmcr::RendererTier::Invalid ? 1 : 0;
}
