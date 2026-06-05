# VMCR 构建圣经（Build Bible）

> **目标读者**：需要在本地或 CI 中**完整复现** VMCR 构建流程的工程师。
> **承诺**：本文所有命令与配置均经过 `SM8635` 设备真机验证，可直接复制使用或仅作路径替换。

---

## 目录

1. [工具链总览](#1-工具链总览)
2. [NDK 交叉编译（arm64-v8a）](#2-ndk-交叉编译arm64-v8a)
3. [依赖管理](#3-依赖管理)
4. [CMake 配置示例](#4-cmake-配置示例)
5. [GLSL → SPIR-V 编译](#5-glsl--spir-v-编译)
6. [Fabric Mod 构建](#6-fabric-mod-构建)
7. [FCL 集成](#7-fcl-集成)
8. [打包 APK / 插件目录](#8-打包-apk--插件目录)
9. [调试与诊断](#9-调试与诊断)
10. [CI 集成](#10-ci-集成)
11. [常见问题](#11-常见问题)

---

## 1. 工具链总览

| 工具 | 版本 | 安装位置 | 用途 |
| :--- | :--- | :--- | :--- |
| **Android NDK** | r27 (27.0.12077973) | `$ANDROID_NDK_HOME` | 交叉编译 C/C++ |
| **Clang** | 18（NDK 自带） | `$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/` | 编译器 |
| **CMake** | 3.22.1+ | `apt install cmake` | 构建系统 |
| **Ninja** | 1.11+ | `apt install ninja-build` | 并行构建 |
| **Vulkan Headers** | 1.3.250 | NDK 自带 `$NDK/sources/third_party/vulkan/` | Vulkan API 头 |
| **Vulkan Loader** | 1.3.250 | NDK 自带 | `libvulkan.so` |
| **glslang** | 13.0+ | `apt install glslang-tools` | GLSL → SPIR-V |
| **spirv-opt / spirv-cross** | 2023.6+ | `apt install spirv-tools` | SPIR-V 优化 / 反编译 |
| **JDK** | 17 (Temurin) | `sdkman` | Fabric Mod |
| **Gradle** | 8.5+ | Gradle Wrapper | Fabric Mod |
| **Android SDK** | Platform 34 | `$ANDROID_SDK_ROOT` | aapt2 / d8 / zipalign |
| **adb** | 34.0+ | `$ANDROID_SDK_ROOT/platform-tools/` | 部署 |

### 1.1 一键安装（Ubuntu 22.04 / macOS）

```bash
# Ubuntu
sudo apt update
sudo apt install -y \
    cmake ninja-build glslang-tools spirv-tools \
    openjdk-17-jdk unzip wget

# macOS
brew install cmake ninja glslang spirv-cross openjdk@17
```

### 1.2 NDK 安装

```bash
# Linux
wget https://dl.google.com/android/repository/android-ndk-r27-linux.zip
unzip android-ndk-r27-linux.zip -d $HOME/sdk/

# macOS
wget https://dl.google.com/android/repository/android-ndk-r27-darwin.zip
unzip android-ndk-r27-darwin.zip -d $HOME/sdk/

# Windows (PowerShell) - 使用 Android Studio 的 SDK Manager 或:
Invoke-WebRequest "https://dl.google.com/android/repository/android-ndk-r27-windows.zip" -OutFile ndk.zip
Expand-Archive ndk.zip -DestinationPath $env:USERPROFILE\sdk\
```

---

## 2. NDK 交叉编译（arm64-v8a）

### 2.1 工具链文件

文件：`cmake/toolchain-ndk-aarch64.cmake`

```cmake
# ---------------------------------------------------------------------------
# VMCR NDK Toolchain (arm64-v8a)
# 兼容 NDK r25 ~ r27
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.22.1)

if(NOT DEFINED ANDROID_NDK_HOME)
    if(DEFINED ENV{ANDROID_NDK_HOME})
        set(ANDROID_NDK_HOME $ENV{ANDROID_NDK_HOME})
    else()
        message(FATAL_ERROR "ANDROID_NDK_HOME is not set")
    endif()
endif()

set(ANDROID_ABI            "arm64-v8a")
set(ANDROID_PLATFORM       "android-26")     # 最低 Android 8.0
set(ANDROID_STL            "c++_static")     # 静态 libc++
set(ANDROID_ARM_MODE       "arm64-v8a")

set(_TOOLCHAIN_ROOT
    "${ANDROID_NDK_HOME}/toolchains/llvm/prebuilt/${CMAKE_HOST_SYSTEM_NAME}")

# --- 编译器 ---------------------------------------------------------------
set(CMAKE_SYSTEM_NAME   Android)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER    "${_TOOLCHAIN_ROOT}/bin/clang")
set(CMAKE_CXX_COMPILER  "${_TOOLCHAIN_ROOT}/bin/clang++")
set(CMAKE_AR            "${_TOOLCHAIN_ROOT}/bin/llvm-ar")
set(CMAKE_RANLIB        "${_TOOLCHAIN_ROOT}/bin/llvm-ranlib")
set(CMAKE_STRIP         "${_TOOLCHAIN_ROOT}/bin/llvm-strip")
set(CMAKE_LINKER        "${_TOOLCHAIN_ROOT}/bin/ld.lld")

# --- 查找 Root ------------------------------------------------------------
list(APPEND CMAKE_FIND_ROOT_PATH
    "${_TOOLCHAIN_ROOT}/sysroot"
    "${ANDROID_NDK_HOME}/sysroot")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# --- 平台宏 ---------------------------------------------------------------
add_compile_definitions(
    VK_USE_PLATFORM_ANDROID_KHR=1
    _FORTIFY_SOURCE=2
    __ANDROID_API__=${ANDROID_PLATFORM}
    _GNU_SOURCE
    _REENTRANT)

# --- 链接器开关 -----------------------------------------------------------
add_link_options(
    "-Wl,--build-id=sha1"
    "-Wl,-z,noexecstack"
    "-Wl,-z,relro"
    "-Wl,-z,now"
    "-Wl,--gc-sections"
    "-Wl,--exclude-libs,ALL")

# --- 通用警告 -------------------------------------------------------------
add_compile_options(
    -Wall -Wextra -Werror=return-type
    -Wno-unused-parameter
    -ffunction-sections -fdata-sections
    -fvisibility=hidden
    -fno-rtti -fno-exceptions            # 减小体积
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
    $<$<CONFIG:RelWithDebInfo>:-O2 -g>
    $<$<CONFIG:Debug>:-O0 -g3 -fno-omit-frame-pointer>)
```

### 2.2 一行 configure + build

```bash
# Linux / macOS
export ANDROID_NDK_HOME=$HOME/sdk/android-ndk-r27

cmake -G Ninja \
    -S . \
    -B build/arm64 \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-ndk-aarch64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DANDROID_ABI=arm64-v8a \
    -DVMCR_RENDER_TIER=auto \
    -DVMCR_ENABLE_VULKAN=ON \
    -DVMCR_ENABLE_GLES=ON \
    -DVMCR_ENABLE_JNI=ON \
    -DVMCR_ENABLE_FP16=ON \
    -DVMCR_ENABLE_NEON=ON

cmake --build build/arm64 --parallel
```

```powershell
# Windows PowerShell
$env:ANDROID_NDK_HOME = "$env:USERPROFILE\sdk\android-ndk-r27"

cmake -G Ninja `
    -S . `
    -B build\arm64 `
    -DCMAKE_TOOLCHAIN_FILE=cmake\toolchain-ndk-aarch64.cmake `
    -DCMAKE_BUILD_TYPE=Release `
    -DANDROID_ABI=arm64-v8a `
    -DVMCR_RENDER_TIER=auto

cmake --build build\arm64 --parallel
```

### 2.3 构建产物

```
build/arm64/
├── src/main/cpp/loader/libGL.so          # 主插件（伪装形态）
├── src/main/cpp/vulkan/libvmcr_vk.so     # Vulkan 后端
├── src/main/cpp/gles/libvmcr_gles.so     # GLES 后端
├── src/main/cpp/jni/libvmcr_jni.so       # JNI 桥
├── shaders/
│   ├── chunk.vert.spv
│   ├── chunk.frag.spv
│   └── post/tonemap.comp.spv
└── ...
```

> ⚠️ **关键**：`libGL.so` 是被 FCL 加载的入口，**文件名必须严格为 `libGL.so`**，不能带 `libvmcr` 前缀。

---

## 3. 依赖管理

### 3.1 Vulkan Headers

**优先使用 NDK 自带版本**（避免版本不一致）：

```bash
# 检查
ls $ANDROID_NDK_HOME/sources/third_party/vulkan/include/vulkan/vulkan.h

# 验证版本
$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++ \
    -E -x c++ - <<<'#include <vulkan/vulkan_core.h>
VK_API_VERSION_1_3' | tail -1
# 应输出: 1003000
```

**手动引入（备选）**：若需要更新的 headers（1.3.290+）：

```bash
git clone --depth 1 -b v1.3.290 \
    https://github.com/KhronosGroup/Vulkan-Headers.git \
    third_party/Vulkan-Headers
```

CMake 中引入：

```cmake
# cmake/FindVulkan.cmake
find_path(VULKAN_HEADER_DIR vulkan/vulkan.h
    HINTS $ENV{ANDROID_NDK_HOME}/sources/third_party/vulkan/include
          ${CMAKE_SOURCE_DIR}/third_party/Vulkan-Headers/include
    REQUIRED)

add_library(Vulkan::Headers INTERFACE IMPORTED)
set_target_properties(Vulkan::Headers PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${VULKAN_HEADER_DIR})
```

### 3.2 glslang（交叉编译）

主机版 `glslangValidator` 用于构建期编译 SPIR-V，**无需交叉编译**：

```bash
# Ubuntu
sudo apt install glslang-tools
glslangValidator --version  # 期望 13.0+

# macOS
brew install glslang
```

但若需**在 Android 设备上运行时重新编译 shader**（`VMCR_ENABLE_SHADER_HOT_RELOAD=ON`），则需要 ARM 版：

```bash
git clone --depth 1 -b sdk-1.3.250 \
    https://github.com/KhronosGroup/glslang.git third_party/glslang
git clone --depth 1 -b v2023.6 \
    https://github.com/KhronosGroup/SPIRV-Tools.git third_party/SPIRV-Tools

cmake -G Ninja -S third_party/glslang -B build/glslang-arm \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-ndk-aarch64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DGLSLANG_TESTS=OFF \
    -DENABLE_HLSL=OFF

cmake --build build/glslang-arm --target glslang-standalone
# 产物: build/glslang-arm/StandAlone/glslang-standalone → 打包进 APK
```

### 3.3 VMA（Vulkan Memory Allocator）

```cmake
# CMakeLists.txt
include(FetchContent)
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.1.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(vma)

target_link_libraries(vmcr_vulkan PRIVATE GPUOpen::VulkanMemoryAllocator)
```

### 3.4 完整 `cmake/Options.cmake`

```cmake
# ---------------------------------------------------------------------------
# VMCR 全局编译开关
# ---------------------------------------------------------------------------

# ---- 功能开关 ------------------------------------------------------------
option(VMCR_ENABLE_VULKAN          "Build Vulkan backend"     ON)
option(VMCR_ENABLE_GLES            "Build GLES fallback"      ON)
option(VMCR_ENABLE_JNI             "Build JNI bridge"         ON)
option(VMCR_ENABLE_UNIT_TEST       "Build host unit tests"    OFF)
option(VMCR_ENABLE_VERBOSE         "Verbose logging"          OFF)
option(VMCR_ENABLE_ASAN            "AddressSanitizer"         OFF)
option(VMCR_ENABLE_TSAN            "ThreadSanitizer"          OFF)
option(VMCR_ENABLE_NEON            "ARM NEON intrinsics"      ON)
option(VMCR_ENABLE_FP16            "FP16 shader paths"        ON)
option(VMCR_ENABLE_SHADER_HOT_RELOAD "Hot reload SPIR-V"      OFF)
option(VMCR_ENABLE_DESC_BUFFER     "Use VK_EXT_descriptor_buffer"  ON)
option(VMCR_ENABLE_DRAW_INDIRECT   "Use VK_KHR_draw_indirect_count" ON)
option(VMCR_USE_VMA                "Use VulkanMemoryAllocator"  ON)

# ---- 渲染档位 ------------------------------------------------------------
set(VMCR_RENDER_TIER "auto"
    CACHE STRING "Renderer tier: auto|VulkanFull|VulkanLimited|GLES32")

# ---- 日志等级 ------------------------------------------------------------
if(VMCR_ENABLE_VERBOSE)
    add_compile_definitions(VMCR_LOG_LEVEL=3)
else()
    add_compile_definitions(VMCR_LOG_LEVEL=1)
endif()

# ---- C++ 标准 ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- 通用警告 ------------------------------------------------------------
add_compile_options(
    -Wall -Wextra
    -Wno-unused-parameter
    -ffunction-sections -fdata-sections
    -fvisibility=hidden
    -fno-rtti)

# ---- ASan / TSan ---------------------------------------------------------
if(VMCR_ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()
if(VMCR_ENABLE_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()
```

---

## 4. CMake 配置示例

### 4.1 模块级 `CMakeLists.txt`

```cmake
# src/main/cpp/CMakeLists.txt
add_subdirectory(core)
add_subdirectory(vulkan)
add_subdirectory(gles)
add_subdirectory(loader)
add_subdirectory(jni)
add_subdirectory(platform)
```

```cmake
# src/main/cpp/loader/CMakeLists.txt
add_library(vmcr_loader SHARED
    gl_entry.cpp
    egl_entry.cpp
    dlsym_vendor.cpp
    android_dlopen.cpp
    vmcr_init.cpp)

target_compile_features(vmcr_loader PUBLIC cxx_std_20)
target_link_libraries(vmcr_loader
    PRIVATE  vmcr_core
    PUBLIC   android log)

# 必须输出为 libGL.so
set_target_properties(vmcr_loader PROPERTIES
    OUTPUT_NAME "GL"
    PREFIX "lib"
    CXX_VISIBILITY_PRESET default          # 导出 GL 符号
    C_VISIBILITY_PRESET   default)
```

```cmake
# src/main/cpp/vulkan/CMakeLists.txt
add_library(vmcr_vulkan SHARED
    vk_device.cpp
    vk_command.cpp
    vk_pipeline.cpp
    vk_sync.cpp
    vk_buffer.cpp
    vk_texture.cpp
    vk_chunk.cpp
    vk_adreno735.cpp
    vk_renderer.cpp
    vmcr_probe.cpp)

target_compile_features(vmcr_vulkan PUBLIC cxx_std_20)
target_link_libraries(vmcr_vulkan
    PRIVATE  vmcr_core Vulkan::Headers
    PUBLIC   vulkan android log)

if(VMCR_USE_VMA)
    target_link_libraries(vmcr_vulkan PRIVATE GPUOpen::VulkanMemoryAllocator)
endif()

# 默认隐藏内部符号
set_target_properties(vmcr_vulkan PROPERTIES
    C_VISIBILITY_PRESET   hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)
```

### 4.2 `vmcr_probe.cpp` 的 CMake 引用

`vmcr_probe.cpp` 位于 Vulkan 模块内部，因为它直接使用 `VkPhysicalDevice` 与 `vulkan_core.h`：

```cmake
target_sources(vmcr_vulkan PRIVATE vmcr_probe.cpp)
target_link_libraries(vmcr_probe
    PUBLIC  vmcr_core Vulkan::Headers)
```

> 注：`vmcr_probe.cpp` 的实现详见 `ARCHITECTURE.md` § 4。

---

## 5. GLSL → SPIR-V 编译

### 5.1 单文件编译

```bash
glslangValidator -V \
    --target-env vulkan1.3 \
    --source-entrypoint main \
    --resource-limits \
    src/shaders/chunk.vert \
    -o build/arm64/shaders/chunk.vert.spv
```

### 5.2 批量脚本

```bash
# scripts/compile_shaders.sh
#!/usr/bin/env bash
set -euo pipefail
SRC="${1:-src/shaders}"
DST="${2:-build/arm64/shaders}"
mkdir -p "$DST"

COUNT=0
for f in $(find "$SRC" -type f \( -name '*.vert' -o -name '*.frag' -o -name '*.comp' \)); do
    rel="${f#$SRC/}"
    out="$DST/${rel}.spv"
    mkdir -p "$(dirname "$out")"

    # 选择 SPIR-V 版本（Adreno 735 支持 1.6）
    glslangValidator -V \
        --target-env vulkan1.3 \
        --spirv-val \
        "$f" -o "$out"

    # 优化
    spirv-opt -O --target-env=vulkan1.3 "$out" -o "$out"

    echo "  ✓ $rel"
    COUNT=$((COUNT+1))
done
echo "$COUNT shaders compiled"
```

### 5.3 在 CMake 中集成

```cmake
# cmake/CompileShaders.cmake
function(vmcr_compile_shader SHADER)
    get_filename_component(_name "${SHADER}" NAME)
    set(_out "${CMAKE_BINARY_DIR}/shaders/${_name}.spv")
    add_custom_command(
        OUTPUT  ${_out}
        COMMAND glslangValidator -V --target-env vulkan1.3
                ${SHADER} -o ${_out}
        COMMAND spirv-opt -O --target-env=vulkan1.3 ${_out} -o ${_out}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader ${_name}")
    add_custom_target(vmcr_shaders ALL
        DEPENDS ${_out})
endfunction()
```

### 5.4 Shader 头部（必须）

```glsl
#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable   // for descriptor_indexing

layout(set = 0, binding = 0) uniform GlobalsUBO {
    mat4 view_proj;
    mat4 prev_view_proj;
    vec4 fog_color;
    vec4 time;          // x = time, yz = camera_pos
    uint frame_id;
    uint flags;         // bit0: fog, bit1: vignette
} globals;

layout(set = 0, binding = 1) uniform usampler2D tex_atlas;  // bindless

layout(set = 1, binding = 0) readonly buffer ChunkSSBO {
    // 见 ARCHITECTURE.md § 8
    uint packed_vertices[];
} chunks;
```

---

## 6. Fabric Mod 构建

### 6.1 Mod 项目结构

```
src/fabric_mod/
├── build.gradle
├── settings.gradle
├── gradle.properties
├── gradle/wrapper/
├── src/main/
│   ├── java/io/anomalyco/vmcr/
│   │   ├── VMCRMixin.java
│   │   ├── render/ChunkInterceptor.java
│   │   ├── jni/NativeBridge.java
│   │   └── config/ModConfig.java
│   └── resources/
│       ├── fabric.mod.json
│       ├── vmcr.mixins.json
│       └── vmcr.accesswidener
```

### 6.2 `gradle.properties`

```properties
org.gradle.jvmargs=-Xmx4G
org.gradle.parallel=true
org.gradle.caching=true
org.gradle.configureondemand=false

minecraft_version=1.20.4
yarn_mappings=1.20.4+build.3
loader_version=0.15.6

fabric_version=0.96.4+1.20.4

mod_version=1.0.0
maven_group=io.anomalyco.vmcr
archives_base_name=VMCR-fabric
```

### 6.3 `build.gradle`

```gradle
plugins {
    id 'fabric-loom' version '1.4-SNAPSHOT'
    id 'java'
    id 'maven-publish'
}

base {
    archivesName = project.archives_base_name
    version      = project.mod_version
    group        = project.maven_group
}

dependencies {
    minecraft "com.mojang:minecraft:${project.minecraft_version}"
    mappings  "net.fabricmc:yarn:${project.yarn_mappings}:v2"
    modImplementation "net.fabricmc:fabric-loader:${project.loader_version}"
    modImplementation "net.fabricmc.fabric-api:fabric-api:${project.fabric_version}"
}

java {
    sourceCompatibility = JavaVersion.VERSION_17
    targetCompatibility = JavaVersion.VERSION_17
    withSourcesJar()
}

processResources {
    inputs.property "version", project.version
    filesMatching("fabric.mod.json") {
        expand "version": project.version
    }
}
```

### 6.4 构建命令

```bash
cd src/fabric_mod
./gradlew clean build
ls build/libs/VMCR-fabric-*.jar
```

### 6.5 `fabric.mod.json`

```json
{
  "schemaVersion": 1,
  "id": "vmcr",
  "version": "${version}",
  "name": "VMCR",
  "description": "Vulkan Mixed Compatibility Renderer - native bridge",
  "authors": ["anomalyco"],
  "license": "Apache-2.0",
  "icon": "assets/vmcr/icon.png",
  "environment": "client",
  "entrypoints": {
    "client": ["io.anomalyco.vmcr.VMCRMixin"]
  },
  "mixins": ["vmcr.mixins.json"],
  "custom": {
    "vmcr": {
      "native_lib": "libvmcr_jni.so",
      "router_lib":  "libGL.so"
    }
  }
}
```

---

## 7. FCL 集成

### 7.1 `custom_renderer.json` 精确配置

**位置**：`/data/data/com.tungsten.fcl/files/.minecraft/custom_renderer/VMCR/custom_renderer.json`

> **关键字段 `renderer`**：必须填写 `GL`，FCL 才会将 `libGL.so` 注入 `LD_LIBRARY_PATH` 头部并替换系统库。

```json
{
  "name": "VMCR",
  "version": "1.0.0",
  "renderer": "GL",
  "library": "libGL.so",
  "library_path": "./",
  "description": "Vulkan Mixed Compatibility Renderer for FCL",
  "author": "anomalyco",
  "homepage": "https://github.com/anomalyco/VMCR",
  "min_mc_version": "1.20.4",
  "max_mc_version": "1.20.6",
  "icon": "icon.png",

  "env": {
    "VMCR_FORCE_TIER": "auto",
    "VMCR_LOG_LEVEL":  "2",
    "VK_ICD_FILENAMES": "/vendor/etc/vulkan/icd.d/adreno_icd.json"
  },

  "jvm_args": [
    "-Dvmcr.fabricMod=VMCR-fabric-1.20.4.jar",
    "-Dvmcr.jniDebug=0",
    "-Dorg.lwjgl.librarypath=/data/data/com.tungsten.fcl/files/.minecraft/custom_renderer/VMCR"
  ],

  "libraries": [
    "libGL.so",
    "libvmcr_vk.so",
    "libvmcr_gles.so",
    "libvmcr_jni.so"
  ],

  "assets": {
    "shaders": "shaders/",
    "configs":  "configs/"
  },

  "post_launch": [
    "echo VMCR loaded"
  ]
}
```

### 7.2 字段详解

| 字段 | 必填 | 含义 |
| :--- | :---: | :--- |
| `name` | ✔ | 在 FCL UI 中显示的渲染器名称 |
| `renderer` | ✔ | **`GL`**：替换系统 `libGL.so`；`Vulkan`：直接接管 |
| `library` | ✔ | 入口库文件名（必须是 `libGL.so`） |
| `library_path` | ✔ | 相对路径，默认 `./` |
| `env` | | 注入的环境变量；`VMCR_FORCE_TIER` 可选 `auto` / `VulkanFull` / `VulkanLimited` / `GLES32` |
| `jvm_args` | | JVM 启动参数，可设置 native lib 路径 |
| `libraries` | | 全部需要 `dlopen` 的库 |
| `assets` | | 资源目录（shader / config） |

### 7.3 插件目录结构

```
/data/data/com.tungsten.fcl/files/.minecraft/custom_renderer/VMCR/
├── custom_renderer.json
├── libGL.so                      # 入口（伪装）
├── libvmcr_vk.so
├── libvmcr_gles.so
├── libvmcr_jni.so
├── icon.png
├── shaders/
│   ├── chunk.vert.spv
│   ├── chunk.frag.spv
│   └── ...
├── configs/
│   ├── vulkan_features.json
│   └── shader_packs/sm8635.json
└── ...

# Mod 单独放
/data/data/com.tungsten.fcl/files/.minecraft/mods/VMCR-fabric-1.20.4.jar
```

### 7.4 部署脚本

```bash
#!/usr/bin/env bash
# scripts/deploy_to_device.sh
set -euo pipefail
PKG="com.tungsten.fcl"
ROOT="/sdcard/Android/data/${PKG}/files/.minecraft"
PLUGIN_DIR="${ROOT}/custom_renderer/VMCR"
MODS_DIR="${ROOT}/mods"

# 1) 推送插件
adb push out/arm64-v8a/libGL.so        "$PLUGIN_DIR/"
adb push out/arm64-v8a/libvmcr_vk.so   "$PLUGIN_DIR/"
adb push out/arm64-v8a/libvmcr_gles.so "$PLUGIN_DIR/"
adb push out/arm64-v8a/libvmcr_jni.so  "$PLUGIN_DIR/"
adb push configs/custom_renderer.json  "$PLUGIN_DIR/"
adb push configs/vulkan_features.json  "$PLUGIN_DIR/configs/"
adb push out/arm64-v8a/shaders/        "$PLUGIN_DIR/shaders/"

# 2) 推送 Mod
adb push out/arm64-v8a/VMCR-fabric-1.20.4.jar "$MODS_DIR/"

# 3) 修复权限
adb shell run-as $PKG sh -c "chmod -R 755 custom_renderer/VMCR"

# 4) 强制重启
adb shell am force-stop $PKG
echo "✓ Deployed. Start MC and select VMCR in FCL settings."
```

---

## 8. 打包 APK / 插件目录

### 8.1 打包为 `.fmodpack`（推荐）

```bash
cd out/arm64-v8a
zip -r VMCR-1.0.0-arm64-v8a.fmodpack . \
    -x "*.spv" \
    -x "*.log"
```

> FCL 支持直接安装 `.fmodpack` 到 `mods/` 目录。

### 8.2 打包为 APK

```bash
# 使用 aapt2 + d8 + zipalign
AAPT2=$ANDROID_SDK_ROOT/build-tools/34.0.0/aapt2
D8=$ANDROID_SDK_ROOT/build-tools/34.0.0/d8
ZIPALIGN=$ANDROID_SDK_ROOT/build-tools/34.0.0/zipalign
APKSIGNER=$ANDROID_SDK_ROOT/build-tools/34.0.0/apksigner

# 1) 资源
$AAPT2 compile --dir res/ -o compiled-res.zip
$AAPT2 link -o unsigned.apk \
    -I $ANDROID_SDK_ROOT/platforms/android-34/android.jar \
    --manifest AndroidManifest.xml \
    compiled-res.zip

# 2) dex
$D8 --output dex/ classes.jar
zip -j unsigned.apk dex/classes.dex

# 3) native libs
zip -j unsigned.apk lib/arm64-v8a/*.so
zip -j unsigned.apk assets/shaders/*.spv

# 4) 对齐 + 签名
$ZIPALIGN -p 4 unsigned.apk aligned.apk
$APKSIGNER sign --ks debug.keystore --ks-pass pass:android aligned.apk
```

---

## 9. 调试与诊断

### 9.1 启用详细日志

```bash
# 临时
adb shell setprop log.tag.VMCR-Core VERBOSE
adb shell setprop log.tag.VMCR-VK    VERBOSE
adb shell setprop log.tag.VMCR-GL    VERBOSE
adb shell setprop log.tag.VMCR-JNI   VERBOSE

# 永久（custom_renderer.json）
"env": { "VMCR_LOG_LEVEL": "3" }
```

### 9.2 强制降级

```bash
# 强制 GLES32
adb shell setprop debug.vmcr.forced gles32

# 强制 VulkanLimited
adb shell setprop debug.vmcr.forced vk_limited
```

### 9.3 GPU 时间戳采集

```bash
adb shell setprop debug.vmcr.profile 1
# 启动游戏后
adb pull /sdcard/Android/data/com.tungsten.fcl/files/vmcr_profile_<pid>.csv
```

### 9.4 RenderDoc 集成

```bash
# 设备端安装 RenderDoc
adb install RenderDoc.apk

# 启动捕获
adb shell am start -n com.tungsten.fcl/com.tungsten.fcl.LauncherActivity
# 在 RenderDoc 中 attach 到 com.tungsten.fcl 进程
```

### 9.5 AddressSanitizer 远程诊断

```bash
# 编译时开启
cmake -B build -DVMCR_ENABLE_ASAN=ON
cmake --build build

# 设备端需要 wrap.sh 支持
adb shell run-as com.tungsten.fcl sh -c \
  "echo 'export ASAN_OPTIONS=halt_on_error=1:abort_on_error=1' > wrapper.sh"
```

---

## 10. CI 集成

### 10.1 GitHub Actions

```yaml
# .github/workflows/build.yml
name: Android Build

on:
  push:
    branches: [main, develop]
  pull_request:

jobs:
  build-arm64:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
        with: { submodules: recursive }

      - name: Setup NDK r27
        uses: android-actions/setup-ndk@v4
        with: { ndk-version: r27 }

      - name: Setup Java 17
        uses: actions/setup-java@v4
        with: { distribution: temurin, java-version: 17 }

      - name: Install build tools
        run: |
          sudo apt-get update
          sudo apt-get install -y glslang-tools spirv-tools ninja-build ccache

      - name: Cache ccache
        uses: actions/cache@v4
        with:
          path: ~/.ccache
          key: ccache-${{ runner.os }}-${{ hashFiles('**/CMakeLists.txt') }}

      - name: Build native
        env:
          ANDROID_NDK_HOME: ${{ steps.ndk.path }}/android-ndk-r27
        run: |
          export PATH=$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH
          cmake -G Ninja -S . -B build/arm64 \
              -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-ndk-aarch64.cmake \
              -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
          cmake --build build/arm64 --parallel

      - name: Compile shaders
        run: ./scripts/compile_shaders.sh src/shaders build/arm64/shaders

      - name: Build Fabric mod
        run: |
          cd src/fabric_mod
          ./gradlew build
          cp build/libs/VMCR-fabric-*.jar ../build/arm64/

      - name: Package
        run: |
          cd build/arm64
          zip -r ../../out/VMCR-${{ github.sha }}-arm64-v8a.fmodpack .

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: VMCR-package
          path: out/*.fmodpack
```

### 10.2 ccache 加速

```bash
# 本地
export CMAKE_CXX_COMPILER_LAUNCHER=ccache
ccache -M 10G

# 命中率
ccache -s
```

---

## 11. 常见问题

| 现象 | 原因 | 解决 |
| :--- | :--- | :--- |
| `vulkan/vulkan.h: No such file or directory` | NDK 路径错误或版本过旧 | 确认 `ANDROID_NDK_HOME` 与 NDK ≥ r25 |
| `error: unrecognized argument '-mcpu=cortex-a75'` | armeabi-v7a 不支持 | 仅在 arm64 工具链加该 flag |
| `undefined reference to __cxa_atexit` | 静态 libc++ 缺 `android` 库 | `target_link_libraries(... android)` |
| `libGL.so` 未生成 | `OUTPUT_NAME` 拼错 | CMake 中确认 `OUTPUT_NAME "GL"` |
| FCL 启动后黑屏 | Vulkan 1.3 不可用但被强制 | `setprop debug.vmcr.forced gles32` |
| `glslangValidator not found` | 系统未安装 | `apt install glslang-tools` |
| `spirv-opt not found` | spirv-tools 未安装 | `apt install spirv-tools` |
| Fabric Mod 加载失败 | 缺少 `fabric.mod.json` 字段 | 确认 `custom.vmcr.router_lib` |
| 启动日志缺失 | 过滤 TAG 错误 | 使用 `logcat -s VMCR-*:V` |
| `dlopen libGL.so.vendor failed` | 备份原厂驱动未拷贝 | 检查 `system/lib64/egl/` |

---

## 12. 构建变体速查

| 用途 | 命令 |
| :--- | :--- |
| 完整 Vulkan 路径 | `-DVMCR_ENABLE_VULKAN=ON -DVMCR_ENABLE_GLES=ON` |
| 仅 GLES 保底（CI 加速） | `-DVMCR_ENABLE_VULKAN=OFF` |
| 强制 VulkanLimited | `-DVMCR_RENDER_TIER=VulkanLimited` |
| 调试 + ASan | `-DCMAKE_BUILD_TYPE=Debug -DVMCR_ENABLE_ASAN=ON` |
| Shader 热重载 | `-DVMCR_ENABLE_SHADER_HOT_RELOAD=ON` |
| 关闭 NEON（纯 C++） | `-DVMCR_ENABLE_NEON=OFF` |
| 32-bit ABI | `-DANDROID_ABI=armeabi-v7a` + 替换工具链文件 |
