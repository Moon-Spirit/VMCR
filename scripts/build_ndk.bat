@echo off
REM ===========================================================================
REM VMCR Android NDK 构建脚本 (Windows + NDK 30)
REM 验证项目在 NDK 工具链下能正确编译
REM ===========================================================================
setlocal

if "%1"=="--clean" (
    if exist build\ndk-arm64 rmdir /s /q build\ndk-arm64
)

set NDK_HOME=C:\Users\zh134\AppData\Local\Android\Sdk\ndk\30.0.14904198
if not exist "%NDK_HOME%\build\cmake\android.toolchain.cmake" (
    echo [ERROR] NDK toolchain not found at %NDK_HOME%
    exit /b 1
)

set VCVARS="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
call %VCVARS% >nul 2>&1

if not exist build\ndk-arm64 mkdir build\ndk-arm64

echo [CONFIGURE]
cmake -G Ninja -S . -B build\ndk-arm64 ^
    -DCMAKE_TOOLCHAIN_FILE="%NDK_HOME%\build\cmake\android.toolchain.cmake" ^
    -DANDROID_ABI=arm64-v8a ^
    -DANDROID_PLATFORM=android-26 ^
    -DANDROID_STL=c++_static ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DVMCR_RENDER_TIER=auto
if errorlevel 1 exit /b 1

echo [BUILD]
cmake --build build\ndk-arm64 --parallel
if errorlevel 1 exit /b 1

echo [DONE]
dir build\ndk-arm64\src\main\cpp\loader\*.so 2>nul
dir build\ndk-arm64\src\main\cpp\vulkan\*.so 2>nul
dir build\ndk-arm64\src\main\cpp\gles\*.so 2>nul
dir build\ndk-arm64\tools\*.exe 2>nul
