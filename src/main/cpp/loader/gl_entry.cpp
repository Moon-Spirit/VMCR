// ===========================================================================
// src/main/cpp/loader/gl_entry.cpp
// libGL.so 入口符号. 这些函数被 MC 客户端调用, 由 VMCR 路由到
// 当前 backend (Vulkan / GLES).
//
// 符号可见性: 全部 visibility default, 否则 MC System.loadLibrary("GL")
//             找不到这些符号.
#define VMCR_EXPORTS_GLSYMS 1
#include "vmcr/log.h"
#include "vmcr/vendor_gl.h"

#include <cstring>

// 全局 backend 句柄 (来自 libvmcr_vk.so / libvmcr_gles.so)
extern "C" {
    // 由 libvmcr_vk.so / libvmcr_gles.so 提供的可选入口
    // 若 dlsym 失败, 则 GLES32 路径就是默认 forwarder
    extern void vmcr_renderer_register() __attribute__((weak));
}

namespace {

// 简易的 hook 路由: 现在所有 GL 调用直接走 vendor (Phase 0/1 行为)
// Phase 2 之后, 由 BackendRouter.current() 决定走 VK 还是 GLES.
template <typename Fn, typename... Args>
auto call_vendor_gl(Fn fn, Args... args) {
    return fn(args...);
}

}  // namespace

// =====================================================================
// GLES 3.2 入口 - 全部 forward 到 vendor
// (Phase 0/1 行为: 不重写任何 GL 状态机)
// =====================================================================

#define DEFINE_GL(name, sig, params) \
    extern "C" __attribute__((visibility("default"))) \
    sig { \
        auto& t = vmcr::vendor::gl(); \
        if (t.name) t.name params; \
    }

#define DEFINE_GL_RET(name, sig, params, ret_init) \
    extern "C" __attribute__((visibility("default"))) \
    sig { \
        auto& t = vmcr::vendor::gl(); \
        ret_init; \
        if (t.name) return t.name params; \
        return ret_init; \
    }

DEFINE_GL(glActiveTexture,                  void (GLenum texture),                                 (texture))
DEFINE_GL(glAttachShader,                   void (GLuint program, GLuint shader),                  (program, shader))
DEFINE_GL(glBindAttribLocation,             void (GLuint program, GLuint index, const GLchar* name),(program, index, name))
DEFINE_GL(glBindBuffer,                     void (GLenum target, GLuint buffer),                   (target, buffer))
DEFINE_GL(glBindFramebuffer,                void (GLenum target, GLuint framebuffer),              (target, framebuffer))
DEFINE_GL(glBindRenderbuffer,               void (GLenum target, GLuint renderbuffer),             (target, renderbuffer))
DEFINE_GL(glBindTexture,                    void (GLenum target, GLuint texture),                  (target, texture))
DEFINE_GL(glBindVertexArray,                void (GLuint array),                                   (array))
DEFINE_GL(glBlendColor,                     void (GLfloat r, GLfloat g, GLfloat b, GLfloat a),     (r, g, b, a))
DEFINE_GL(glBlendEquation,                  void (GLenum mode),                                    (mode))
DEFINE_GL(glBlendEquationSeparate,          void (GLenum modeRGB, GLenum modeA),                   (modeRGB, modeA))
DEFINE_GL(glBlendFunc,                      void (GLenum sfactor, GLenum dfactor),                 (sfactor, dfactor))
DEFINE_GL(glBlendFuncSeparate,              void (GLenum sfactorRGB, GLenum dfactorRGB,
                                                GLenum sfactorAlpha, GLenum dfactorAlpha),        (sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha))
DEFINE_GL(glBufferData,                     void (GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage), (target, size, data, usage))
DEFINE_GL(glBufferSubData,                  void (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data), (target, offset, size, data))
DEFINE_GL(glClear,                          void (GLbitfield mask),                                (mask))
DEFINE_GL(glClearColor,                     void (GLfloat r, GLfloat g, GLfloat b, GLfloat a),     (r, g, b, a))
DEFINE_GL(glClearDepthf,                    void (GLfloat d),                                      (d))
DEFINE_GL(glClearStencil,                   void (GLint s),                                        (s))
DEFINE_GL(glColorMask,                      void (GLboolean r, GLboolean g, GLboolean b, GLboolean a), (r, g, b, a))
DEFINE_GL(glCompileShader,                  void (GLuint shader),                                  (shader))
DEFINE_GL(glCompressedTexImage2D,           void (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data), (target, level, internalformat, width, height, border, imageSize, data))
DEFINE_GL_RET(glCreateProgram,              GLuint (void),                                         (), GLuint(0))
DEFINE_GL(glCreateShader,                   GLuint (GLenum type),                                  (type))
DEFINE_GL(glCullFace,                       void (GLenum mode),                                    (mode))
DEFINE_GL(glDeleteBuffers,                  void (GLsizei n, const GLuint* buffers),               (n, buffers))
DEFINE_GL(glDeleteFramebuffers,             void (GLsizei n, const GLuint* framebuffers),          (n, framebuffers))
DEFINE_GL(glDeleteProgram,                  void (GLuint program),                                 (program))
DEFINE_GL(glDeleteRenderbuffers,            void (GLsizei n, const GLuint* renderbuffers),         (n, renderbuffers))
DEFINE_GL(glDeleteShader,                   void (GLuint shader),                                  (shader))
DEFINE_GL(glDeleteTextures,                 void (GLsizei n, const GLuint* textures),              (n, textures))
DEFINE_GL(glDeleteVertexArrays,             void (GLsizei n, const GLuint* arrays),                (n, arrays))
DEFINE_GL(glDepthFunc,                      void (GLenum func),                                    (func))
DEFINE_GL(glDepthMask,                      void (GLboolean flag),                                 (flag))
DEFINE_GL(glDetachShader,                   void (GLuint program, GLuint shader),                  (program, shader))
DEFINE_GL(glDisable,                        void (GLenum cap),                                     (cap))
DEFINE_GL(glDisableVertexAttribArray,       void (GLuint index),                                   (index))
DEFINE_GL(glDrawArrays,                     void (GLenum mode, GLint first, GLsizei count),        (mode, first, count))
DEFINE_GL(glDrawElements,                   void (GLenum mode, GLsizei count, GLenum type, const GLvoid* indices), (mode, count, type, indices))
DEFINE_GL(glDrawElementsInstanced,          void (GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei instancecount), (mode, count, type, indices, instancecount))
DEFINE_GL(glEnable,                         void (GLenum cap),                                     (cap))
DEFINE_GL(glEnableVertexAttribArray,        void (GLuint index),                                   (index))
DEFINE_GL(glFinish,                         void (void),                                           ())
DEFINE_GL(glFlush,                          void (void),                                           ())
DEFINE_GL(glFramebufferRenderbuffer,        void (GLenum target, GLenum attachment, GLenum rbTarget, GLuint renderbuffer), (target, attachment, rbTarget, renderbuffer))
DEFINE_GL(glFramebufferTexture2D,           void (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level), (target, attachment, textarget, texture, level))
DEFINE_GL(glFrontFace,                      void (GLenum mode),                                    (mode))
DEFINE_GL(glGenBuffers,                     void (GLsizei n, GLuint* buffers),                     (n, buffers))
DEFINE_GL(glGenerateMipmap,                 void (GLenum target),                                  (target))
DEFINE_GL(glGenFramebuffers,                void (GLsizei n, GLuint* framebuffers),                (n, framebuffers))
DEFINE_GL(glGenRenderbuffers,               void (GLsizei n, GLuint* renderbuffers),               (n, renderbuffers))
DEFINE_GL(glGenTextures,                    void (GLsizei n, GLuint* textures),                    (n, textures))
DEFINE_GL(glGenVertexArrays,                void (GLsizei n, GLuint* arrays),                      (n, arrays))
DEFINE_GL(glGetActiveAttrib,                void (GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name), (program, index, bufSize, length, size, type, name))
DEFINE_GL(glGetActiveUniform,               void (GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name), (program, index, bufSize, length, size, type, name))
DEFINE_GL(glGetAttachedShaders,             void (GLuint program, GLsizei maxCount, GLsizei* count, GLuint* shaders), (program, maxCount, count, shaders))
DEFINE_GL(glGetAttribLocation,              GLint (GLuint program, const GLchar* name),           (program, name))
DEFINE_GL_RET(glGetError,                   GLenum (void),                                         (), GLenum(GL_NO_ERROR))
DEFINE_GL(glGetFloatv,                      void (GLenum pname, GLfloat* params),                  (pname, params))
DEFINE_GL(glGetFramebufferAttachmentParameteriv, void (GLenum target, GLenum attachment, GLenum pname, GLint* params), (target, attachment, pname, params))
DEFINE_GL(glGetIntegerv,                    void (GLenum pname, GLint* params),                    (pname, params))
DEFINE_GL(glGetProgramInfoLog,              void (GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog), (program, bufSize, length, infoLog))
DEFINE_GL(glGetProgramiv,                   void (GLuint program, GLenum pname, GLint* params),    (program, pname, params))
DEFINE_GL(glGetRenderbufferParameteriv,     void (GLenum target, GLenum pname, GLint* params),    (target, pname, params))
DEFINE_GL(glGetShaderInfoLog,               void (GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog), (shader, bufSize, length, infoLog))
DEFINE_GL(glGetShaderiv,                    void (GLuint shader, GLenum pname, GLint* params),    (shader, pname, params))
DEFINE_GL(glGetShaderPrecisionFormat,       void (GLenum shaderType, GLenum precisionType, GLint* range, GLint* precision), (shaderType, precisionType, range, precision))
DEFINE_GL(glGetShaderSource,                void (GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* source), (shader, bufSize, length, source))
DEFINE_GL_RET(glGetString,                  const GLubyte* (GLenum name),                          (name), const GLubyte*(nullptr))
DEFINE_GL(glGetTexParameterfv,              void (GLenum target, GLenum pname, GLfloat* params),  (target, pname, params))
DEFINE_GL(glGetTexParameteriv,              void (GLenum target, GLenum pname, GLint* params),    (target, pname, params))
DEFINE_GL(glGetUniformfv,                   void (GLuint program, GLint location, GLfloat* params), (program, location, params))
DEFINE_GL(glGetUniformiv,                   void (GLuint program, GLint location, GLint* params), (program, location, params))
DEFINE_GL_RET(glGetUniformLocation,         GLint (GLuint program, const GLchar* name),           (program, name), GLint(-1))
DEFINE_GL(glGetVertexAttribfv,              void (GLuint index, GLenum pname, GLfloat* params),   (index, pname, params))
DEFINE_GL(glGetVertexAttribiv,              void (GLuint index, GLenum pname, GLint* params),     (index, pname, params))
DEFINE_GL(glHint,                           void (GLenum target, GLenum mode),                     (target, mode))
DEFINE_GL_RET(glIsBuffer,                   GLboolean (GLuint buffer),                             (buffer), GLboolean(0))
DEFINE_GL_RET(glIsEnabled,                  GLboolean (GLenum cap),                                (cap),    GLboolean(0))
DEFINE_GL_RET(glIsFramebuffer,              GLboolean (GLuint framebuffer),                       (framebuffer), GLboolean(0))
DEFINE_GL_RET(glIsProgram,                  GLboolean (GLuint program),                           (program), GLboolean(0))
DEFINE_GL_RET(glIsRenderbuffer,             GLboolean (GLuint renderbuffer),                      (renderbuffer), GLboolean(0))
DEFINE_GL_RET(glIsShader,                   GLboolean (GLuint shader),                             (shader), GLboolean(0))
DEFINE_GL_RET(glIsTexture,                  GLboolean (GLuint texture),                           (texture), GLboolean(0))
DEFINE_GL_RET(glIsVertexArray,              GLboolean (GLuint array),                              (array), GLboolean(0))
DEFINE_GL(glLineWidth,                      void (GLfloat width),                                 (width))
DEFINE_GL(glLinkProgram,                    void (GLuint program),                                 (program))
DEFINE_GL(glPixelStorei,                    void (GLenum pname, GLint param),                      (pname, param))
DEFINE_GL(glPolygonOffset,                  void (GLfloat factor, GLfloat units),                 (factor, units))
DEFINE_GL(glReadPixels,                     void (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels), (x, y, width, height, format, type, pixels))
DEFINE_GL(glRenderbufferStorage,            void (GLenum target, GLenum internalformat, GLsizei width, GLsizei height), (target, internalformat, width, height))
DEFINE_GL(glRenderbufferStorageMultisample, void (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height), (target, samples, internalformat, width, height))
DEFINE_GL(glSampleCoverage,                 void (GLfloat value, GLboolean invert),                (value, invert))
DEFINE_GL(glScissor,                        void (GLint x, GLint y, GLsizei width, GLsizei height), (x, y, width, height))
DEFINE_GL(glShaderBinary,                   void (GLsizei n, const GLuint* shaders, GLenum binaryFormat, const GLvoid* binary, GLsizei length), (n, shaders, binaryFormat, binary, length))
DEFINE_GL(glShaderSource,                   void (GLuint shader, GLsizei count, const GLchar* const* str, const GLint* length), (shader, count, str, length))
DEFINE_GL(glStencilFunc,                    void (GLenum func, GLint ref, GLuint mask),            (func, ref, mask))
DEFINE_GL(glStencilFuncSeparate,            void (GLenum face, GLenum func, GLint ref, GLuint mask), (face, func, ref, mask))
DEFINE_GL(glStencilMask,                    void (GLuint mask),                                    (mask))
DEFINE_GL(glStencilMaskSeparate,            void (GLenum face, GLuint mask),                       (face, mask))
DEFINE_GL(glStencilOp,                      void (GLenum fail, GLenum zfail, GLenum zpass),       (fail, zfail, zpass))
DEFINE_GL(glStencilOpSeparate,              void (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass), (face, sfail, dpfail, dppass))
DEFINE_GL(glTexImage2D,                     void (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels), (target, level, internalformat, width, height, border, format, type, pixels))
DEFINE_GL(glTexParameterf,                  void (GLenum target, GLenum pname, GLfloat param),    (target, pname, param))
DEFINE_GL(glTexParameterfv,                 void (GLenum target, GLenum pname, const GLfloat* params), (target, pname, params))
DEFINE_GL(glTexParameteri,                  void (GLenum target, GLenum pname, GLint param),      (target, pname, param))
DEFINE_GL(glTexParameteriv,                 void (GLenum target, GLenum pname, const GLint* params), (target, pname, params))
DEFINE_GL(glTexSubImage2D,                  void (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels), (target, level, xoffset, yoffset, width, height, format, type, pixels))
DEFINE_GL(glUniform1f,                      void (GLint location, GLfloat v0),                     (location, v0))
DEFINE_GL(glUniform1fv,                     void (GLint location, GLsizei count, const GLfloat* value), (location, count, value))
DEFINE_GL(glUniform1i,                      void (GLint location, GLint v0),                       (location, v0))
DEFINE_GL(glUniform1iv,                     void (GLint location, GLsizei count, const GLint* value), (location, count, value))
DEFINE_GL(glUniform2f,                      void (GLint location, GLfloat v0, GLfloat v1),         (location, v0, v1))
DEFINE_GL(glUniform2fv,                     void (GLint location, GLsizei count, const GLfloat* value), (location, count, value))
DEFINE_GL(glUniform2i,                      void (GLint location, GLint v0, GLint v1),             (location, v0, v1))
DEFINE_GL(glUniform2iv,                     void (GLint location, GLsizei count, const GLint* value), (location, count, value))
DEFINE_GL(glUniform3f,                      void (GLint location, GLfloat v0, GLfloat v1, GLfloat v2), (location, v0, v1, v2))
DEFINE_GL(glUniform3fv,                     void (GLint location, GLsizei count, const GLfloat* value), (location, count, value))
DEFINE_GL(glUniform3i,                      void (GLint location, GLint v0, GLint v1, GLint v2),   (location, v0, v1, v2))
DEFINE_GL(glUniform3iv,                     void (GLint location, GLsizei count, const GLint* value), (location, count, value))
DEFINE_GL(glUniform4f,                      void (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3), (location, v0, v1, v2, v3))
DEFINE_GL(glUniform4fv,                     void (GLint location, GLsizei count, const GLfloat* value), (location, count, value))
DEFINE_GL(glUniform4i,                      void (GLint location, GLint v0, GLint v1, GLint v2, GLint v3), (location, v0, v1, v2, v3))
DEFINE_GL(glUniform4iv,                     void (GLint location, GLsizei count, const GLint* value), (location, count, value))
DEFINE_GL(glUniformMatrix2fv,               void (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (location, count, transpose, value))
DEFINE_GL(glUniformMatrix3fv,               void (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (location, count, transpose, value))
DEFINE_GL(glUniformMatrix4fv,               void (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (location, count, transpose, value))
DEFINE_GL(glUseProgram,                     void (GLuint program),                                 (program))
DEFINE_GL(glValidateProgram,                void (GLuint program),                                 (program))
DEFINE_GL(glVertexAttrib1f,                 void (GLuint index, GLfloat v0),                      (index, v0))
DEFINE_GL(glVertexAttrib1fv,                void (GLuint index, const GLfloat* v),                (index, v))
DEFINE_GL(glVertexAttrib2f,                 void (GLuint index, GLfloat v0, GLfloat v1),          (index, v0, v1))
DEFINE_GL(glVertexAttrib2fv,                void (GLuint index, const GLfloat* v),                (index, v))
DEFINE_GL(glVertexAttrib3f,                 void (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2), (index, v0, v1, v2))
DEFINE_GL(glVertexAttrib3fv,                void (GLuint index, const GLfloat* v),                (index, v))
DEFINE_GL(glVertexAttrib4f,                 void (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3), (index, v0, v1, v2, v3))
DEFINE_GL(glVertexAttrib4fv,                void (GLuint index, const GLfloat* v),                (index, v))
DEFINE_GL(glVertexAttribPointer,            void (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer), (index, size, type, normalized, stride, pointer))
DEFINE_GL(glVertexAttribDivisor,            void (GLuint index, GLuint divisor),                   (index, divisor))
DEFINE_GL(glViewport,                       void (GLint x, GLint y, GLsizei width, GLsizei height), (x, y, width, height))
