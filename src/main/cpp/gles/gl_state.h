// ===========================================================================
// src/main/cpp/gles/gl_state.h
// Phase 0 最小 GLES 状态
// ===========================================================================
#pragma once

#include <cstdint>
#include <array>

namespace vmcr::gles {

struct GlState {
    bool initialized = false;
    void* current_context = nullptr;
    std::array<int, 4> viewport = {0, 0, 0, 0};
    std::array<float, 4> clear_color = {0.0f, 0.0f, 0.0f, 0.0f};
    float clear_depth = 1.0f;
    int   clear_stencil = 0;
    int gl_error = 0;
    std::uint64_t draw_calls = 0;
    std::uint64_t texture_uploads = 0;
    std::uint64_t shader_compiles = 0;
    std::uint64_t frame_count = 0;
};

GlState& state() noexcept;
void set_error(int err) noexcept;
int  get_error() noexcept;

}  // namespace vmcr::gles
