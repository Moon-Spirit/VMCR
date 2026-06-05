# ---------------------------------------------------------------------------
# VMCR Windows 构建脚本 (PowerShell 5.1+)
# ---------------------------------------------------------------------------
[CmdletBinding()]
param(
    [string]$Abi       = "arm64-v8a",
    [string]$Config    = "Release",
    [string]$Tier      = "auto",
    [switch]$Shaders,
    [switch]$Clean,
    [switch]$Asan
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$BuildDir    = Join-Path $ProjectRoot "build\$Abi"
$OutDir      = Join-Path $ProjectRoot "out\$Abi"

if (-not $env:ANDROID_NDK_HOME) {
    throw "ANDROID_NDK_HOME is not set"
}

if ($Clean) {
    Remove-Item -Recurse -Force $BuildDir -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force $OutDir   -ErrorAction SilentlyContinue
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
New-Item -ItemType Directory -Force -Path $OutDir   | Out-Null

$Toolchain = Join-Path $ProjectRoot "cmake\toolchain-ndk-aarch64.cmake"
$AsanFlag  = if ($Asan) { "-DVMCR_ENABLE_ASAN=ON" } else { "" }

Write-Host "==> Configure" -ForegroundColor Cyan
& cmake -G Ninja `
    -S $ProjectRoot `
    -B $BuildDir `
    -DCMAKE_TOOLCHAIN_FILE=$Toolchain `
    -DCMAKE_BUILD_TYPE=$Config `
    -DANDROID_ABI=$Abi `
    -DVMCR_RENDER_TIER=$Tier `
    $AsanFlag

if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

Write-Host "==> Build" -ForegroundColor Cyan
& cmake --build $BuildDir --parallel
if ($LASTEXITCODE -ne 0) { throw "CMake build failed" }

Write-Host "==> Collect" -ForegroundColor Cyan
$artifacts = @(
    "src\main\cpp\loader\libGL.so",
    "src\main\cpp\vulkan\libvmcr_vk.so",
    "src\main\cpp\gles\libvmcr_gles.so",
    "src\main\cpp\jni\libvmcr_jni.so"
)
foreach ($a in $artifacts) {
    $src = Join-Path $BuildDir $a
    if (Test-Path $src) { Copy-Item $src $OutDir }
}

Write-Host "Build OK: $OutDir" -ForegroundColor Green
Get-ChildItem $OutDir | Format-Table Name, Length -AutoSize
