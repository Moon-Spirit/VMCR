// ===========================================================================
// FCL 鎻掍欢 build.gradle.kts
// 鐢?Android Gradle Plugin 8.x 鎵撳寘 APK, lib/ 鐩綍鍚?
//   - libGL.so           (VMCR 涓诲叆鍙?
//   - libEGL.so          (VMCR EGL 鍏ュ彛)
//   - libvmcr_vk.so      (Vulkan 鍚庣, dlopen)
//   - libvmcr_gles.so    (GLES 鍚庣, dlopen)
//   - libvmcr_jni.so     (JNI 妗? dlopen)
//
// 缂栬瘧娴佺▼: Gradle 瑙﹀彂 cmake (CMakeLists.txt) 缂栬瘧 .so,
// 鐒跺悗鎵撳叆 APK 鐨?lib/<abi>/ 鐩綍.
// ===========================================================================
plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.mio.plugin.renderer"
    compileSdk = 34
    // 使用系统已安装的 NDK (CI: 27.3.13750724 / 本地: 30.0.14904198)
    // 不指定具体版�? AGP 自动选已安装的最�?
    defaultConfig {
        applicationId = "com.mio.plugin.renderer"
        applicationIdSuffix = ".vmcr"
        minSdk = 26              // Android 8.0 (匹配 FCL 最低要�?
        targetSdk = 34
        versionCode = 202
        versionName = "0.2.2"

        ndk {
            // 4 涓?ABI: 64/32 浣?ARM + 64/32 浣?Intel
            //   arm64-v8a   : SM8635 绛夌幇浠ｈ澶?(Vulkan 1.3 鐩爣)
            //   armeabi-v7a : 32 浣?ARM (1.7.10 鑰佽澶? 鏃?GLES 璺緞)
            //   x86_64      : 64 浣?Intel (Android Studio 妯℃嫙鍣?
            //   x86         : 32 浣?Intel (鑰?Android 妯℃嫙鍣?
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
        // 璋冭瘯绛惧悕 (鐢熶骇绛惧悕鐢?CI 閰嶇疆)
        getByName("debug") {
            storeFile = file("${System.getProperty("user.home")}/.android/debug.keystore")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false   // 鏆傛椂鍏抽棴 R8, 閬垮厤娣锋穯 native 绗﹀彿
            signingConfig = signingConfigs.getByName("debug")
        }
    }

    externalNativeBuild {
        cmake {
            // build.gradle.kts 鍦?app/, CMakeLists.txt 鍦?VMCR 鏍圭洰褰?            // 鐢?../ 鍗冲彲 (gradle 浠?build.gradle.kts 鎵€鍦ㄧ洰褰曚负鍩哄噯)
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
    // 浠呯敤 Android SDK 鏍囧噯搴? 鏃犵涓夋柟渚濊禆
}




