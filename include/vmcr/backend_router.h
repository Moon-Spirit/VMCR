// ===========================================================================
// include/vmcr/backend_router.h
// BackendRouter 单例 - 持有当前 IRenderer*, 线程安全切换
// ===========================================================================
#pragma once

#include "vmcr/backend.h"

#include <atomic>
#include <memory>
#include <mutex>

namespace vmcr {

class BackendRouter {
public:
    static BackendRouter& instance() noexcept;

    // 初始化 (由 JNI_OnLoad 调用一次)
    // force_tier: 0 = auto 探测, 否则强制
    bool initialize(RendererTier force_tier = RendererTier::Invalid) noexcept;

    // 切换后端 (会销毁旧后端, 安装新后端)
    // 调用前应暂停所有 GL 调用线程
    void set_backend(RendererPtr b) noexcept;

    // 关闭
    void shutdown() noexcept;

    // Hot path - 无锁
    IRenderer* current() const noexcept { return current_.load(std::memory_order_acquire); }

    // 路由提交
    void submit(const DrawCmd& cmd) noexcept {
        if (auto* r = current()) {
            r->submit(cmd);
        }
    }

    // 元信息
    RendererTier tier() const noexcept { return tier_.load(std::memory_order_acquire); }
    const char*   tier_name() const noexcept;

private:
    BackendRouter() = default;
    ~BackendRouter() = default;
    BackendRouter(const BackendRouter&) = delete;
    BackendRouter& operator=(const BackendRouter&) = delete;

    std::atomic<IRenderer*>  current_{nullptr};
    std::atomic<RendererTier> tier_{RendererTier::Invalid};
    std::mutex                switch_mtx_;
};

}  // namespace vmcr
