param(
    [Parameter(Mandatory)][ValidateSet('directx', 'opengl', 'vulkan')][string]$Backend,
    [Parameter(Mandatory)][ValidateSet('visualstudio', 'clang', 'gcc')][string]$Toolchain,
    [Parameter(Mandatory)][ValidateSet('Debug', 'Release')][string]$Config
)
$ErrorActionPreference = 'Stop'
$env:VCPKG_ROOT = Split-Path (Get-Command vcpkg).Source
$preset = "windows-$Backend-$Toolchain-$($Config.ToLowerInvariant())"
$configureArgs = @('--preset', $preset, '-S', 'src', '-B', "build/$Backend/$Toolchain",
    "-DENISHI_SHADER_RUNTIME_ROOT=./shader",
    "-DVCPKG_INSTALLED_DIR=$env:GITHUB_WORKSPACE/vcpkg_installed",
    "-DVCPKG_TARGET_TRIPLET=$env:VCPKG_TARGET_TRIPLET",
    "-DPython_EXECUTABLE=$env:pythonLocation/python.exe",
    "-DPython3_EXECUTABLE=$env:pythonLocation/python.exe",
    "-DENISHI_VULKAN_TESTS=$($Backend -eq 'vulkan')",
    "-DENISHI_SCRIPT_TESTS=$($Toolchain -ne 'visualstudio')",
    "-DENISHI_CURSOR_TESTS=$($Toolchain -ne 'visualstudio')",
    "-DENISHI_ECS_TESTS=$($Toolchain -ne 'visualstudio')")
cmake @configureArgs
exit $LASTEXITCODE
