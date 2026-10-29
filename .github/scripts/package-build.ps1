param(
    [Parameter(Mandatory)][ValidateSet('directx', 'opengl', 'vulkan')][string]$Backend,
    [Parameter(Mandatory)][ValidateSet('visualstudio', 'clang', 'gcc')][string]$Toolchain,
    [Parameter(Mandatory)][ValidateSet('Debug', 'Release')][string]$Config
)
$ErrorActionPreference = 'Stop'
$buildDirectory = Join-Path $PWD "build/$Backend/$Toolchain"
$binaryDirectory = $buildDirectory
if ($Toolchain -eq 'visualstudio') {
    $binaryDirectory = Join-Path $binaryDirectory $Config
}
$destination = Join-Path $PWD 'artifacts/application'
New-Item -ItemType Directory -Path $destination -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $binaryDirectory 'enishi_engine.exe') -Destination $destination
Copy-Item -LiteralPath (Join-Path $binaryDirectory 'enishi-shell.exe') -Destination $destination
Get-ChildItem -LiteralPath $binaryDirectory -File |
    Where-Object { $_.Extension -in '.dll', '.pdb' } |
    Copy-Item -Destination $destination

$packageBin = Join-Path $PWD "vcpkg_installed/$env:VCPKG_TARGET_TRIPLET"
if ($Config -eq 'Debug') {
    $packageBin = Join-Path $packageBin 'debug'
}
$packageBin = Join-Path $packageBin 'bin'
$icuLibraries = @(Get-ChildItem -LiteralPath $packageBin -Filter 'icu*.dll' -File)
if ($icuLibraries.Count -eq 0) {
    throw "ICU runtime DLLs were not found: $packageBin"
}
$icuLibraries | Copy-Item -Destination $destination
if ($Toolchain -eq 'gcc') {
    $compilerBin = Split-Path (Get-Command g++).Source
    foreach ($library in 'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll') {
        Copy-Item -LiteralPath (Join-Path $compilerBin $library) -Destination $destination
    }
}

$shaders = Join-Path $buildDirectory "shader/$Backend"
if (@(Get-ChildItem -LiteralPath $shaders -File -Recurse).Count -eq 0) {
    throw "Generated shaders were not found: $shaders"
}
$shaderDestination = Join-Path $destination 'shader'
New-Item -ItemType Directory -Path $shaderDestination -Force | Out-Null
Copy-Item -LiteralPath $shaders -Destination $shaderDestination -Recurse
Copy-Item -LiteralPath (Join-Path $PWD 'config') -Destination $destination -Recurse
