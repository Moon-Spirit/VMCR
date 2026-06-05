package io.anomalyco.vmcr.jni;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.nio.ByteBuffer;

/**
 * JNI 桥 - Phase 3 完整实现
 *
 * 当前 stub: 提供 Native.loadLibrary + DirectByteBuffer 持有
 */
public final class NativeBridge {
    private static final Logger LOGGER = LoggerFactory.getLogger("VMCR-JNI");
    public static final int CHUNK_BUFFER_SIZE = 64 * 1024 * 1024;  // 64 MiB

    private static volatile boolean loaded = false;
    private final ByteBuffer chunkBuffer;
    private long nativeHandle = 0L;

    public NativeBridge() {
        chunkBuffer = ByteBuffer.allocateDirect(CHUNK_BUFFER_SIZE);
    }

    public static synchronized boolean load() {
        if (loaded) return true;
        try {
            System.loadLibrary("vmcr_jni");
            loaded = true;
            LOGGER.info("[VMCR] libvmcr_jni.so loaded");
        } catch (UnsatisfiedLinkError e) {
            LOGGER.error("[VMCR] failed to load libvmcr_jni.so: {}", e.getMessage());
            return false;
        }
        return true;
    }

    public synchronized void bind() {
        if (!load()) return;
        nativeHandle = nAttachDirectBuffer(chunkBuffer);
        LOGGER.info("[VMCR] Native bridge bound, handle=0x{}", Long.toHexString(nativeHandle));
    }

    public void submitChunk(int bytes) {
        chunkBuffer.flip();
        nSubmitChunk(nativeHandle, bytes);
        chunkBuffer.clear();
    }

    public ByteBuffer buffer() { return chunkBuffer; }

    // ---- JNI 入口 (Phase 3 完整实现) ----
    private native long   nAttachDirectBuffer(ByteBuffer buf);
    private native void   nSubmitChunk(long handle, int bytes);
}
