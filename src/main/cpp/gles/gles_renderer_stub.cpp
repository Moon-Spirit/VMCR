// ===========================================================================
// src/main/cpp/gles/gles_renderer_stub.cpp
// 主机侧 stub - 不调用 vendor GL, 仅满足链接器
// ===========================================================================
#include "vmcr/backend.h"
#include <memory>

namespace vmcr::gles {

class GlesRendererStub : public vmcr::IRenderer {
public:
    bool init(const vmcr::InitParams&) noexcept override { return true; }
    void destroy() noexcept override {}
    void begin_frame() noexcept override {}
    void submit(const vmcr::DrawCmd&) noexcept override {}
    void end_frame() noexcept override {}
    void on_surface_changed(uint32_t, uint32_t) noexcept override {}
    const char* name() const noexcept override { return "gles-stub"; }
    vmcr::RendererTier tier() const noexcept override { return vmcr::RendererTier::GLES32; }
};

}  // namespace vmcr::gles

extern "C" {
#ifdef _WIN32
__declspec(dllexport)
#endif
void* vmcr_renderer_create() {
    return new vmcr::gles::GlesRendererStub();
}
}  // extern "C"
