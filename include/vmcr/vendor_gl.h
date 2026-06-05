// ===========================================================================
// include/vmcr/vendor_gl.h - 原厂 GLES 入口函数表
// 由 dlsym_vendor.cpp 在启动时填充
// ===========================================================================
#pragma once

#include <cstdint>

// 简化的 GLES 类型, 避免引入 GLES 头
using GLenum  = uint32_t;
using GLboolean = uint8_t;
using GLbitfield = uint32_t;
using GLbyte  = int8_t;
using GLubyte = uint8_t;
using GLshort = int16_t;
using GLushort = uint16_t;
using GLint   = int32_t;
using GLuint  = uint32_t;
using GLsizei = int32_t;
using GLfloat = float;
using GLclampf = float;
using GLvoid  = void;
using GLchar  = char;
using GLintptr = intptr_t;
using GLsizeiptr = intptr_t;

namespace vmcr::vendor {

// ---------------------------------------------------------------------------
// 函数指针表 (PFN_*)
// 返回类型按 GLES 3.2 规范, 之前错误地把所有返回 void, 已修正
// ---------------------------------------------------------------------------
struct GlFunctionTable {
    bool loaded = false;

    // ===== Core (void 返回) =====
    void (*glActiveTexture)(GLenum) = nullptr;
    void (*glAttachShader)(GLuint, GLuint) = nullptr;
    void (*glBindAttribLocation)(GLuint, GLuint, const GLchar*) = nullptr;
    void (*glBindBuffer)(GLenum, GLuint) = nullptr;
    void (*glBindFramebuffer)(GLenum, GLuint) = nullptr;
    void (*glBindRenderbuffer)(GLenum, GLuint) = nullptr;
    void (*glBindTexture)(GLenum, GLuint) = nullptr;
    void (*glBindVertexArray)(GLuint) = nullptr;
    void (*glBlendColor)(GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
    void (*glBlendEquation)(GLenum) = nullptr;
    void (*glBlendEquationSeparate)(GLenum, GLenum) = nullptr;
    void (*glBlendFunc)(GLenum, GLenum) = nullptr;
    void (*glBlendFuncSeparate)(GLenum, GLenum, GLenum, GLenum) = nullptr;
    void (*glBufferData)(GLenum, GLsizeiptr, const GLvoid*, GLenum) = nullptr;
    void (*glBufferSubData)(GLenum, GLintptr, GLsizeiptr, const GLvoid*) = nullptr;
    GLenum (*glCheckFramebufferStatus)(GLenum) = nullptr;   // returns GLenum
    void (*glClear)(GLbitfield) = nullptr;
    void (*glClearColor)(GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
    void (*glClearDepthf)(GLfloat) = nullptr;
    void (*glClearStencil)(GLint) = nullptr;
    void (*glColorMask)(GLboolean, GLboolean, GLboolean, GLboolean) = nullptr;
    void (*glCompileShader)(GLuint) = nullptr;
    void (*glCompressedTexImage2D)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*) = nullptr;
    GLuint (*glCreateProgram)(void) = nullptr;                // returns GLuint
    GLuint (*glCreateShader)(GLenum) = nullptr;               // returns GLuint
    void (*glCullFace)(GLenum) = nullptr;
    void (*glDeleteBuffers)(GLsizei, const GLuint*) = nullptr;
    void (*glDeleteFramebuffers)(GLsizei, const GLuint*) = nullptr;
    void (*glDeleteProgram)(GLuint) = nullptr;
    void (*glDeleteRenderbuffers)(GLsizei, const GLuint*) = nullptr;
    void (*glDeleteShader)(GLuint) = nullptr;
    void (*glDeleteTextures)(GLsizei, const GLuint*) = nullptr;
    void (*glDeleteVertexArrays)(GLsizei, const GLuint*) = nullptr;
    void (*glDepthFunc)(GLenum) = nullptr;
    void (*glDepthMask)(GLboolean) = nullptr;
    void (*glDetachShader)(GLuint, GLuint) = nullptr;
    void (*glDisable)(GLenum) = nullptr;
    void (*glDisableVertexAttribArray)(GLuint) = nullptr;
    void (*glDrawArrays)(GLenum, GLint, GLsizei) = nullptr;
    void (*glDrawElements)(GLenum, GLsizei, GLenum, const GLvoid*) = nullptr;
    void (*glDrawElementsInstanced)(GLenum, GLsizei, GLenum, const GLvoid*, GLsizei) = nullptr;
    void (*glEnable)(GLenum) = nullptr;
    void (*glEnableVertexAttribArray)(GLuint) = nullptr;
    void (*glFinish)(void) = nullptr;
    void (*glFlush)(void) = nullptr;
    void (*glFramebufferRenderbuffer)(GLenum, GLenum, GLenum, GLuint) = nullptr;
    void (*glFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint) = nullptr;
    void (*glFrontFace)(GLenum) = nullptr;
    void (*glGenBuffers)(GLsizei, GLuint*) = nullptr;
    void (*glGenerateMipmap)(GLenum) = nullptr;
    void (*glGenFramebuffers)(GLsizei, GLuint*) = nullptr;
    void (*glGenRenderbuffers)(GLsizei, GLuint*) = nullptr;
    void (*glGenTextures)(GLsizei, GLuint*) = nullptr;
    void (*glGenVertexArrays)(GLsizei, GLuint*) = nullptr;
    void (*glGetActiveAttrib)(GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*) = nullptr;
    void (*glGetActiveUniform)(GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*) = nullptr;
    void (*glGetAttachedShaders)(GLuint, GLsizei, GLsizei*, GLuint*) = nullptr;
    GLint (*glGetAttribLocation)(GLuint, const GLchar*) = nullptr;  // returns GLint
    GLenum (*glGetError)(void) = nullptr;                          // returns GLenum
    void (*glGetFloatv)(GLenum, GLfloat*) = nullptr;
    void (*glGetFramebufferAttachmentParameteriv)(GLenum, GLenum, GLenum, GLint*) = nullptr;
    void (*glGetIntegerv)(GLenum, GLint*) = nullptr;
    void (*glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
    void (*glGetProgramiv)(GLuint, GLenum, GLint*) = nullptr;
    void (*glGetRenderbufferParameteriv)(GLenum, GLenum, GLint*) = nullptr;
    void (*glGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
    void (*glGetShaderiv)(GLuint, GLenum, GLint*) = nullptr;
    void (*glGetShaderPrecisionFormat)(GLenum, GLenum, GLint*, GLint*) = nullptr;
    void (*glGetShaderSource)(GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
    const GLubyte* (*glGetString)(GLenum) = nullptr;               // returns const GLubyte*
    const GLubyte* (*glGetStringi)(GLenum, GLuint) = nullptr;       // GL 3.0+ returns const GLubyte*
    void (*glGetTexParameterfv)(GLenum, GLenum, GLfloat*) = nullptr;
    void (*glGetTexParameteriv)(GLenum, GLenum, GLint*) = nullptr;
    void (*glGetUniformfv)(GLuint, GLint, GLfloat*) = nullptr;
    void (*glGetUniformiv)(GLuint, GLint, GLint*) = nullptr;
    GLint (*glGetUniformLocation)(GLuint, const GLchar*) = nullptr; // returns GLint
    void (*glGetVertexAttribfv)(GLuint, GLenum, GLfloat*) = nullptr;
    void (*glGetVertexAttribiv)(GLuint, GLenum, GLint*) = nullptr;
    void (*glHint)(GLenum, GLenum) = nullptr;
    GLboolean (*glIsBuffer)(GLuint) = nullptr;                     // returns GLboolean
    GLboolean (*glIsEnabled)(GLenum) = nullptr;                    // returns GLboolean
    GLboolean (*glIsFramebuffer)(GLuint) = nullptr;               // returns GLboolean
    GLboolean (*glIsProgram)(GLuint) = nullptr;                   // returns GLboolean
    GLboolean (*glIsRenderbuffer)(GLuint) = nullptr;              // returns GLboolean
    GLboolean (*glIsShader)(GLuint) = nullptr;                    // returns GLboolean
    GLboolean (*glIsTexture)(GLuint) = nullptr;                   // returns GLboolean
    GLboolean (*glIsVertexArray)(GLuint) = nullptr;               // returns GLboolean
    void (*glLineWidth)(GLfloat) = nullptr;
    void (*glLinkProgram)(GLuint) = nullptr;
    void (*glPixelStorei)(GLenum, GLint) = nullptr;
    void (*glPolygonOffset)(GLfloat, GLfloat) = nullptr;
    void (*glReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*) = nullptr;
    void (*glRenderbufferStorage)(GLenum, GLenum, GLsizei, GLsizei) = nullptr;
    void (*glRenderbufferStorageMultisample)(GLenum, GLsizei, GLenum, GLsizei, GLsizei) = nullptr;
    void (*glSampleCoverage)(GLfloat, GLboolean) = nullptr;
    void (*glScissor)(GLint, GLint, GLsizei, GLsizei) = nullptr;
    void (*glShaderBinary)(GLsizei, const GLuint*, GLenum, const GLvoid*, GLsizei) = nullptr;
    void (*glShaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*) = nullptr;
    void (*glStencilFunc)(GLenum, GLint, GLuint) = nullptr;
    void (*glStencilFuncSeparate)(GLenum, GLenum, GLint, GLuint) = nullptr;
    void (*glStencilMask)(GLuint) = nullptr;
    void (*glStencilMaskSeparate)(GLenum, GLuint) = nullptr;
    void (*glStencilOp)(GLenum, GLenum, GLenum) = nullptr;
    void (*glStencilOpSeparate)(GLenum, GLenum, GLenum, GLenum) = nullptr;
    void (*glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*) = nullptr;
    void (*glTexParameterf)(GLenum, GLenum, GLfloat) = nullptr;
    void (*glTexParameterfv)(GLenum, GLenum, const GLfloat*) = nullptr;
    void (*glTexParameteri)(GLenum, GLenum, GLint) = nullptr;
    void (*glTexParameteriv)(GLenum, GLenum, const GLint*) = nullptr;
    void (*glTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*) = nullptr;
    void (*glUniform1f)(GLint, GLfloat) = nullptr;
    void (*glUniform1fv)(GLint, GLsizei, const GLfloat*) = nullptr;
    void (*glUniform1i)(GLint, GLint) = nullptr;
    void (*glUniform1iv)(GLint, GLsizei, const GLint*) = nullptr;
    void (*glUniform2f)(GLint, GLfloat, GLfloat) = nullptr;
    void (*glUniform2fv)(GLint, GLsizei, const GLfloat*) = nullptr;
    void (*glUniform2i)(GLint, GLint, GLint) = nullptr;
    void (*glUniform2iv)(GLint, GLsizei, const GLint*) = nullptr;
    void (*glUniform3f)(GLint, GLfloat, GLfloat, GLfloat) = nullptr;
    void (*glUniform3fv)(GLint, GLsizei, const GLfloat*) = nullptr;
    void (*glUniform3i)(GLint, GLint, GLint, GLint) = nullptr;
    void (*glUniform3iv)(GLint, GLsizei, const GLint*) = nullptr;
    void (*glUniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
    void (*glUniform4fv)(GLint, GLsizei, const GLfloat*) = nullptr;
    void (*glUniform4i)(GLint, GLint, GLint, GLint, GLint) = nullptr;
    void (*glUniform4iv)(GLint, GLsizei, const GLint*) = nullptr;
    void (*glUniformMatrix2fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
    void (*glUniformMatrix3fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
    void (*glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
    void (*glUseProgram)(GLuint) = nullptr;
    void (*glValidateProgram)(GLuint) = nullptr;
    void (*glVertexAttrib1f)(GLuint, GLfloat) = nullptr;
    void (*glVertexAttrib1fv)(GLuint, const GLfloat*) = nullptr;
    void (*glVertexAttrib2f)(GLuint, GLfloat, GLfloat) = nullptr;
    void (*glVertexAttrib2fv)(GLuint, const GLfloat*) = nullptr;
    void (*glVertexAttrib3f)(GLuint, GLfloat, GLfloat, GLfloat) = nullptr;
    void (*glVertexAttrib3fv)(GLuint, const GLfloat*) = nullptr;
    void (*glVertexAttrib4f)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
    void (*glVertexAttrib4fv)(GLuint, const GLfloat*) = nullptr;
    void (*glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*) = nullptr;
    void (*glVertexAttribDivisor)(GLuint, GLuint) = nullptr;
    void (*glViewport)(GLint, GLint, GLsizei, GLsizei) = nullptr;

    // ===== EGL (仅 hook 用, forwarder 不需要) =====
    void* eglReserved = nullptr;
};

// 全局单例
GlFunctionTable& gl() noexcept;

// 加载原厂 libGLESv2.so, 解析所有 GLES 3.2 入口
bool load_vendor() noexcept;
void unload_vendor() noexcept;

}  // namespace vmcr::vendor
