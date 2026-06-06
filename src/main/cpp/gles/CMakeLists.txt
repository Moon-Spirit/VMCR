# GLES 2.0/3.0/3.2 实现 (v2)
# 入口: libGL.so, 导出 130+ 个 gl* 符号
# Phase 0: 全部占位 (记录状态或 no-op)
# Phase 1+: 翻译到 Vulkan

# Phase 0 文件: gl_state.cpp + gles_entry.cpp (130 个 gl*)
# Phase 1+ 增: gles_buffer.cpp, gles_texture.cpp, gles_shader.cpp, gles_program.cpp,
#               gles_framebuffer.cpp, gles_draw.cpp, gles_clear.cpp, gles_state_set.cpp,
#               gles_query.cpp
set(_vmcr_gles_src
    gl_state.cpp
    gles_entry.cpp
    crash_handler.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_library(vmcr_gles SHARED ${_vmcr_gles_src})
    set_target_properties(vmcr_gles PROPERTIES
        OUTPUT_NAME "GL"
        PREFIX "lib"
        C_VISIBILITY_PRESET   default
        CXX_VISIBILITY_PRESET default)
else()
    add_library(vmcr_gles STATIC ${_vmcr_gles_src})
endif()

target_compile_features(vmcr_gles PUBLIC cxx_std_20)
target_include_directories(vmcr_gles PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(vmcr_gles PUBLIC android log)
endif()
