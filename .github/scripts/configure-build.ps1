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
    '-DVCPKG_MANIFEST_INSTALL=OFF',
    "-DVCPKG_TARGET_TRIPLET=$env:VCPKG_TARGET_TRIPLET",
    "-DPython_EXECUTABLE=$env:ENISHI_PYTHON",
    "-DPython3_EXECUTABLE=$env:ENISHI_PYTHON",
    '-DENISHI_VULKAN_TESTS=OFF',
    '-DENISHI_OPENGL_TESTS=OFF',
    '-DENISHI_EFFECT_RENDER_TESTS=OFF',
    '-DENISHI_RENDERER_RESOURCE_TESTS=OFF',
    '-DENISHI_COMMON_TESTS=OFF',
    '-DENISHI_SCRIPT_TESTS=OFF',
    '-DENISHI_CURSOR_TESTS=OFF',
    '-DENISHI_ECS_TESTS=OFF')
cmake @configureArgs
exit $LASTEXITCODE
