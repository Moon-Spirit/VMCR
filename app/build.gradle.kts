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
    ndkVersion = "27.3.13750724"   // 与 NDK r27d 一致

    defaultConfig {
        applicationId = "io.anomalyco.vmcr"
        minSdk = 26              // Android 8.0 (匹配 FCL 最低要求)
        targetSdk = 34
        versionCode = 200
        versionName = "0.2.0"

        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64")
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
            path = file("../../CMakeLists.txt")
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
