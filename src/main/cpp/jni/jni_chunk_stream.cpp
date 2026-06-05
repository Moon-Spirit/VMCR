// ===========================================================================
// src/main/cpp/jni/jni_chunk_stream.cpp - Chunk Mesh 流接收 (Phase 3)
// 当前为 stub
// ===========================================================================
#include "vmcr/log.h"
#include <jni.h>

namespace {

constexpr const char* kTag = vmcr::log::kTagJNI;

}  // namespace

extern "C" {

JNIEXPORT jlong JNICALL
Java_io_anomalyco_vmcr_jni_NativeBridge_nAttachDirectBuffer(
    JNIEnv* /*env*/, jclass /*clazz*/, jobject /*buf*/) {
    LOG_D(kTag, "nAttachDirectBuffer (stub)");
    return 0;
}

JNIEXPORT void JNICALL
Java_io_anomalyco_vmcr_jni_NativeBridge_nSubmitChunk(
    JNIEnv* /*env*/, jclass /*clazz*/, jlong /*handle*/, jint /*bytes*/) {
    LOG_D(kTag, "nSubmitChunk (stub)");
}

}  // extern "C"
