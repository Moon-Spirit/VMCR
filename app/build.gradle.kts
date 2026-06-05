// ===========================================================================
// FCL 插件 build.gradle.kts
// 用 Android Gradle Plugin 8.x 打包 APK, lib/ 目录含:
//   - libGL.so           (VMCR 主入口)
//   - libEGL.so          (VMCR EGL 入口)
//   - libvmcr_vk.so      (Vulkan 后端, dlopen)
//   - libvmcr_gles.so    (GLES 后端, dlopen)
//   - libvmcr_jni.so     (JNI 桥, dlopen)
//
// 编译流程: Gradle 触发 cmake (CMakeLists.txt) 编译 .so,
// 然后打入 APK 的 lib/<abi>/ 目录.
// ===========================================================================
plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "io.anomalyco.vmcr"
    compileSdk = 34
    // 使用系统已安装的 NDK (CI: 27.3.13750724 / 本地: 30.0.14904198)
    // 不指定具体版本, AGP 自动选已安装的最新

    defaultConfig {
        applicationId = "io.anomalyco.vmcr"
        minSdk = 26              // Android 8.0 (匹配 FCL 最低要求)
        targetSdk = 34
        versionCode = 200
        versionName = "0.2.0"

        ndk {
            // 4 个 ABI: 64/32 位 ARM + 64/32 位 Intel
            //   arm64-v8a   : SM8635 等现代设备 (Vulkan 1.3 目标)
            //   armeabi-v7a : 32 位 ARM (1.7.10 老设备, 旧 GLES 路径)
            //   x86_64      : 64 位 Intel (Android Studio 模拟器)
            //   x86         : 32 位 Intel (老 Android 模拟器)
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64", "x86")
        }

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DANDROID_STL=c++_static",
                    "-DVMCR_RENDER_TIER=auto",
                    "-DVMCR_USE_GLSLANG=ON",
                    "-DVMCR_USE_SPIRV_CROSS=ON",
                    "-DVMCR_USE_VULKAN_HPP=ON",
                    "-DVMCR_USE_VMA=ON",
                    "-DVMCR_USE_VKBOOTSTRAP=ON",
                    "-DVMCR_USE_GLM=ON",
                    "-DVMCR_USE_SPDLOG=ON"
                )
                version = "3.22.1"
                cppFlags += listOf("-std=c++20", "-fvisibility=hidden", "-fexceptions")
                cFlags += listOf("-fvisibility=hidden")
            }
        }
    }

    signingConfigs {
        // 调试签名 (生产签名由 CI 配置)
        getByName("debug") {
            storeFile = file("${System.getProperty("user.home")}/.android/debug.keystore")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false   // 暂时关闭 R8, 避免混淆 native 符号
            signingConfig = signingConfigs.getByName("debug")
        }
    }

    externalNativeBuild {
        cmake {
            // build.gradle.kts 在 app/, CMakeLists.txt 在 VMCR 根目录
            // 用 ../ 即可 (gradle 以 build.gradle.kts 所在目录为基准)
            path = file("../CMakeLists.txt")
            version = "3.22.1"
        }
    }

    buildFeatures {
        buildConfig = true
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    packaging {
        jniLibs {
            useLegacyPackaging = true
            keepDebugSymbols += listOf("**/*.so")
        }
    }
}

dependencies {
    // 仅用 Android SDK 标准库, 无第三方依赖
}
