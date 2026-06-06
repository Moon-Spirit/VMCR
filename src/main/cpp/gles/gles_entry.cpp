// ===========================================================================
// src/main/cpp/gles/gles_entry.cpp
// libGL.so 入口: GLES 2.0 完整 130+ 个 gl* 符号
// Phase 0: 全占位 (状态记录或 no-op)
// Phase 1+: 真实 Vulkan 翻译
// ===========================================================================
#include "vmcr/log.h"
#include "vmcr/export.h"
#include "vmcr/gles_types.h"
#include "gl_state.h"

#include <cstring>

namespace vmcr::gles {
namespace {
constexpr const char* kTag = "VMCR-GL";
inline void bump_counter(std::uint64_t GlState::*member) {
    (state().*member)++;
}
}  // namespace
}  // namespace vmcr::gles

using namespace vmcr::gles;
using GLint   = vmcr::gles::GLint;
using GLuint  = vmcr::gles::GLuint;
using GLsizei = vmcr::gles::GLsizei;

extern "C" {

// ---- State (40) ----
VMCR_EXPORT void glViewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    state().viewport = {x, y, w, h};
}

VMCR_EXPORT void glScissor(GLint x, GLint y, GLsizei w, GLsizei h) {
    // Phase 0: ignore
}

VMCR_EXPORT void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    state().clear_color = {r, g, b, a};
}

VMCR_EXPORT void glClearDepthf(GLfloat d) { state().clear_depth = d; }
VMCR_EXPORT void glClearStencil(GLint s) { state().clear_stencil = s; }

VMCR_EXPORT void glClear(GLbitfield mask) {
    (void)mask;
    state().draw_calls++;
}

VMCR_EXPORT void glEnable(GLenum cap) { (void)cap; }
VMCR_EXPORT void glDisable(GLenum cap) { (void)cap; }
VMCR_EXPORT GLboolean glIsEnabled(GLenum cap) { (void)cap; return 0; }

VMCR_EXPORT void glGetBooleanv(GLenum pname, GLboolean* params) {
    if (params) *params = 0;
    (void)pname;
}
VMCR_EXPORT void glGetIntegerv(GLenum pname, GLint* params) {
    if (!params) return;
    switch (pname) {
        case 0x0D33: *params = 16384; break;  // GL_MAX_TEXTURE_SIZE
        case 0x0D3A: { params[0] = 4096; params[1] = 4096; break; }
        case 0x8869: *params = 16; break;  // GL_MAX_VERTEX_ATTRIBS
        case 0x821B: *params = 3; break;   // GL_MAJOR_VERSION
        case 0x821C: *params = 2; break;   // GL_MINOR_VERSION
        default: *params = 0; break;
    }
}
VMCR_EXPORT void glGetFloatv(GLenum pname, GLfloat* params) {
    if (params) *params = 0;
    (void)pname;
}
VMCR_EXPORT const GLubyte* glGetString(GLenum name) {
    static const char* vendor = "VMCR";
    static const char* renderer = "Adreno (TM) 735 VMCR";
    static const char* version = "OpenGL ES 3.2 VMCR";
    static const char* gles = "OpenGL ES GLSL ES 3.20";
    switch (name) {
        case 0x1F00: return (const GLubyte*)vendor;
        case 0x1F01: return (const GLubyte*)renderer;
        case 0x1F02: return (const GLubyte*)version;
        case 0x8B8C: return (const GLubyte*)gles;
        case 0x1F03: return (const GLubyte*)"";  // GL_EXTENSIONS
        default: return (const GLubyte*)"";
    }
}
VMCR_EXPORT void glHint(GLenum target, GLenum mode) { (void)target; (void)mode; }
VMCR_EXPORT void glPixelStorei(GLenum pname, GLint param) { (void)pname; (void)param; }
VMCR_EXPORT void glReadPixels(GLint x, GLint y, GLsizei w, GLsizei h,
                              GLenum format, GLenum type, GLvoid* pixels) {
    if (pixels) std::memset(pixels, 0, w * h * 4);
}
VMCR_EXPORT GLenum glGetError(void) { return get_error(); }
VMCR_EXPORT void glFinish(void) {}
VMCR_EXPORT void glFlush(void) {}

// ---- Buffer (10) ----
VMCR_EXPORT void glGenBuffers(GLsizei n, GLuint* buffers) {
    if (!buffers) return;
    static GLuint next = 1;
    for (GLsizei i = 0; i < n; ++i) buffers[i] = next++;
}
VMCR_EXPORT void glDeleteBuffers(GLsizei n, const GLuint* buffers) { (void)n; (void)buffers; }
VMCR_EXPORT void glBindBuffer(GLenum target, GLuint buffer) { (void)target; (void)buffer; }
VMCR_EXPORT void glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) {
    (void)target; (void)size; (void)data; (void)usage;
}
VMCR_EXPORT void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data) {
    (void)target; (void)offset; (void)size; (void)data;
}
VMCR_EXPORT GLboolean glIsBuffer(GLuint buffer) { (void)buffer; return 0; }
VMCR_EXPORT void glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params) {
    (void)target; (void)pname; if (params) *params = 0;
}
VMCR_EXPORT void glGetBufferPointerv(GLenum target, GLenum pname, GLvoid** params) {
    (void)target; (void)pname; if (params) *params = nullptr;
}

// ---- Texture (20) ----
VMCR_EXPORT void glGenTextures(GLsizei n, GLuint* textures) {
    if (!textures) return;
    static GLuint next = 1;
    for (GLsizei i = 0; i < n; ++i) textures[i] = next++;
}
VMCR_EXPORT void glDeleteTextures(GLsizei n, const GLuint* textures) { (void)n; (void)textures; }
VMCR_EXPORT void glBindTexture(GLenum target, GLuint texture) { (void)target; (void)texture; }
VMCR_EXPORT void glActiveTexture(GLenum texture) { (void)texture; }
VMCR_EXPORT void glTexImage2D(GLenum target, GLint level, GLint internalformat,
                              GLsizei width, GLsizei height, GLint border,
                              GLenum format, GLenum type, const GLvoid* pixels) {
    (void)target; (void)level; (void)internalformat; (void)border; (void)pixels;
    bump_counter(&GlState::texture_uploads);
}
VMCR_EXPORT void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                  GLsizei width, GLsizei height, GLenum format, GLenum type,
                                  const GLvoid* pixels) {
    (void)target; (void)level; (void)xoffset; (void)yoffset; (void)pixels;
    bump_counter(&GlState::texture_uploads);
}
VMCR_EXPORT void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat,
                                  GLint x, GLint y, GLsizei w, GLsizei h, GLint border) {
    (void)target; (void)level; (void)internalformat; (void)border;
}
VMCR_EXPORT void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                     GLint x, GLint y, GLsizei w, GLsizei h) {
    (void)target; (void)level;
}
VMCR_EXPORT void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat,
                                        GLsizei width, GLsizei height, GLint border,
                                        GLsizei imageSize, const GLvoid* data) {
    (void)target; (void)level; (void)internalformat; (void)border; (void)imageSize; (void)data;
}
VMCR_EXPORT void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                            GLsizei width, GLsizei height, GLenum format,
                                            GLsizei imageSize, const GLvoid* data) {
    (void)target; (void)level; (void)data; (void)imageSize;
}
VMCR_EXPORT void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    (void)target; (void)pname; (void)param;
}
VMCR_EXPORT void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    (void)target; (void)pname; (void)param;
}
VMCR_EXPORT void glTexParameteriv(GLenum target, GLenum pname, const GLint* params) {
    (void)target; (void)pname; (void)params;
}
VMCR_EXPORT void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params) {
    (void)target; (void)pname; (void)params;
}
VMCR_EXPORT void glGetTexParameteriv(GLenum target, GLenum pname, GLint* params) {
    (void)target; (void)pname; if (params) *params = 0;
}
VMCR_EXPORT void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params) {
    (void)target; (void)pname; if (params) *params = 0;
}
VMCR_EXPORT void glGenerateMipmap(GLenum target) { (void)target; }
VMCR_EXPORT GLboolean glIsTexture(GLuint texture) { (void)texture; return 0; }
VMCR_EXPORT void glSampleCoverage(GLfloat v, GLboolean invert) { (void)v; (void)invert; }

// ---- Shader (20) ----
VMCR_EXPORT GLuint glCreateShader(GLenum type) {
    (void)type;
    bump_counter(&GlState::shader_compiles);
    static GLuint next = 1;
    return next++;
}
VMCR_EXPORT void glDeleteShader(GLuint shader) { (void)shader; }
VMCR_EXPORT void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* str, const GLint* length) {
    (void)shader; (void)count; (void)str; (void)length;
}
VMCR_EXPORT void glCompileShader(GLuint shader) { (void)shader; }
VMCR_EXPORT void glGetShaderiv(GLuint shader, GLenum pname, GLint* params) {
    (void)shader; (void)pname; if (params) *params = 1;
}
VMCR_EXPORT void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog) {
    (void)shader; (void)maxLength;
    if (length) *length = 0;
    if (infoLog && maxLength > 0) infoLog[0] = 0;
}
VMCR_EXPORT void glGetShaderPrecisionFormat(GLenum shaderType, GLenum precisionType, GLint* range, GLint* precision) {
    (void)shaderType; (void)precisionType;
    if (range) { range[0] = 127; range[1] = 127; }
    if (precision) *precision = 24;
}
VMCR_EXPORT void glGetShaderSource(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* source) {
    (void)shader; (void)maxLength; if (length) *length = 0; if (source) source[0] = 0;
}

VMCR_EXPORT GLuint glCreateProgram(void) { static GLuint n = 1; return n++; }
VMCR_EXPORT void glDeleteProgram(GLuint program) { (void)program; }
VMCR_EXPORT void glAttachShader(GLuint program, GLuint shader) { (void)program; (void)shader; }
VMCR_EXPORT void glDetachShader(GLuint program, GLuint shader) { (void)program; (void)shader; }
VMCR_EXPORT void glLinkProgram(GLuint program) { (void)program; }
VMCR_EXPORT void glUseProgram(GLuint program) { (void)program; }
VMCR_EXPORT void glGetProgramiv(GLuint program, GLenum pname, GLint* params) {
    (void)program; (void)pname; if (params) *params = 1;
}
VMCR_EXPORT void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog) {
    (void)program; (void)maxLength;
    if (length) *length = 0;
    if (infoLog && maxLength > 0) infoLog[0] = 0;
}
VMCR_EXPORT GLint glGetUniformLocation(GLuint program, const GLchar* name) {
    (void)program; (void)name; return 0;
}
VMCR_EXPORT GLint glGetAttribLocation(GLuint program, const GLchar* name) {
    (void)program; (void)name; return 0;
}
VMCR_EXPORT void glGetActiveUniform(GLuint program, GLuint index, GLsizei maxLength,
                                    GLsizei* length, GLint* size, GLenum* type, GLchar* name) {
    (void)program; (void)index; (void)maxLength; (void)size; (void)type; (void)name;
    if (length) *length = 0;
}
VMCR_EXPORT void glGetActiveAttrib(GLuint program, GLuint index, GLsizei maxLength,
                                   GLsizei* length, GLint* size, GLenum* type, GLchar* name) {
    (void)program; (void)index; (void)maxLength; (void)size; (void)type; (void)name;
    if (length) *length = 0;
}
VMCR_EXPORT GLuint glGetUniformBlockIndex(GLuint program, const GLchar* name) {
    (void)program; (void)name; return 0;
}
VMCR_EXPORT void glGetActiveUniformBlockiv(GLuint program, GLuint index, GLenum pname, GLint* params) {
    (void)program; (void)index; (void)pname; if (params) *params = 0;
}
VMCR_EXPORT void glGetActiveUniformBlockName(GLuint program, GLuint index, GLsizei maxLength,
                                             GLsizei* length, GLchar* name) {
    (void)program; (void)index; (void)maxLength; (void)name;
    if (length) *length = 0;
}

VMCR_EXPORT void glUniform1f(GLint location, GLfloat v0) { (void)location; (void)v0; }
VMCR_EXPORT void glUniform2f(GLint location, GLfloat v0, GLfloat v1) { (void)location; (void)v0; (void)v1; }
VMCR_EXPORT void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) { (void)location; (void)v0; (void)v1; (void)v2; }
VMCR_EXPORT void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) { (void)location; (void)v0; (void)v1; (void)v2; (void)v3; }
VMCR_EXPORT void glUniform1i(GLint location, GLint v0) { (void)location; (void)v0; }
VMCR_EXPORT void glUniform2i(GLint location, GLint v0, GLint v1) { (void)location; (void)v0; (void)v1; }
VMCR_EXPORT void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) { (void)location; (void)v0; (void)v1; (void)v2; }
VMCR_EXPORT void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) { (void)location; (void)v0; (void)v1; (void)v2; (void)v3; }
VMCR_EXPORT void glUniform1fv(GLint location, GLsizei count, const GLfloat* value) { (void)location; (void)count; (void)value; }
VMCR_EXPORT void glUniform2fv(GLint location, GLsizei count, const GLfloat* value) { (void)location; (void)count; (void)value; }
VMCR_EXPORT void glUniform3fv(GLint location, GLsizei count, const GLfloat* value) { (void)location; (void)count; (void)value; }
VMCR_EXPORT void glUniform4fv(GLint location, GLsizei count, const GLfloat* value) { (void)location; (void)count; (void)value; }
VMCR_EXPORT void glUniform1iv(GLint location, GLsizei count, const GLint* value) { (void)location; (void)count; (void)value; }
VMCR_EXPORT void glUniform2iv(GLint location, GLsizei count, const GLint* value) { (void)location; (void)count; (void)value; }
VMCR_EXPORT void glUniform3iv(GLint location, GLsizei count, const GLint* value) { (void)location; (void)count; (void)value; }
VMCR_EXPORT void glUniform4iv(GLint location, GLsizei count, const GLint* value) { (void)location; (void)count; (void)value; }
VMCR_EXPORT void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { (void)location; (void)count; (void)transpose; (void)value; }
VMCR_EXPORT void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { (void)location; (void)count; (void)transpose; (void)value; }
VMCR_EXPORT void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { (void)location; (void)count; (void)transpose; (void)value; }

VMCR_EXPORT void glVertexAttrib1f(GLuint index, GLfloat x) { (void)index; (void)x; }
VMCR_EXPORT void glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y) { (void)index; (void)x; (void)y; }
VMCR_EXPORT void glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z) { (void)index; (void)x; (void)y; (void)z; }
VMCR_EXPORT void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) { (void)index; }
VMCR_EXPORT void glVertexAttrib1fv(GLuint index, const GLfloat* v) { (void)index; (void)v; }
VMCR_EXPORT void glVertexAttrib2fv(GLuint index, const GLfloat* v) { (void)index; (void)v; }
VMCR_EXPORT void glVertexAttrib3fv(GLuint index, const GLfloat* v) { (void)index; (void)v; }
VMCR_EXPORT void glVertexAttrib4fv(GLuint index, const GLfloat* v) { (void)index; (void)v; }
VMCR_EXPORT void glVertexAttribPointer(GLuint index, GLint size, GLenum type,
                                       GLboolean normalized, GLsizei stride, const GLvoid* pointer) {
    (void)index; (void)size; (void)type; (void)normalized; (void)stride; (void)pointer;
}
VMCR_EXPORT void glEnableVertexAttribArray(GLuint index) { (void)index; }
VMCR_EXPORT void glDisableVertexAttribArray(GLuint index) { (void)index; }
VMCR_EXPORT void glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params) {
    (void)index; (void)pname; if (params) *params = 0;
}
VMCR_EXPORT void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params) {
    (void)index; (void)pname; if (params) *params = 0;
}
VMCR_EXPORT void glGetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer) {
    (void)index; (void)pname; if (pointer) *pointer = nullptr;
}

VMCR_EXPORT void glBindAttribLocation(GLuint program, GLuint index, const GLchar* name) {
    (void)program; (void)index; (void)name;
}

// ---- Framebuffer (15) ----
VMCR_EXPORT void glGenFramebuffers(GLsizei n, GLuint* ids) {
    if (!ids) return;
    static GLuint next = 1;
    for (GLsizei i = 0; i < n; ++i) ids[i] = next++;
}
VMCR_EXPORT void glDeleteFramebuffers(GLsizei n, const GLuint* ids) { (void)n; (void)ids; }
VMCR_EXPORT void glBindFramebuffer(GLenum target, GLuint framebuffer) { (void)target; (void)framebuffer; }
VMCR_EXPORT void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    (void)target; (void)attachment; (void)textarget; (void)texture; (void)level;
}
VMCR_EXPORT void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    (void)target; (void)attachment; (void)texture; (void)level; (void)layer;
}
VMCR_EXPORT GLenum glCheckFramebufferStatus(GLenum target) { (void)target; return 0x8CD5; /* GL_FRAMEBUFFER_COMPLETE */ }
VMCR_EXPORT void glGenRenderbuffers(GLsizei n, GLuint* ids) {
    if (!ids) return;
    static GLuint next = 1;
    for (GLsizei i = 0; i < n; ++i) ids[i] = next++;
}
VMCR_EXPORT void glDeleteRenderbuffers(GLsizei n, const GLuint* ids) { (void)n; (void)ids; }
VMCR_EXPORT void glBindRenderbuffer(GLenum target, GLuint renderbuffer) { (void)target; (void)renderbuffer; }
VMCR_EXPORT void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei w, GLsizei h) {
    (void)target; (void)internalformat; (void)w; (void)h;
}
VMCR_EXPORT void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei w, GLsizei h) {
    (void)target; (void)samples; (void)internalformat; (void)w; (void)h;
}
VMCR_EXPORT void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum rbTarget, GLuint renderbuffer) {
    (void)target; (void)attachment; (void)rbTarget; (void)renderbuffer;
}
VMCR_EXPORT GLboolean glIsFramebuffer(GLuint framebuffer) { (void)framebuffer; return 0; }
VMCR_EXPORT GLboolean glIsRenderbuffer(GLuint renderbuffer) { (void)renderbuffer; return 0; }
VMCR_EXPORT void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params) {
    (void)target; (void)attachment; (void)pname; if (params) *params = 0;
}
VMCR_EXPORT void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params) {
    (void)target; (void)pname; if (params) *params = 0;
}

// ---- Draw (10) ----
VMCR_EXPORT void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    (void)mode; (void)first; (void)count;
    bump_counter(&GlState::draw_calls);
}
VMCR_EXPORT void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {
    (void)mode; (void)count; (void)type; (void)indices;
    bump_counter(&GlState::draw_calls);
}
VMCR_EXPORT void glCullFace(GLenum mode) { (void)mode; }
VMCR_EXPORT void glFrontFace(GLenum mode) { (void)mode; }
VMCR_EXPORT void glDepthFunc(GLenum func) { (void)func; }
VMCR_EXPORT void glDepthMask(GLboolean flag) { (void)flag; }
VMCR_EXPORT void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) { (void)r; (void)g; (void)b; (void)a; }
VMCR_EXPORT void glStencilFunc(GLenum func, GLint ref, GLuint mask) { (void)func; (void)ref; (void)mask; }
VMCR_EXPORT void glStencilMask(GLuint mask) { (void)mask; }
VMCR_EXPORT void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) { (void)fail; (void)zfail; (void)zpass; }
VMCR_EXPORT void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) {
    (void)face; (void)func; (void)ref; (void)mask;
}
VMCR_EXPORT void glStencilMaskSeparate(GLenum face, GLuint mask) { (void)face; (void)mask; }
VMCR_EXPORT void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
    (void)face; (void)sfail; (void)dpfail; (void)dppass;
}
VMCR_EXPORT void glBlendFunc(GLenum sfactor, GLenum dfactor) { (void)sfactor; (void)dfactor; }
VMCR_EXPORT void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
    (void)sfactorRGB; (void)dfactorRGB; (void)sfactorAlpha; (void)dfactorAlpha;
}
VMCR_EXPORT void glBlendEquation(GLenum mode) { (void)mode; }
VMCR_EXPORT void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) { (void)modeRGB; (void)modeAlpha; }
VMCR_EXPORT void glLineWidth(GLfloat width) { (void)width; }
VMCR_EXPORT void glPolygonOffset(GLfloat factor, GLfloat units) { (void)factor; (void)units; }
VMCR_EXPORT void glBlendColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { (void)r; (void)g; (void)b; (void)a; }
VMCR_EXPORT void glBlendBarrier(void) {}

// ---- VAO (Vertex Array Object) ----
VMCR_EXPORT void glGenVertexArrays(GLsizei n, GLuint* arrays) {
    if (!arrays) return;
    static GLuint next = 1;
    for (GLsizei i = 0; i < n; ++i) arrays[i] = next++;
}
VMCR_EXPORT void glDeleteVertexArrays(GLsizei n, const GLuint* arrays) { (void)n; (void)arrays; }
VMCR_EXPORT void glBindVertexArray(GLuint array) { (void)array; }
VMCR_EXPORT GLboolean glIsVertexArray(GLuint array) { (void)array; return 0; }

VMCR_EXPORT GLboolean glIsProgram(GLuint program) { (void)program; return 0; }
VMCR_EXPORT GLboolean glIsShader(GLuint shader) { (void)shader; return 0; }

}  // extern "C"
