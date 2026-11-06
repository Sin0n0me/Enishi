param(
    [ValidateSet('directx', 'opengl', 'vulkan')][string]$Backend = 'directx',
    [ValidateSet('Debug', 'Release')][string]$Config = 'Debug',
    [ValidateSet('clang', 'visualstudio')][string]$Compiler = 'clang'
)
$ErrorActionPreference = 'Stop'
function Invoke-CheckedCommand {
    param([string]$Command, [string[]]$Arguments)
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Command failed with exit code $LASTEXITCODE" }
}

function Save-VcpkgFailureLogs {
    param([string]$BuildDirectory)
    $destination = Join-Path $BuildDirectory 'diagnostics/vcpkg'
    $sources = @{
        installation = (Join-Path $env:VCPKG_ROOT 'buildtrees')
        manifest = (Join-Path $BuildDirectory 'vcpkg_installed/vcpkg/blds')
    }
    foreach ($entry in $sources.GetEnumerator()) {
        $source = $entry.Value
        if (-not (Test-Path -LiteralPath $source -PathType Container)) { continue }
        foreach ($log in Get-ChildItem -LiteralPath $source -Recurse -File -Filter '*.log') {
            $relative = $log.FullName.Substring($source.Length).TrimStart('\', '/')
            $target = Join-Path (Join-Path $destination $entry.Key) $relative
            New-Item -ItemType Directory -Force -Path (Split-Path $target) | Out-Null
            Copy-Item -LiteralPath $log.FullName -Destination $target -Force
        }
    }
    Write-Host "vcpkg failure logs: $destination"
}

foreach ($required in @('src/CMakeLists.txt', 'tools/SPIRV-Cross/CMakeLists.txt',
    'src/engine/script/Musubu/compiler/musubu_engine/Cargo.toml', 'third_party/tomlplusplus/CMakeLists.txt')) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Missing $required. Mount the repository and initialize its submodules." }
}
$vcpkgRoot = $env:VCPKG_ROOT
if ([string]::IsNullOrWhiteSpace($vcpkgRoot) -or
    -not (Test-Path -LiteralPath (Join-Path $vcpkgRoot 'vcpkg.exe') -PathType Leaf)) {
    throw 'The image must provide its pinned vcpkg installation through VCPKG_ROOT'
}
& C:\BuildTools\Common7\Tools\Launch-VsDevShell.ps1 -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
if ([string]::IsNullOrWhiteSpace($env:VCToolsVersion)) { throw 'MSVC developer environment initialization failed' }
# VsDevShell selects VS's bundled vcpkg; retain the image's manifest-pinned installation.
$env:VCPKG_ROOT = $vcpkgRoot
$env:VCPKG_INSTALLATION_ROOT = $vcpkgRoot
Write-Host "vcpkg root: $vcpkgRoot"
$env:VULKAN_SDK = 'C:\VulkanSDK'
$env:PATH = "C:\Tools\Ninja;C:\Tools\Git\cmd;C:\Tools\uv;C:\cargo\bin;C:\Tools\python\.venv\Scripts;C:\VulkanSDK\Bin;C:\BuildTools\VC\Tools\Llvm\x64\bin;C:\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;$env:PATH"
$ninja = 'C:\Tools\Ninja\ninja.exe'
Invoke-CheckedCommand $ninja @('--version')
Invoke-CheckedCommand cmake @('--version')
$env:UV_PROJECT_ENVIRONMENT = 'C:\Tools\python\.venv'
Invoke-CheckedCommand uv @('sync', '--locked')
$python = Join-Path $env:UV_PROJECT_ENVIRONMENT 'Scripts/python.exe'
$preset = "windows-$Backend-$Compiler-$($Config.ToLowerInvariant())"
$directory = Join-Path $PWD "build/docker/$preset"
$installed = Join-Path $directory 'vcpkg_installed'
$generatorOptions = @()
if ($Compiler -eq 'clang') {
    $generatorOptions += "-DCMAKE_MAKE_PROGRAM=$ninja"
}
$configureArguments = @('--preset', $preset, '-S', 'src', '-B', $directory,
    "-DCMAKE_RC_COMPILER=$((Get-Command rc.exe -ErrorAction Stop).Source)",
    "-DCMAKE_MT=$((Get-Command mt.exe -ErrorAction Stop).Source)",
    "-DVCPKG_INSTALLED_DIR=$installed", '-DVCPKG_TARGET_TRIPLET=x64-windows',
    "-DPython_EXECUTABLE=$python", "-DPython3_EXECUTABLE=$python",
    '-DENISHI_SHADER_RUNTIME_ROOT=./shader',
    '-DENISHI_COMMON_TESTS=OFF', '-DENISHI_EFFECT_RENDER_TESTS=OFF',
    '-DENISHI_SCRIPT_TESTS=OFF', '-DENISHI_CURSOR_TESTS=OFF', '-DENISHI_ECS_TESTS=OFF',
    '-DENISHI_RENDERER_RESOURCE_TESTS=OFF', '-DENISHI_OPENGL_TESTS=OFF', '-DENISHI_VULKAN_TESTS=OFF') + $generatorOptions
try {
    Invoke-CheckedCommand cmake $configureArguments
} catch {
    $configureFailure = $_
    try {
        Save-VcpkgFailureLogs $directory
    } catch {
        Write-Warning "Could not preserve vcpkg logs: $_"
    }
    throw $configureFailure
}
Invoke-CheckedCommand cmake @('--build', $directory, '--config', $Config, '--target', 'enishi_engine', 'enishi-shell')

$binaries = $directory
if ($Compiler -eq 'visualstudio') { $binaries = Join-Path $directory $Config }
$icuDirectory = Join-Path $installed 'x64-windows/bin'
if ($Config -eq 'Debug') { $icuDirectory = Join-Path $installed 'x64-windows/debug/bin' }
$icu = @(Get-ChildItem -LiteralPath $icuDirectory -Filter 'icu*.dll' -File)
if ($icu.Count -eq 0) { throw "ICU runtime DLLs are missing: $icuDirectory" }
$icu | Copy-Item -Destination $binaries
Write-Host "Executables: $binaries"
