// ===========================================================================
// src/main/cpp/loader/dlsym_vendor.cpp
// 加载并解析原厂 libGLESv2.so / libEGL.so 的所有入口
// ===========================================================================
#include "vmcr/log.h"
#include "vmcr/vendor_gl.h"
#include "vmcr/vendor_egl.h"
#include "vmcr/export.h"

#include <dlfcn.h>
#include <cstring>

namespace vmcr::vendor {

namespace {

GlFunctionTable  g_gl;
EglFunctionTable g_egl;
void*            g_lib_gles = nullptr;
void*            g_lib_egl  = nullptr;
bool             g_loaded   = false;

// 辅助: dlsym 取符号, 失败时返回 nullptr
template <typename T>
T sym(void* h, const char* name) {
    return reinterpret_cast<T>(::dlsym(h, name));
}

}  // namespace

// 全局访问器 (放在匿名命名空间外, 可被外部调用)
// 标记 VMCR_EXPORT 以便其他 .so (libEGL.so) 能 dlsym 到
VMCR_EXPORT GlFunctionTable& gl() noexcept { return g_gl; }
VMCR_EXPORT EglFunctionTable& egl() noexcept { return g_egl; }

bool load_vendor() noexcept {
    if (g_loaded) return true;

    // --- libGLESv2.so ---------------------------------------------------
    g_lib_gles = ::dlopen("libGLESv2.so", RTLD_NOW | RTLD_LOCAL);
    if (!g_lib_gles) {
        LOG_E(log::kTagCore, "dlopen(libGLESv2.so) failed: %s", ::dlerror());
        return false;
    }

    auto& t = g_gl;
    t.glActiveTexture                  = sym<decltype(t.glActiveTexture)>(g_lib_gles, "glActiveTexture");
    t.glAttachShader                   = sym<decltype(t.glAttachShader)>(g_lib_gles, "glAttachShader");
    t.glBindAttribLocation             = sym<decltype(t.glBindAttribLocation)>(g_lib_gles, "glBindAttribLocation");
    t.glBindBuffer                     = sym<decltype(t.glBindBuffer)>(g_lib_gles, "glBindBuffer");
    t.glBindFramebuffer                = sym<decltype(t.glBindFramebuffer)>(g_lib_gles, "glBindFramebuffer");
    t.glBindRenderbuffer               = sym<decltype(t.glBindRenderbuffer)>(g_lib_gles, "glBindRenderbuffer");
    t.glBindTexture                    = sym<decltype(t.glBindTexture)>(g_lib_gles, "glBindTexture");
    t.glBindVertexArray                = sym<decltype(t.glBindVertexArray)>(g_lib_gles, "glBindVertexArray");
    t.glBlendColor                     = sym<decltype(t.glBlendColor)>(g_lib_gles, "glBlendColor");
    t.glBlendEquation                  = sym<decltype(t.glBlendEquation)>(g_lib_gles, "glBlendEquation");
    t.glBlendEquationSeparate          = sym<decltype(t.glBlendEquationSeparate)>(g_lib_gles, "glBlendEquationSeparate");
    t.glBlendFunc                      = sym<decltype(t.glBlendFunc)>(g_lib_gles, "glBlendFunc");
    t.glBlendFuncSeparate              = sym<decltype(t.glBlendFuncSeparate)>(g_lib_gles, "glBlendFuncSeparate");
    t.glBufferData                     = sym<decltype(t.glBufferData)>(g_lib_gles, "glBufferData");
    t.glBufferSubData                  = sym<decltype(t.glBufferSubData)>(g_lib_gles, "glBufferSubData");
    t.glCheckFramebufferStatus         = sym<decltype(t.glCheckFramebufferStatus)>(g_lib_gles, "glCheckFramebufferStatus");
    t.glClear                          = sym<decltype(t.glClear)>(g_lib_gles, "glClear");
    t.glClearColor                     = sym<decltype(t.glClearColor)>(g_lib_gles, "glClearColor");
    t.glClearDepthf                    = sym<decltype(t.glClearDepthf)>(g_lib_gles, "glClearDepthf");
    t.glClearStencil                   = sym<decltype(t.glClearStencil)>(g_lib_gles, "glClearStencil");
    t.glColorMask                      = sym<decltype(t.glColorMask)>(g_lib_gles, "glColorMask");
    t.glCompileShader                  = sym<decltype(t.glCompileShader)>(g_lib_gles, "glCompileShader");
    t.glCompressedTexImage2D           = sym<decltype(t.glCompressedTexImage2D)>(g_lib_gles, "glCompressedTexImage2D");
    t.glCreateProgram                  = sym<decltype(t.glCreateProgram)>(g_lib_gles, "glCreateProgram");
    t.glCreateShader                   = sym<decltype(t.glCreateShader)>(g_lib_gles, "glCreateShader");
    t.glCullFace                       = sym<decltype(t.glCullFace)>(g_lib_gles, "glCullFace");
    t.glDeleteBuffers                  = sym<decltype(t.glDeleteBuffers)>(g_lib_gles, "glDeleteBuffers");
    t.glDeleteFramebuffers             = sym<decltype(t.glDeleteFramebuffers)>(g_lib_gles, "glDeleteFramebuffers");
    t.glDeleteProgram                  = sym<decltype(t.glDeleteProgram)>(g_lib_gles, "glDeleteProgram");
    t.glDeleteRenderbuffers            = sym<decltype(t.glDeleteRenderbuffers)>(g_lib_gles, "glDeleteRenderbuffers");
    t.glDeleteShader                   = sym<decltype(t.glDeleteShader)>(g_lib_gles, "glDeleteShader");
    t.glDeleteTextures                 = sym<decltype(t.glDeleteTextures)>(g_lib_gles, "glDeleteTextures");
    t.glDeleteVertexArrays             = sym<decltype(t.glDeleteVertexArrays)>(g_lib_gles, "glDeleteVertexArrays");
    t.glDepthFunc                      = sym<decltype(t.glDepthFunc)>(g_lib_gles, "glDepthFunc");
    t.glDepthMask                      = sym<decltype(t.glDepthMask)>(g_lib_gles, "glDepthMask");
    t.glDetachShader                   = sym<decltype(t.glDetachShader)>(g_lib_gles, "glDetachShader");
    t.glDisable                        = sym<decltype(t.glDisable)>(g_lib_gles, "glDisable");
    t.glDisableVertexAttribArray       = sym<decltype(t.glDisableVertexAttribArray)>(g_lib_gles, "glDisableVertexAttribArray");
    t.glDrawArrays                     = sym<decltype(t.glDrawArrays)>(g_lib_gles, "glDrawArrays");
    t.glDrawElements                   = sym<decltype(t.glDrawElements)>(g_lib_gles, "glDrawElements");
    t.glDrawElementsInstanced          = sym<decltype(t.glDrawElementsInstanced)>(g_lib_gles, "glDrawElementsInstanced");
    t.glEnable                         = sym<decltype(t.glEnable)>(g_lib_gles, "glEnable");
    t.glEnableVertexAttribArray        = sym<decltype(t.glEnableVertexAttribArray)>(g_lib_gles, "glEnableVertexAttribArray");
    t.glFinish                         = sym<decltype(t.glFinish)>(g_lib_gles, "glFinish");
    t.glFlush                          = sym<decltype(t.glFlush)>(g_lib_gles, "glFlush");
    t.glFramebufferRenderbuffer        = sym<decltype(t.glFramebufferRenderbuffer)>(g_lib_gles, "glFramebufferRenderbuffer");
    t.glFramebufferTexture2D           = sym<decltype(t.glFramebufferTexture2D)>(g_lib_gles, "glFramebufferTexture2D");
    t.glFrontFace                      = sym<decltype(t.glFrontFace)>(g_lib_gles, "glFrontFace");
    t.glGenBuffers                     = sym<decltype(t.glGenBuffers)>(g_lib_gles, "glGenBuffers");
    t.glGenerateMipmap                 = sym<decltype(t.glGenerateMipmap)>(g_lib_gles, "glGenerateMipmap");
    t.glGenFramebuffers                = sym<decltype(t.glGenFramebuffers)>(g_lib_gles, "glGenFramebuffers");
    t.glGenRenderbuffers               = sym<decltype(t.glGenRenderbuffers)>(g_lib_gles, "glGenRenderbuffers");
    t.glGenTextures                    = sym<decltype(t.glGenTextures)>(g_lib_gles, "glGenTextures");
    t.glGenVertexArrays                = sym<decltype(t.glGenVertexArrays)>(g_lib_gles, "glGenVertexArrays");
    t.glGetActiveAttrib                = sym<decltype(t.glGetActiveAttrib)>(g_lib_gles, "glGetActiveAttrib");
    t.glGetActiveUniform               = sym<decltype(t.glGetActiveUniform)>(g_lib_gles, "glGetActiveUniform");
    t.glGetAttachedShaders             = sym<decltype(t.glGetAttachedShaders)>(g_lib_gles, "glGetAttachedShaders");
    t.glGetAttribLocation              = sym<decltype(t.glGetAttribLocation)>(g_lib_gles, "glGetAttribLocation");
    t.glGetError                       = sym<decltype(t.glGetError)>(g_lib_gles, "glGetError");
    t.glGetFloatv                      = sym<decltype(t.glGetFloatv)>(g_lib_gles, "glGetFloatv");
    t.glGetFramebufferAttachmentParameteriv = sym<decltype(t.glGetFramebufferAttachmentParameteriv)>(g_lib_gles, "glGetFramebufferAttachmentParameteriv");
    t.glGetIntegerv                    = sym<decltype(t.glGetIntegerv)>(g_lib_gles, "glGetIntegerv");
    t.glGetProgramInfoLog              = sym<decltype(t.glGetProgramInfoLog)>(g_lib_gles, "glGetProgramInfoLog");
    t.glGetProgramiv                   = sym<decltype(t.glGetProgramiv)>(g_lib_gles, "glGetProgramiv");
    t.glGetRenderbufferParameteriv     = sym<decltype(t.glGetRenderbufferParameteriv)>(g_lib_gles, "glGetRenderbufferParameteriv");
    t.glGetShaderInfoLog               = sym<decltype(t.glGetShaderInfoLog)>(g_lib_gles, "glGetShaderInfoLog");
    t.glGetShaderiv                    = sym<decltype(t.glGetShaderiv)>(g_lib_gles, "glGetShaderiv");
    t.glGetShaderPrecisionFormat       = sym<decltype(t.glGetShaderPrecisionFormat)>(g_lib_gles, "glGetShaderPrecisionFormat");
    t.glGetShaderSource                = sym<decltype(t.glGetShaderSource)>(g_lib_gles, "glGetShaderSource");
    t.glGetString                      = sym<decltype(t.glGetString)>(g_lib_gles, "glGetString");
    t.glGetStringi                     = sym<decltype(t.glGetStringi)>(g_lib_gles, "glGetStringi");
    t.glGetTexParameterfv              = sym<decltype(t.glGetTexParameterfv)>(g_lib_gles, "glGetTexParameterfv");
    t.glGetTexParameteriv              = sym<decltype(t.glGetTexParameteriv)>(g_lib_gles, "glGetTexParameteriv");
    t.glGetUniformfv                   = sym<decltype(t.glGetUniformfv)>(g_lib_gles, "glGetUniformfv");
    t.glGetUniformiv                   = sym<decltype(t.glGetUniformiv)>(g_lib_gles, "glGetUniformiv");
    t.glGetUniformLocation             = sym<decltype(t.glGetUniformLocation)>(g_lib_gles, "glGetUniformLocation");
    t.glGetVertexAttribfv              = sym<decltype(t.glGetVertexAttribfv)>(g_lib_gles, "glGetVertexAttribfv");
    t.glGetVertexAttribiv              = sym<decltype(t.glGetVertexAttribiv)>(g_lib_gles, "glGetVertexAttribiv");
    t.glHint                           = sym<decltype(t.glHint)>(g_lib_gles, "glHint");
    t.glIsBuffer                       = sym<decltype(t.glIsBuffer)>(g_lib_gles, "glIsBuffer");
    t.glIsEnabled                      = sym<decltype(t.glIsEnabled)>(g_lib_gles, "glIsEnabled");
    t.glIsFramebuffer                  = sym<decltype(t.glIsFramebuffer)>(g_lib_gles, "glIsFramebuffer");
    t.glIsProgram                      = sym<decltype(t.glIsProgram)>(g_lib_gles, "glIsProgram");
    t.glIsRenderbuffer                 = sym<decltype(t.glIsRenderbuffer)>(g_lib_gles, "glIsRenderbuffer");
    t.glIsShader                       = sym<decltype(t.glIsShader)>(g_lib_gles, "glIsShader");
    t.glIsTexture                      = sym<decltype(t.glIsTexture)>(g_lib_gles, "glIsTexture");
    t.glIsVertexArray                  = sym<decltype(t.glIsVertexArray)>(g_lib_gles, "glIsVertexArray");
    t.glLineWidth                      = sym<decltype(t.glLineWidth)>(g_lib_gles, "glLineWidth");
    t.glLinkProgram                    = sym<decltype(t.glLinkProgram)>(g_lib_gles, "glLinkProgram");
    t.glPixelStorei                    = sym<decltype(t.glPixelStorei)>(g_lib_gles, "glPixelStorei");
    t.glPolygonOffset                  = sym<decltype(t.glPolygonOffset)>(g_lib_gles, "glPolygonOffset");
    t.glReadPixels                     = sym<decltype(t.glReadPixels)>(g_lib_gles, "glReadPixels");
    t.glRenderbufferStorage            = sym<decltype(t.glRenderbufferStorage)>(g_lib_gles, "glRenderbufferStorage");
    t.glRenderbufferStorageMultisample = sym<decltype(t.glRenderbufferStorageMultisample)>(g_lib_gles, "glRenderbufferStorageMultisample");
    t.glSampleCoverage                 = sym<decltype(t.glSampleCoverage)>(g_lib_gles, "glSampleCoverage");
    t.glScissor                        = sym<decltype(t.glScissor)>(g_lib_gles, "glScissor");
    t.glShaderBinary                   = sym<decltype(t.glShaderBinary)>(g_lib_gles, "glShaderBinary");
    t.glShaderSource                   = sym<decltype(t.glShaderSource)>(g_lib_gles, "glShaderSource");
    t.glStencilFunc                    = sym<decltype(t.glStencilFunc)>(g_lib_gles, "glStencilFunc");
    t.glStencilFuncSeparate            = sym<decltype(t.glStencilFuncSeparate)>(g_lib_gles, "glStencilFuncSeparate");
    t.glStencilMask                    = sym<decltype(t.glStencilMask)>(g_lib_gles, "glStencilMask");
    t.glStencilMaskSeparate            = sym<decltype(t.glStencilMaskSeparate)>(g_lib_gles, "glStencilMaskSeparate");
    t.glStencilOp                      = sym<decltype(t.glStencilOp)>(g_lib_gles, "glStencilOp");
    t.glStencilOpSeparate              = sym<decltype(t.glStencilOpSeparate)>(g_lib_gles, "glStencilOpSeparate");
    t.glTexImage2D                     = sym<decltype(t.glTexImage2D)>(g_lib_gles, "glTexImage2D");
    t.glTexParameterf                  = sym<decltype(t.glTexParameterf)>(g_lib_gles, "glTexParameterf");
    t.glTexParameterfv                 = sym<decltype(t.glTexParameterfv)>(g_lib_gles, "glTexParameterfv");
    t.glTexParameteri                  = sym<decltype(t.glTexParameteri)>(g_lib_gles, "glTexParameteri");
    t.glTexParameteriv                 = sym<decltype(t.glTexParameteriv)>(g_lib_gles, "glTexParameteriv");
    t.glTexSubImage2D                  = sym<decltype(t.glTexSubImage2D)>(g_lib_gles, "glTexSubImage2D");
    t.glUniform1f                      = sym<decltype(t.glUniform1f)>(g_lib_gles, "glUniform1f");
    t.glUniform1fv                     = sym<decltype(t.glUniform1fv)>(g_lib_gles, "glUniform1fv");
    t.glUniform1i                      = sym<decltype(t.glUniform1i)>(g_lib_gles, "glUniform1i");
    t.glUniform1iv                     = sym<decltype(t.glUniform1iv)>(g_lib_gles, "glUniform1iv");
    t.glUniform2f                      = sym<decltype(t.glUniform2f)>(g_lib_gles, "glUniform2f");
    t.glUniform2fv                     = sym<decltype(t.glUniform2fv)>(g_lib_gles, "glUniform2fv");
    t.glUniform2i                      = sym<decltype(t.glUniform2i)>(g_lib_gles, "glUniform2i");
    t.glUniform2iv                     = sym<decltype(t.glUniform2iv)>(g_lib_gles, "glUniform2iv");
    t.glUniform3f                      = sym<decltype(t.glUniform3f)>(g_lib_gles, "glUniform3f");
    t.glUniform3fv                     = sym<decltype(t.glUniform3fv)>(g_lib_gles, "glUniform3fv");
    t.glUniform3i                      = sym<decltype(t.glUniform3i)>(g_lib_gles, "glUniform3i");
    t.glUniform3iv                     = sym<decltype(t.glUniform3iv)>(g_lib_gles, "glUniform3iv");
    t.glUniform4f                      = sym<decltype(t.glUniform4f)>(g_lib_gles, "glUniform4f");
    t.glUniform4fv                     = sym<decltype(t.glUniform4fv)>(g_lib_gles, "glUniform4fv");
    t.glUniform4i                      = sym<decltype(t.glUniform4i)>(g_lib_gles, "glUniform4i");
    t.glUniform4iv                     = sym<decltype(t.glUniform4iv)>(g_lib_gles, "glUniform4iv");
    t.glUniformMatrix2fv               = sym<decltype(t.glUniformMatrix2fv)>(g_lib_gles, "glUniformMatrix2fv");
    t.glUniformMatrix3fv               = sym<decltype(t.glUniformMatrix3fv)>(g_lib_gles, "glUniformMatrix3fv");
    t.glUniformMatrix4fv               = sym<decltype(t.glUniformMatrix4fv)>(g_lib_gles, "glUniformMatrix4fv");
    t.glUseProgram                     = sym<decltype(t.glUseProgram)>(g_lib_gles, "glUseProgram");
    t.glValidateProgram                = sym<decltype(t.glValidateProgram)>(g_lib_gles, "glValidateProgram");
    t.glVertexAttrib1f                 = sym<decltype(t.glVertexAttrib1f)>(g_lib_gles, "glVertexAttrib1f");
    t.glVertexAttrib1fv                = sym<decltype(t.glVertexAttrib1fv)>(g_lib_gles, "glVertexAttrib1fv");
    t.glVertexAttrib2f                 = sym<decltype(t.glVertexAttrib2f)>(g_lib_gles, "glVertexAttrib2f");
    t.glVertexAttrib2fv                = sym<decltype(t.glVertexAttrib2fv)>(g_lib_gles, "glVertexAttrib2fv");
    t.glVertexAttrib3f                 = sym<decltype(t.glVertexAttrib3f)>(g_lib_gles, "glVertexAttrib3f");
    t.glVertexAttrib3fv                = sym<decltype(t.glVertexAttrib3fv)>(g_lib_gles, "glVertexAttrib3fv");
    t.glVertexAttrib4f                 = sym<decltype(t.glVertexAttrib4f)>(g_lib_gles, "glVertexAttrib4f");
    t.glVertexAttrib4fv                = sym<decltype(t.glVertexAttrib4fv)>(g_lib_gles, "glVertexAttrib4fv");
    t.glVertexAttribPointer            = sym<decltype(t.glVertexAttribPointer)>(g_lib_gles, "glVertexAttribPointer");
    t.glVertexAttribDivisor            = sym<decltype(t.glVertexAttribDivisor)>(g_lib_gles, "glVertexAttribDivisor");
    t.glViewport                       = sym<decltype(t.glViewport)>(g_lib_gles, "glViewport");
    t.loaded = true;

    LOG_I(log::kTagCore, "Loaded %zu GLES3.2 symbols from libGLESv2.so",
          sizeof(GlFunctionTable) / sizeof(void*));

    return load_egl();
}

bool load_egl() noexcept {
    g_lib_egl = ::dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
    if (!g_lib_egl) {
        LOG_E(log::kTagCore, "dlopen(libEGL.so) failed: %s", ::dlerror());
        return false;
    }

    auto& e = g_egl;
    e.eglInitialize            = sym<decltype(e.eglInitialize)>(g_lib_egl, "eglInitialize");
    e.eglTerminate             = sym<decltype(e.eglTerminate)>(g_lib_egl, "eglTerminate");
    e.eglBindAPI               = sym<decltype(e.eglBindAPI)>(g_lib_egl, "eglBindAPI");
    e.eglGetDisplay            = sym<decltype(e.eglGetDisplay)>(g_lib_egl, "eglGetDisplay");
    e.eglGetCurrentDisplay     = sym<decltype(e.eglGetCurrentDisplay)>(g_lib_egl, "eglGetCurrentDisplay");
    e.eglGetCurrentContext     = sym<decltype(e.eglGetCurrentContext)>(g_lib_egl, "eglGetCurrentContext");
    e.eglGetCurrentSurface     = sym<decltype(e.eglGetCurrentSurface)>(g_lib_egl, "eglGetCurrentSurface");
    e.eglMakeCurrent           = sym<decltype(e.eglMakeCurrent)>(g_lib_egl, "eglMakeCurrent");
    e.eglSwapBuffers           = sym<decltype(e.eglSwapBuffers)>(g_lib_egl, "eglSwapBuffers");
    e.eglSwapInterval          = sym<decltype(e.eglSwapInterval)>(g_lib_egl, "eglSwapInterval");
    e.eglGetProcAddress        = sym<decltype(e.eglGetProcAddress)>(g_lib_egl, "eglGetProcAddress");
    e.eglChooseConfig          = sym<decltype(e.eglChooseConfig)>(g_lib_egl, "eglChooseConfig");
    e.eglGetConfigAttrib       = sym<decltype(e.eglGetConfigAttrib)>(g_lib_egl, "eglGetConfigAttrib");
    e.eglCreateWindowSurface   = sym<decltype(e.eglCreateWindowSurface)>(g_lib_egl, "eglCreateWindowSurface");
    e.eglCreatePbufferSurface  = sym<decltype(e.eglCreatePbufferSurface)>(g_lib_egl, "eglCreatePbufferSurface");
    e.eglDestroySurface        = sym<decltype(e.eglDestroySurface)>(g_lib_egl, "eglDestroySurface");
    e.eglCreateContext         = sym<decltype(e.eglCreateContext)>(g_lib_egl, "eglCreateContext");
    e.eglDestroyContext        = sym<decltype(e.eglDestroyContext)>(g_lib_egl, "eglDestroyContext");
    e.eglGetError              = sym<decltype(e.eglGetError)>(g_lib_egl, "eglGetError");
    e.eglQueryContext          = sym<decltype(e.eglQueryContext)>(g_lib_egl, "eglQueryContext");
    e.eglQuerySurface          = sym<decltype(e.eglQuerySurface)>(g_lib_egl, "eglQuerySurface");
    e.eglQueryString           = sym<decltype(e.eglQueryString)>(g_lib_egl, "eglQueryString");
    e.eglWaitClient            = sym<decltype(e.eglWaitClient)>(g_lib_egl, "eglWaitClient");
    e.eglWaitNative            = sym<decltype(e.eglWaitNative)>(g_lib_egl, "eglWaitNative");
    e.loaded = true;

    LOG_I(log::kTagCore, "Loaded EGL symbols from libEGL.so");
    return true;
}

void unload_vendor() noexcept {
    if (g_lib_gles) { ::dlclose(g_lib_gles); g_lib_gles = nullptr; }
    if (g_lib_egl)  { ::dlclose(g_lib_egl);  g_lib_egl  = nullptr; }
    g_loaded = false;
    g_gl = {};
    g_egl = {};
}

}  // namespace vmcr::vendor
