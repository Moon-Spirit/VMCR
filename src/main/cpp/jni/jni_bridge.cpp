// ===========================================================================
// src/main/cpp/jni/jni_bridge.cpp - JNI 桥 (Phase 3 实现)
// 当前为 stub
// ===========================================================================
#include "vmcr/log.h"
#include <jni.h>

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    LOG_I(vmcr::log::kTagJNI, "[JNI] OnLoad (stub for Phase 0/1)");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* /*vm*/, void* /*reserved*/) {
    LOG_I(vmcr::log::kTagJNI, "[JNI] OnUnload");
}

}  // extern "C"
