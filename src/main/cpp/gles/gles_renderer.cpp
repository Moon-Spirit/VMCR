// ===========================================================================
// src/main/cpp/gles/gles_renderer.cpp - GLES 3.2 保底后端 (Phase 1)
// Phase 0 阶段: 不实现 GL 状态机缓存, 所有调用 1:1 转发到原厂
// ===========================================================================
#include "vmcr/backend.h"
#include "vmcr/log.h"
#include "vmcr/vendor_gl.h"
#include "vmcr/vendor_egl.h"

namespace vmcr::gles {

constexpr const char* kTag = log::kTagGL;

class GlesRenderer : public vmcr::IRenderer {
public:
    bool init(const vmcr::InitParams& params) noexcept override {
        params_ = params;

        if (!vmcr::vendor::g_gl.loaded) {
            LOG_E(kTag, "vendor GL not loaded");
            return false;
        }

        // 不接管 eglMakeCurrent, 仅记录当前 GL 版本
        // mc 通过 GL_NUM_COMPRESSED_TEXTURE_FORMATS 等查询版本
        GLint major = 0, minor = 0;
        if (vmcr::vendor::g_gl.glGetIntegerv) {
            vmcr::vendor::g_gl.glGetIntegerv(0x3000 /* GL_MAJOR_VERSION */, &major);
            vmcr::vendor::g_gl.glGetIntegerv(0x3001 /* GL_MINOR_VERSION */, &minor);
        }
        LOG_I(kTag, "[GLES] init OK, vendor GLES %d.%d", major, minor);
        return true;
    }

    void destroy() noexcept override {
        LOG_I(kTag, "[GLES] destroy");
    }

    void begin_frame() noexcept override {
        // no-op: vendor driver already tracking state
    }

    void submit(const vmcr::DrawCmd& cmd) noexcept override {
        // Phase 1: 不实现, vendor driver 自行处理
        (void)cmd;
    }

    void end_frame() noexcept override {
        // no-op: vendor driver 自行 swap
    }

    void on_surface_changed(uint32_t w, uint32_t h) noexcept override {
        if (vmcr::vendor::g_gl.glViewport) {
            vmcr::vendor::g_gl.glViewport(0, 0, (GLint)w, (GLint)h);
        }
    }

    const char* name() const noexcept override { return "gles-forwarder"; }
    vmcr::RendererTier tier() const noexcept override { return vmcr::RendererTier::GLES32; }

private:
    vmcr::InitParams params_{};
};

}  // namespace vmcr::gles

// 工厂入口
extern "C" {
#ifdef _WIN32
__declspec(dllexport)
#endif
void* vmcr_renderer_create() {
    return new vmcr::gles::GlesRenderer();
}
}  // extern "C"
