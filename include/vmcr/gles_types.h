// ===========================================================================
// include/vmcr/gles_types.h
// 简化的 GLES 2.0/3.0/3.2 类型定义 (避免引入 <GLES*/gl.h> 头)
// Phase 0 用
// ===========================================================================
#pragma once

#include <cstdint>

namespace vmcr::gles {

using GLenum      = std::uint32_t;
using GLboolean   = std::uint8_t;
using GLbitfield  = std::uint32_t;
using GLbyte      = std::int8_t;
using GLubyte     = std::uint8_t;
using GLshort     = std::int16_t;
using GLushort    = std::uint16_t;
using GLint       = std::int32_t;
using GLuint      = std::uint32_t;
using GLsizei     = std::int32_t;
using GLfloat     = float;
using GLclampf    = float;
using GLvoid      = void;
using GLchar      = char;
using GLintptr    = std::intptr_t;
using GLsizeiptr  = std::intptr_t;

}  // namespace vmcr::gles
