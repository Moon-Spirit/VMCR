// ===========================================================================
// include/vmcr/backend.h - 渲染后端抽象接口
// ===========================================================================
#pragma once

#include <cstdint>
#include <memory>

// 前置声明避免引入 EGL/Vulkan 头
typedef void* EGLDisplay;
typedef void* EGLContext;
typedef void* EGLSurface;
struct ANativeWindow;

namespace vmcr {

// ---------------------------------------------------------------------------
// 渲染档位 (与 ARCHITECTURE.md / ROADMAP.md 对齐)
// ---------------------------------------------------------------------------
enum class RendererTier : uint32_t {
    Invalid         = 0,
    VulkanFull      = 1,    // Vulkan 1.3 + 关键扩展齐全
    VulkanLimited   = 2,    // Vulkan 1.1/1.2 + 扩展替代
    GLES32          = 3,    // OpenGL ES 3.2 保底
};

const char* tier_to_string(RendererTier t) noexcept;
RendererTier string_to_tier(const char* s) noexcept;

// ---------------------------------------------------------------------------
// 初始化参数 (由 EGL 入口在 MC 创建上下文时填充)
// ---------------------------------------------------------------------------
struct InitParams {
    EGLDisplay      display    = nullptr;
    EGLContext      context    = nullptr;
    EGLSurface      surface    = nullptr;
    ANativeWindow*  window     = nullptr;
    uint32_t        width      = 0;
    uint32_t        height     = 0;
    uint32_t        force_tier = 0;   // 0 = auto
    int             gles_major = 0;
    int             gles_minor = 0;
};

// ---------------------------------------------------------------------------
// 绘制命令 (Phase 3 起使用, Phase 0/1 留空)
// ---------------------------------------------------------------------------
struct DrawCmd {
    uint32_t material_id      = 0;     // 0=solid 1=cutout 2=transparent
    uint32_t index_count      = 0;
    uint32_t instance_count   = 1;
    uint32_t first_index      = 0;
    int32_t  vertex_offset    = 0;
    uint64_t chunk_ssbo_offset = 0;
    uint64_t chunk_ubo_offset  = 0;
};

// ---------------------------------------------------------------------------
// IRenderer - 后端抽象接口
// ---------------------------------------------------------------------------
class IRenderer {
public:
    virtual ~IRenderer() = default;

    // 生命周期
    virtual bool init(const InitParams& params) noexcept            = 0;
    virtual void destroy() noexcept                                = 0;

    // 帧循环
    virtual void begin_frame() noexcept                            = 0;
    virtual void submit(const DrawCmd& cmd) noexcept                = 0;
    virtual void end_frame() noexcept                              = 0;

    // 状态变更
    virtual void on_surface_changed(uint32_t w, uint32_t h) noexcept = 0;

    // 元信息
    virtual const char*  name() const noexcept                      = 0;
    virtual RendererTier tier() const noexcept                      = 0;

    // 统计 (可选)
    virtual uint64_t frame_count() const noexcept                  { return 0; }
    virtual double    last_frame_ms() const noexcept               { return 0.0; }
};

using RendererPtr = std::unique_ptr<IRenderer>;

// ---------------------------------------------------------------------------
// BackendFactory - 通过 dlopen 加载后端
// ---------------------------------------------------------------------------
class BackendFactory {
public:
    // 注册静态后端 (在编译期链接时)
    static void register_static(const char* so_name,
                                RendererTier tier,
                                RendererPtr (*create)()) noexcept;

    // 按档位选择最佳后端, 加载其 .so, 返回 IRenderer*
    // 若 force_tier != Invalid, 则强制使用该档位
    static IRenderer* create(RendererTier force_tier = RendererTier::Invalid) noexcept;
};

}  // namespace vmcr
