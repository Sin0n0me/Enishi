param(
    [Parameter(Mandatory)][ValidateSet('directx', 'opengl', 'vulkan')][string]$Backend,
    [Parameter(Mandatory)][ValidateSet('visualstudio', 'clang', 'gcc')][string]$Toolchain,
    [Parameter(Mandatory)][ValidateSet('Debug', 'Release')][string]$Config
)
$ErrorActionPreference = 'Stop'
$vcpkgRoot = Split-Path (Get-Command vcpkg).Source
$configureArgs = @('-S', 'src', '-B', "build/$Backend/$Toolchain",
    "-DCMAKE_TOOLCHAIN_FILE=$vcpkgRoot/scripts/buildsystems/vcpkg.cmake",
    "-DVCPKG_INSTALLED_DIR=$env:GITHUB_WORKSPACE/vcpkg_installed",
    "-DVCPKG_TARGET_TRIPLET=$env:VCPKG_TARGET_TRIPLET",
    "-DPython_EXECUTABLE=$env:pythonLocation/python.exe",
    "-DPython3_EXECUTABLE=$env:pythonLocation/python.exe",
    "-DUSE_OPENGL40=$($Backend -eq 'opengl')", "-DUSE_VULKAN=$($Backend -eq 'vulkan')",
    "-DENISHI_VULKAN_TESTS=$($Backend -eq 'vulkan')",
    "-DENISHI_SCRIPT_TESTS=$($Toolchain -ne 'visualstudio')",
    "-DENISHI_CURSOR_TESTS=$($Toolchain -ne 'visualstudio')",
    "-DENISHI_ECS_TESTS=$($Toolchain -ne 'visualstudio')")
if ($Toolchain -eq 'visualstudio') {
    $configureArgs += @('-G', 'Visual Studio 17 2022', '-A', 'x64')
} else {
    $compiler = if ($Toolchain -eq 'clang') { 'clang' } else { 'gcc' }
    $cxxCompiler = if ($Toolchain -eq 'clang') { 'clang++' } else { 'g++' }
    $configureArgs += @('-G', 'Ninja', "-DCMAKE_BUILD_TYPE=$Config",
        "-DCMAKE_C_COMPILER=$compiler", "-DCMAKE_CXX_COMPILER=$cxxCompiler")
}
cmake @configureArgs
exit $LASTEXITCODE
