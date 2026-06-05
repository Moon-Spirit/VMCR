# ===========================================================================
# tools/vmcr_probe_tool/CMakeLists.txt
# 独立 Vulkan 探测工具 - 主机侧构�? 设备�?adb push 后运�?# ===========================================================================
add_executable(vmcr_probe_tool main.cpp)

target_link_libraries(vmcr_probe_tool PRIVATE vmcr_vulkan)

# 尝试查找系统 Vulkan (主机侧)
find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    target_link_libraries(vmcr_probe_tool PRIVATE Vulkan::Vulkan)
    message(STATUS "vmcr_probe_tool: linking against system Vulkan")
else()
    message(WARNING "vmcr_probe_tool: Vulkan not found on host, will fail at runtime")
endif()
