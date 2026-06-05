// ===========================================================================
// src/main/cpp/platform/android_logger.cpp
// Android 平台特定辅助 (当前仅 stub)
// ===========================================================================
#include "vmcr/log.h"

namespace vmcr::platform {

void init_platform() noexcept {
    LOG_I(vmcr::log::kTagCore, "platform init (Android)");
}

}  // namespace vmcr::platform
