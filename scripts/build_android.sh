#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# VMCR 一键构建脚本 (Linux / macOS)
#
# 用法:
#   ./scripts/build_android.sh --abi arm64-v8a --config Release
#   ./scripts/build_android.sh --abi arm64-v8a --config Debug --shaders
#   ./scripts/build_android.sh --clean
# ---------------------------------------------------------------------------
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
OUT_DIR="${PROJECT_ROOT}/out"
ABI="arm64-v8a"
CONFIG="Release"
BUILD_SHADERS=0
CLEAN=0
ASAN=0
TIER="auto"

print_help() {
    sed -n '2,16p' "$0"
    exit "${1:-0}"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --abi)        ABI="$2"; shift 2 ;;
        --config)     CONFIG="$2"; shift 2 ;;
        --tier)       TIER="$2"; shift 2 ;;
        --shaders)    BUILD_SHADERS=1; shift ;;
        --clean)      CLEAN=1; shift ;;
        --asan)       ASAN=1; shift ;;
        -h|--help)    print_help 0 ;;
        *) echo "Unknown arg: $1"; print_help 1 ;;
    esac
done

if [[ -z "${ANDROID_NDK_HOME:-}" ]]; then
    echo "ERROR: ANDROID_NDK_HOME is not set"
    exit 1
fi

if [[ $CLEAN -eq 1 ]]; then
    rm -rf "$BUILD_DIR" "$OUT_DIR"
fi

mkdir -p "$BUILD_DIR/$ABI" "$OUT_DIR/$ABI"

echo "==> Configure"
cmake -G Ninja \
      -S "$PROJECT_ROOT" \
      -B "$BUILD_DIR/$ABI" \
      -DCMAKE_TOOLCHAIN_FILE="$PROJECT_ROOT/cmake/toolchain-ndk-aarch64.cmake" \
      -DCMAKE_BUILD_TYPE="$CONFIG" \
      -DANDROID_ABI="$ABI" \
      -DVMCR_RENDER_TIER="$TIER" \
      $([[ $ASAN -eq 1 ]] && echo "-DVMCR_ENABLE_ASAN=ON") || {
          echo "configure failed"
          exit 1
      }

echo "==> Build"
cmake --build "$BUILD_DIR/$ABI" --parallel "$(nproc 2>/dev/null || echo 4)" || {
    echo "build failed"
    exit 1
}

echo "==> Collect artifacts"
cp -fv "$BUILD_DIR/$ABI/src/main/cpp/loader/libGL.so"        "$OUT_DIR/$ABI/" 2>/dev/null || true
cp -fv "$BUILD_DIR/$ABI/src/main/cpp/vulkan/libvmcr_vk.so"   "$OUT_DIR/$ABI/" 2>/dev/null || true
cp -fv "$BUILD_DIR/$ABI/src/main/cpp/gles/libvmcr_gles.so"   "$OUT_DIR/$ABI/" 2>/dev/null || true
cp -fv "$BUILD_DIR/$ABI/src/main/cpp/jni/libvmcr_jni.so"     "$OUT_DIR/$ABI/" 2>/dev/null || true

echo
echo "Build OK: $OUT_DIR/$ABI"
ls -la "$OUT_DIR/$ABI/" || true
