@echo off
REM ============================================================================
REM VMCR 主机侧构建脚本 (Windows + VS 2022)
REM 验证项目在主机构建时无编译错误
REM ============================================================================
setlocal

set VCVARS="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist %VCVARS% (
    echo [ERROR] vcvars64.bat not found at %VCVARS%
    exit /b 1
)

call %VCVARS% >nul
if errorlevel 1 (
    echo [ERROR] vcvars64.bat failed
    exit /b 1
)

cd /d "%~dp0\.."

if "%1"=="--clean" (
    if exist build\host rmdir /s /q build\host
)

if not exist build\host mkdir build\host

if not defined VULKAN_SDK set VULKAN_SDK=C:\VulkanSDK\1.4.350.0

echo [CONFIG]
cmake -G Ninja -S . -B build\host ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DVMCR_ENABLE_LOADER=OFF ^
    -DVMCR_ENABLE_JNI=OFF ^
    -DVMCR_ENABLE_UNIT_TEST=ON ^
    -DVMCR_RENDER_TIER=auto
if errorlevel 1 exit /b 1

echo [BUILD]
cmake --build build\host --parallel
if errorlevel 1 exit /b 1

echo [TEST]
cd build\host
ctest --output-on-failure

echo [DONE]
