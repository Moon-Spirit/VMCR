// ===========================================================================
// src/main/cpp/loader/gl_entry.cpp
// libGL.so 入口符号. 这些函数被 MC 客户端调用, 由 VMCR 路由到
// 当前 backend (Vulkan / GLES).
//
// 符号可见性: 全部 visibility default, 否则 MC System.loadLibrary("GL")
//             找不到这些符号.
//
// 关键: 入口必须放在文件作用域, 不能在 namespace 内, 否则 extern "C"
//       会被 namespace 覆盖, dlsym 找不到符号.
// ===========================================================================
#include "vmcr/log.h"
#include "vmcr/vendor_gl.h"
#include "vmcr/export.h"

#include <cstring>

// =====================================================================
// 辅助函数: 打印转发日志 (TRACE 级别, 默认不开启)
// =====================================================================
static inline void vmcr_forward_log(const char* name) {
    LOG_V(log::kTagGL, "forward: %s", name);
}

namespace vmcr::loader {
namespace {

// 通用转发: 调用 vendor, 若无 vendor 符号则返回默认
template <typename T, typename... Args>
inline T forward_with_default(T (*fn)(Args...), T def, Args... args) {
    if (fn) return fn(args...);
    return def;
}

}  // namespace
}  // namespace vmcr::loader

// =====================================================================
// 通用入口宏 (无返回值)
// =====================================================================
#define FWD0(name) \
    extern "C" VMCR_EXPORT void name() { \
        vmcr_forward_log(#name); \
        auto& t = vmcr::vendor::gl(); \
        if (t.name) t.name(); \
    }

#define FWD1(name, T1, p1) \
    extern "C" VMCR_EXPORT void name(T1 p1) { \
        vmcr_forward_log(#name); \
        auto& t = vmcr::vendor::gl(); \
        if (t.name) t.name(p1); \
    }

#define FWD2(name, T1, p1, T2, p2) \
    extern "C" VMCR_EXPORT void name(T1 p1, T2 p2) { \
        vmcr_forward_log(#name); \
        auto& t = vmcr::vendor::gl(); \
        if (t.name) t.name(p1, p2); \
    }

#define FWD3(name, T1, p1, T2, p2, T3, p3) \
    extern "C" VMCR_EXPORT void name(T1 p1, T2 p2, T3 p3) { \
        vmcr_forward_log(#name); \
        auto& t = vmcr::vendor::gl(); \
        if (t.name) t.name(p1, p2, p3); \
    }

#define FWD4(name, T1, p1, T2, p2, T3, p3, T4, p4) \
    extern "C" VMCR_EXPORT void name(T1 p1, T2 p2, T3 p3, T4 p4) { \
        vmcr_forward_log(#name); \
        auto& t = vmcr::vendor::gl(); \
        if (t.name) t.name(p1, p2, p3, p4); \
    }

#define FWD5(name, T1, p1, T2, p2, T3, p3, T4, p4, T5, p5) \
    extern "C" VMCR_EXPORT void name(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) { \
        vmcr_forward_log(#name); \
        auto& t = vmcr::vendor::gl(); \
        if (t.name) t.name(p1, p2, p3, p4, p5); \
    }

#define FWD6(name, T1, p1, T2, p2, T3, p3, T4, p4, T5, p5, T6, p6) \
    extern "C" VMCR_EXPORT void name(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) { \
        vmcr_forward_log(#name); \
        auto& t = vmcr::vendor::gl(); \
        if (t.name) t.name(p1, p2, p3, p4, p5, p6); \
    }

#define FWD7(name, T1, p1, T2, p2, T3, p3, T4, p4, T5, p5, T6, p6, T7, p7) \
    extern "C" VMCR_EXPORT void name(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) { \
        vmcr_forward_log(#name); \
        auto& t = vmcr::vendor::gl(); \
        if (t.name) t.name(p1, p2, p3, p4, p5, p6, p7); \
    }

#define FWD8(name, T1, p1, T2, p2, T3, p3, T4, p4, T5, p5, T6, p6, T7, p7, T8, p8) \
    extern "C" VMCR_EXPORT void name(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) { \
        vmcr_forward_log(#name); \
        auto& t = vmcr::vendor::gl(); \
        if (t.name) t.name(p1, p2, p3, p4, p5, p6, p7, p8); \
    }

#define FWD9(name, T1, p1, T2, p2, T3, p3, T4, p4, T5, p5, T6, p6, T7, p7, T8, p8, T9, p9) \
    extern "C" VMCR_EXPORT void name(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9) { \
        vmcr_forward_log(#name); \
        auto& t = vmcr::vendor::gl(); \
        if (t.name) t.name(p1, p2, p3, p4, p5, p6, p7, p8, p9); \
    }

// =====================================================================
// 有返回值的入口: 全部 inline, 不使用宏
// (NDK clang 对宏的 token 解析有问题, inline 最可靠)
// =====================================================================

// ---------- 0 参数 ----------
extern "C" VMCR_EXPORT GLuint glCreateProgram() {
    vmcr_forward_log("glCreateProgram");
    auto& t = vmcr::vendor::gl();
    return t.glCreateProgram ? t.glCreateProgram() : 0u;
}

extern "C" VMCR_EXPORT GLenum glGetError() {
    auto& t = vmcr::vendor::gl();
    return t.glGetError ? t.glGetError() : GLenum(0);
}

// ---------- 1 参数 ----------
extern "C" VMCR_EXPORT GLuint glCreateShader(GLenum type) {
    vmcr_forward_log("glCreateShader");
    auto& t = vmcr::vendor::gl();
    return t.glCreateShader ? t.glCreateShader(type) : 0u;
}

extern "C" VMCR_EXPORT const GLubyte* glGetString(GLenum name) {
    auto& t = vmcr::vendor::gl();
    return t.glGetString ? t.glGetString(name) : nullptr;
}

extern "C" VMCR_EXPORT GLboolean glIsBuffer(GLuint buffer) {
    auto& t = vmcr::vendor::gl();
    return t.glIsBuffer ? t.glIsBuffer(buffer) : 0;
}

extern "C" VMCR_EXPORT GLboolean glIsEnabled(GLenum cap) {
    auto& t = vmcr::vendor::gl();
    return t.glIsEnabled ? t.glIsEnabled(cap) : 0;
}

extern "C" VMCR_EXPORT GLboolean glIsFramebuffer(GLuint framebuffer) {
    auto& t = vmcr::vendor::gl();
    return t.glIsFramebuffer ? t.glIsFramebuffer(framebuffer) : 0;
}

extern "C" VMCR_EXPORT GLboolean glIsProgram(GLuint program) {
    auto& t = vmcr::vendor::gl();
    return t.glIsProgram ? t.glIsProgram(program) : 0;
}

extern "C" VMCR_EXPORT GLboolean glIsRenderbuffer(GLuint renderbuffer) {
    auto& t = vmcr::vendor::gl();
    return t.glIsRenderbuffer ? t.glIsRenderbuffer(renderbuffer) : 0;
}

extern "C" VMCR_EXPORT GLboolean glIsShader(GLuint shader) {
    auto& t = vmcr::vendor::gl();
    return t.glIsShader ? t.glIsShader(shader) : 0;
}

extern "C" VMCR_EXPORT GLboolean glIsTexture(GLuint texture) {
    auto& t = vmcr::vendor::gl();
    return t.glIsTexture ? t.glIsTexture(texture) : 0;
}

extern "C" VMCR_EXPORT GLboolean glIsVertexArray(GLuint array) {
    auto& t = vmcr::vendor::gl();
    return t.glIsVertexArray ? t.glIsVertexArray(array) : 0;
}

// ---------- 2 参数 ----------
extern "C" VMCR_EXPORT GLint glGetAttribLocation(GLuint program, const GLchar* name) {
    auto& t = vmcr::vendor::gl();
    return t.glGetAttribLocation ? t.glGetAttribLocation(program, name) : -1;
}

extern "C" VMCR_EXPORT GLint glGetUniformLocation(GLuint program, const GLchar* name) {
    auto& t = vmcr::vendor::gl();
    return t.glGetUniformLocation ? t.glGetUniformLocation(program, name) : -1;
}

// =====================================================================
// 全部 GLES 3.2 入口 (Phase 0/1: forward 到 vendor)
// 必须在文件作用域, extern "C" 才能被 dlsym 找到
// =====================================================================

FWD1 (glActiveTexture, GLenum, texture)
FWD2 (glAttachShader, GLuint, program, GLuint, shader)
FWD3 (glBindAttribLocation, GLuint, program, GLuint, index, const GLchar*, name)
FWD2 (glBindBuffer, GLenum, target, GLuint, buffer)
FWD2 (glBindFramebuffer, GLenum, target, GLuint, framebuffer)
FWD2 (glBindRenderbuffer, GLenum, target, GLuint, renderbuffer)
FWD2 (glBindTexture, GLenum, target, GLuint, texture)
FWD1 (glBindVertexArray, GLuint, array)
FWD4 (glBlendColor, GLfloat, r, GLfloat, g, GLfloat, b, GLfloat, a)
FWD1 (glBlendEquation, GLenum, mode)
FWD2 (glBlendEquationSeparate, GLenum, modeRGB, GLenum, modeA)
FWD2 (glBlendFunc, GLenum, sfactor, GLenum, dfactor)
FWD4 (glBlendFuncSeparate, GLenum, sfactorRGB, GLenum, dfactorRGB, GLenum, sfactorAlpha, GLenum, dfactorAlpha)
FWD4 (glBufferData, GLenum, target, GLsizeiptr, size, const GLvoid*, data, GLenum, usage)
FWD4 (glBufferSubData, GLenum, target, GLintptr, offset, GLsizeiptr, size, const GLvoid*, data)
FWD1 (glClear, GLbitfield, mask)
FWD4 (glClearColor, GLfloat, r, GLfloat, g, GLfloat, b, GLfloat, a)
FWD1 (glClearDepthf, GLfloat, d)
FWD1 (glClearStencil, GLint, s)
FWD4 (glColorMask, GLboolean, r, GLboolean, g, GLboolean, b, GLboolean, a)
FWD1 (glCompileShader, GLuint, shader)
FWD8 (glCompressedTexImage2D, GLenum, target, GLint, level, GLenum, internalformat, GLsizei, width, GLsizei, height, GLint, border, GLsizei, imageSize, const GLvoid*, data)
FWD1 (glCullFace, GLenum, mode)
FWD2 (glDeleteBuffers, GLsizei, n, const GLuint*, buffers)
FWD2 (glDeleteFramebuffers, GLsizei, n, const GLuint*, framebuffers)
FWD1 (glDeleteProgram, GLuint, program)
FWD2 (glDeleteRenderbuffers, GLsizei, n, const GLuint*, renderbuffers)
FWD1 (glDeleteShader, GLuint, shader)
FWD2 (glDeleteTextures, GLsizei, n, const GLuint*, textures)
FWD2 (glDeleteVertexArrays, GLsizei, n, const GLuint*, arrays)
FWD1 (glDepthFunc, GLenum, func)
FWD1 (glDepthMask, GLboolean, flag)
FWD2 (glDetachShader, GLuint, program, GLuint, shader)
FWD1 (glDisable, GLenum, cap)
FWD1 (glDisableVertexAttribArray, GLuint, index)
FWD3 (glDrawArrays, GLenum, mode, GLint, first, GLsizei, count)
FWD4 (glDrawElements, GLenum, mode, GLsizei, count, GLenum, type, const GLvoid*, indices)
FWD5 (glDrawElementsInstanced, GLenum, mode, GLsizei, count, GLenum, type, const GLvoid*, indices, GLsizei, instancecount)
FWD1 (glEnable, GLenum, cap)
FWD1 (glEnableVertexAttribArray, GLuint, index)
FWD0 (glFinish)
FWD0 (glFlush)
FWD4 (glFramebufferRenderbuffer, GLenum, target, GLenum, attachment, GLenum, rbTarget, GLuint, renderbuffer)
FWD5 (glFramebufferTexture2D, GLenum, target, GLenum, attachment, GLenum, textarget, GLuint, texture, GLint, level)
FWD1 (glFrontFace, GLenum, mode)
FWD2 (glGenBuffers, GLsizei, n, GLuint*, buffers)
FWD1 (glGenerateMipmap, GLenum, target)
FWD2 (glGenFramebuffers, GLsizei, n, GLuint*, framebuffers)
FWD2 (glGenRenderbuffers, GLsizei, n, GLuint*, renderbuffers)
FWD2 (glGenTextures, GLsizei, n, GLuint*, textures)
FWD2 (glGenVertexArrays, GLsizei, n, GLuint*, arrays)
FWD7 (glGetActiveAttrib, GLuint, program, GLuint, index, GLsizei, bufSize, GLsizei*, length, GLint*, size, GLenum*, type, GLchar*, name)
FWD7 (glGetActiveUniform, GLuint, program, GLuint, index, GLsizei, bufSize, GLsizei*, length, GLint*, size, GLenum*, type, GLchar*, name)
FWD4 (glGetAttachedShaders, GLuint, program, GLsizei, maxCount, GLsizei*, count, GLuint*, shaders)
FWD2 (glGetFloatv, GLenum, pname, GLfloat*, params)
FWD4 (glGetFramebufferAttachmentParameteriv, GLenum, target, GLenum, attachment, GLenum, pname, GLint*, params)
FWD2 (glGetIntegerv, GLenum, pname, GLint*, params)
FWD4 (glGetProgramInfoLog, GLuint, program, GLsizei, bufSize, GLsizei*, length, GLchar*, infoLog)
FWD3 (glGetProgramiv, GLuint, program, GLenum, pname, GLint*, params)
FWD3 (glGetRenderbufferParameteriv, GLenum, target, GLenum, pname, GLint*, params)
FWD4 (glGetShaderInfoLog, GLuint, shader, GLsizei, bufSize, GLsizei*, length, GLchar*, infoLog)
FWD3 (glGetShaderiv, GLuint, shader, GLenum, pname, GLint*, params)
FWD4 (glGetShaderPrecisionFormat, GLenum, shaderType, GLenum, precisionType, GLint*, range, GLint*, precision)
FWD4 (glGetShaderSource, GLuint, shader, GLsizei, bufSize, GLsizei*, length, GLchar*, source)
FWD3 (glGetTexParameterfv, GLenum, target, GLenum, pname, GLfloat*, params)
FWD3 (glGetTexParameteriv, GLenum, target, GLenum, pname, GLint*, params)
FWD3 (glGetUniformfv, GLuint, program, GLint, location, GLfloat*, params)
FWD3 (glGetUniformiv, GLuint, program, GLint, location, GLint*, params)
FWD3 (glGetVertexAttribfv, GLuint, index, GLenum, pname, GLfloat*, params)
FWD3 (glGetVertexAttribiv, GLuint, index, GLenum, pname, GLint*, params)
FWD2 (glHint, GLenum, target, GLenum, mode)
FWD1 (glLineWidth, GLfloat, width)
FWD1 (glLinkProgram, GLuint, program)
FWD2 (glPixelStorei, GLenum, pname, GLint, param)
FWD2 (glPolygonOffset, GLfloat, factor, GLfloat, units)
FWD7 (glReadPixels, GLint, x, GLint, y, GLsizei, width, GLsizei, height, GLenum, format, GLenum, type, GLvoid*, pixels)
FWD4 (glRenderbufferStorage, GLenum, target, GLenum, internalformat, GLsizei, width, GLsizei, height)
FWD5 (glRenderbufferStorageMultisample, GLenum, target, GLsizei, samples, GLenum, internalformat, GLsizei, width, GLsizei, height)
FWD2 (glSampleCoverage, GLfloat, value, GLboolean, invert)
FWD4 (glScissor, GLint, x, GLint, y, GLsizei, width, GLsizei, height)
FWD5 (glShaderBinary, GLsizei, n, const GLuint*, shaders, GLenum, binaryFormat, const GLvoid*, binary, GLsizei, length)
FWD4 (glShaderSource, GLuint, shader, GLsizei, count, const GLchar* const*, str, const GLint*, length)
FWD3 (glStencilFunc, GLenum, func, GLint, ref, GLuint, mask)
FWD4 (glStencilFuncSeparate, GLenum, face, GLenum, func, GLint, ref, GLuint, mask)
FWD1 (glStencilMask, GLuint, mask)
FWD2 (glStencilMaskSeparate, GLenum, face, GLuint, mask)
FWD3 (glStencilOp, GLenum, fail, GLenum, zfail, GLenum, zpass)
FWD4 (glStencilOpSeparate, GLenum, face, GLenum, sfail, GLenum, dpfail, GLenum, dppass)
FWD9 (glTexImage2D, GLenum, target, GLint, level, GLint, internalformat, GLsizei, width, GLsizei, height, GLint, border, GLenum, format, GLenum, type, const GLvoid*, pixels)
FWD3 (glTexParameterf, GLenum, target, GLenum, pname, GLfloat, param)
FWD3 (glTexParameterfv, GLenum, target, GLenum, pname, const GLfloat*, params)
FWD3 (glTexParameteri, GLenum, target, GLenum, pname, GLint, param)
FWD3 (glTexParameteriv, GLenum, target, GLenum, pname, const GLint*, params)
FWD9 (glTexSubImage2D, GLenum, target, GLint, level, GLint, xoffset, GLint, yoffset, GLsizei, width, GLsizei, height, GLenum, format, GLenum, type, const GLvoid*, pixels)
FWD2 (glUniform1f, GLint, location, GLfloat, v0)
FWD3 (glUniform1fv, GLint, location, GLsizei, count, const GLfloat*, value)
FWD2 (glUniform1i, GLint, location, GLint, v0)
FWD3 (glUniform1iv, GLint, location, GLsizei, count, const GLint*, value)
FWD3 (glUniform2f, GLint, location, GLfloat, v0, GLfloat, v1)
FWD3 (glUniform2fv, GLint, location, GLsizei, count, const GLfloat*, value)
FWD3 (glUniform2i, GLint, location, GLint, v0, GLint, v1)
FWD3 (glUniform2iv, GLint, location, GLsizei, count, const GLint*, value)
FWD4 (glUniform3f, GLint, location, GLfloat, v0, GLfloat, v1, GLfloat, v2)
FWD3 (glUniform3fv, GLint, location, GLsizei, count, const GLfloat*, value)
FWD4 (glUniform3i, GLint, location, GLint, v0, GLint, v1, GLint, v2)
FWD3 (glUniform3iv, GLint, location, GLsizei, count, const GLint*, value)
FWD5 (glUniform4f, GLint, location, GLfloat, v0, GLfloat, v1, GLfloat, v2, GLfloat, v3)
FWD3 (glUniform4fv, GLint, location, GLsizei, count, const GLfloat*, value)
FWD5 (glUniform4i, GLint, location, GLint, v0, GLint, v1, GLint, v2, GLint, v3)
FWD3 (glUniform4iv, GLint, location, GLsizei, count, const GLint*, value)
FWD4 (glUniformMatrix2fv, GLint, location, GLsizei, count, GLboolean, transpose, const GLfloat*, value)
FWD4 (glUniformMatrix3fv, GLint, location, GLsizei, count, GLboolean, transpose, const GLfloat*, value)
FWD4 (glUniformMatrix4fv, GLint, location, GLsizei, count, GLboolean, transpose, const GLfloat*, value)
FWD1 (glUseProgram, GLuint, program)
FWD1 (glValidateProgram, GLuint, program)
FWD2 (glVertexAttrib1f, GLuint, index, GLfloat, v0)
FWD2 (glVertexAttrib1fv, GLuint, index, const GLfloat*, v)
FWD3 (glVertexAttrib2f, GLuint, index, GLfloat, v0, GLfloat, v1)
FWD2 (glVertexAttrib2fv, GLuint, index, const GLfloat*, v)
FWD4 (glVertexAttrib3f, GLuint, index, GLfloat, v0, GLfloat, v1, GLfloat, v2)
FWD2 (glVertexAttrib3fv, GLuint, index, const GLfloat*, v)
FWD5 (glVertexAttrib4f, GLuint, index, GLfloat, v0, GLfloat, v1, GLfloat, v2, GLfloat, v3)
FWD2 (glVertexAttrib4fv, GLuint, index, const GLfloat*, v)
FWD6 (glVertexAttribPointer, GLuint, index, GLint, size, GLenum, type, GLboolean, normalized, GLsizei, stride, const GLvoid*, pointer)
FWD2 (glVertexAttribDivisor, GLuint, index, GLuint, divisor)
FWD4 (glViewport, GLint, x, GLint, y, GLsizei, width, GLsizei, height)
