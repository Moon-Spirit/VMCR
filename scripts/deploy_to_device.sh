#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# VMCR 部署脚本
# 用法: ./scripts/deploy_to_device.sh [serial]
# ---------------------------------------------------------------------------
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ABI="arm64-v8a"
OUT_DIR="${PROJECT_ROOT}/out/${ABI}"

PKG="com.tungsten.fcl"
PLUGIN_DIR="/sdcard/Android/data/${PKG}/files/.minecraft/custom_renderer/VMCR"
MODS_DIR="/sdcard/Android/data/${PKG}/files/.minecraft/mods"

ADB="adb"
if [[ $# -ge 1 ]]; then
    ADB="adb -s $1"
fi

echo "==> Deploying VMCR to $PKG"
echo "    Plugin dir: $PLUGIN_DIR"

$ADB shell mkdir -p "$PLUGIN_DIR" 2>/dev/null || true
$ADB shell mkdir -p "$MODS_DIR" 2>/dev/null || true

# Native libs
for f in libGL.so libvmcr_vk.so libvmcr_gles.so libvmcr_jni.so; do
    if [[ -f "$OUT_DIR/$f" ]]; then
        echo "  + $f"
        $ADB push "$OUT_DIR/$f" "$PLUGIN_DIR/$f" >/dev/null
    fi
done

# Configs
$ADB push "$PROJECT_ROOT/configs/custom_renderer.json" "$PLUGIN_DIR/"  >/dev/null
$ADB push "$PROJECT_ROOT/configs/vulkan_features.json" "$PLUGIN_DIR/configs/" >/dev/null 2>&1 || true

# Mod (if built)
mod_jar=$(ls "$OUT_DIR/VMCR-fabric-"*.jar 2>/dev/null || true)
if [[ -n "$mod_jar" ]]; then
    echo "  + $mod_jar"
    $ADB push "$mod_jar" "$MODS_DIR/" >/dev/null
fi

echo "==> Restart MC"
$ADB shell am force-stop "$PKG" 2>/dev/null || true
echo "Deployed. Start MC and select VMCR in FCL settings."
