# FCL 插件 APK 构建 (Windows / PowerShell)

VMCR 的 FCL 插件以 APK 形式分发. 本脚本调用 Gradle 包装的
外部 CMake 构建, 生成最终 APK.

## 前置条件
- Android SDK (含 build-tools 34, platform-34)
- Android NDK r27d (27.3.13750724)
- JDK 17 (Temurin 推荐)
- Gradle 8.x (Wrapper 自带)

## 构建

```powershell
cd app
.\gradlew.bat :app:assembleRelease
# 产物: app\build\outputs\apk\release\app-release.apk
```

## 安装到 FCL
1. 将 APK 传到设备:
   adb push app-release.apk /sdcard/
2. 用设备文件管理器点击安装
3. 打开 FCL, 在渲染器列表中会出现 "VMCR"
4. 选择 VMCR, FCL 启动 MC 时会自动加载 libGL.so / libEGL.so

## 验证加载

```bash
adb logcat -d -s VMCR-Core VMCR-VK VMCR-GL VMCR-JNI
# 期望:
# VMCR-Core I [BOOT] libGL.so loaded, tier=GLES32
# VMCR-GL   I [GLES] init OK, vendor GLES 3.2
```

## 卸载
adb uninstall io.anomalyco.vmcr
