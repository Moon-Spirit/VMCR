// ===========================================================================
// src/main/cpp/gles/gl_state.cpp
// ===========================================================================
#include "vmcr/log.h"
#include "gl_state.h"

namespace vmcr::gles {

GlState& state() noexcept {
    static GlState s;
    return s;
}

void set_error(int err) noexcept {
    state().gl_error = err;
}

int get_error() noexcept {
    int e = state().gl_error;
    state().gl_error = 0;
    return e;
}

}  // namespace vmcr::gles
