# ===========================================================================
# scripts/compile_shaders.sh - 编译 GLSL → SPIR-V
# 用法: ./scripts/compile_shaders.sh [src_dir] [dst_dir] [target_env]
# ===========================================================================
#!/usr/bin/env bash
set -euo pipefail

SRC="${1:-src/shaders}"
DST="${2:-build/arm64/shaders}"
TARGET="${3:-vulkan1.3}"

if ! command -v glslangValidator >/dev/null 2>&1; then
    echo "ERROR: glslangValidator not found. Install: apt install glslang-tools"
    exit 1
fi

# 归一化路径 (转 /)
SRC_NORM=$(echo "$SRC" | tr '\\' '/')
DST_NORM=$(echo "$DST" | tr '\\' '/')

mkdir -p "$DST_NORM"

count=0
errors=0

while IFS= read -r f; do
    # 归一化
    f_norm=$(echo "$f" | tr '\\' '/')
    rel="${f_norm#${SRC_NORM}/}"
    out="$DST_NORM/${rel}.spv"
    mkdir -p "$(dirname "$out")"

    if glslangValidator -V --target-env "$TARGET" "$f" -o "$out" 2>/dev/null; then
        if command -v spirv-opt >/dev/null 2>&1; then
            spirv-opt -O --target-env="$TARGET" "$out" -o "$out" 2>/dev/null || true
        fi
        echo "  OK  $rel"
        count=$((count+1))
    else
        echo "  FAIL $rel"
        glslangValidator -V --target-env "$TARGET" "$f" -o "$out" 2>&1 | head -5
        errors=$((errors+1))
    fi
done < <(find "$SRC" -type f \( -name '*.vert' -o -name '*.frag' -o -name '*.comp' \))

echo "==> $count shaders compiled, $errors errors"
exit $errors
