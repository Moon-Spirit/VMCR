package io.anomalyco.vmcr;

import net.fabricmc.api.ClientModInitializer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * VMCR Fabric Mod 入口 (Phase 0/1 stub).
 *
 * Phase 3 将注册:
 *   - ChunkInterceptor (拦截 LevelRenderer.renderChunkLayer)
 *   - BufferBuilderInterceptor (拦截 putVertex / endVertex)
 *   - NativeBridge.bind() (AttachDirectBuffer 零拷贝)
 */
public final class VMCRMixin implements ClientModInitializer {
    public static final String MOD_ID = "vmcr";
    public static final Logger LOGGER = LoggerFactory.getLogger("VMCR-Mod");

    @Override
    public void onInitializeClient() {
        LOGGER.info("[VMCR] Mod initializing (Phase 0/1 stub)");
        LOGGER.info("[VMCR] JNI bridge: Phase 3 will register");
    }
}
