$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

function Invoke-CheckedCommand {
    param([string]$Command, [string[]]$Arguments)
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Command failed with exit code $LASTEXITCODE" }
}

function Install-Archive {
    param([string]$Uri, [string]$Destination)
    Invoke-WebRequest -UseBasicParsing -Uri $Uri -OutFile C:\Tools\download.zip
    Expand-Archive -LiteralPath C:\Tools\download.zip -DestinationPath $Destination
    Remove-Item -LiteralPath C:\Tools\download.zip
}

Invoke-WebRequest -UseBasicParsing https://aka.ms/vs/17/release/vs_buildtools.exe -OutFile C:\Tools\vs_buildtools.exe
$installer = Start-Process C:\Tools\vs_buildtools.exe -WindowStyle Hidden -Wait -PassThru -ArgumentList @(
    '--quiet', '--wait', '--norestart', '--nocache', '--installPath', 'C:\BuildTools',
    '--add', 'Microsoft.VisualStudio.Workload.VCTools',
    '--add', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
    '--add', 'Microsoft.VisualStudio.Component.Windows10SDK.19041',
    '--add', 'Microsoft.VisualStudio.Component.VC.Llvm.Clang',
    '--add', 'Microsoft.VisualStudio.Component.VC.CMake.Project')
if ($installer.ExitCode -notin @(0, 3010)) { throw "Build Tools installation failed: $($installer.ExitCode)" }
Remove-Item -LiteralPath C:\Tools\vs_buildtools.exe

# The recommended SDK 26100 rc.exe crashes in this Server Core 2022 image.
# Check actual resource compilation before downloading the remaining dependencies.
Set-Content -LiteralPath C:\Tools\verify-sdk.rc -Value '1 RCDATA { 1 }' -Encoding ascii
Invoke-CheckedCommand 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\rc.exe' @(
    '/nologo', '/fo', 'C:\Tools\verify-sdk.res', 'C:\Tools\verify-sdk.rc')
Remove-Item -LiteralPath C:\Tools\verify-sdk.rc, C:\Tools\verify-sdk.res

Install-Archive 'https://github.com/git-for-windows/git/releases/download/v2.47.1.windows.1/MinGit-2.47.1-64-bit.zip' C:\Tools\Git
Install-Archive 'https://github.com/astral-sh/uv/releases/download/0.9.9/uv-x86_64-pc-windows-msvc.zip' C:\Tools\uv
$env:PATH = "C:\Tools\Git\cmd;C:\Tools\uv;C:\cargo\bin;$env:PATH"
Invoke-CheckedCommand uv @('sync', '--locked', '--project', 'C:\Tools\python')

$rust = Get-Content C:\Tools\definitions\rust-toolchain.toml -Raw
if ($rust -notmatch '(?m)^channel = "([0-9]+\.[0-9]+\.[0-9]+)"\r?$') { throw 'Missing Rust channel' }
$channel = $Matches[1]
Invoke-WebRequest -UseBasicParsing https://win.rustup.rs/x86_64 -OutFile C:\Tools\rustup-init.exe
Invoke-CheckedCommand C:\Tools\rustup-init.exe @('-y', '--profile', 'minimal', '--default-toolchain', $channel, '--no-modify-path')
Remove-Item -LiteralPath C:\Tools\rustup-init.exe

$manifest = Get-Content C:\Tools\definitions\vcpkg.json -Raw | ConvertFrom-Json
Invoke-CheckedCommand git @('clone', 'https://github.com/microsoft/vcpkg.git', $env:VCPKG_ROOT)
Invoke-CheckedCommand git @('-C', $env:VCPKG_ROOT, 'checkout', '--detach', $manifest.'builtin-baseline')
Invoke-CheckedCommand C:\Tools\vcpkg\bootstrap-vcpkg.bat @('-disableMetrics')

# Matches the SDK used by the Windows CI, including its offline shader tools.
$version = '1.4.363.0'
Invoke-WebRequest -UseBasicParsing "https://sdk.lunarg.com/sdk/download/$version/windows/vulkansdk-windows-X64-$version.exe" -OutFile C:\Tools\vulkan-installer.exe
$signature = Get-AuthenticodeSignature -LiteralPath C:\Tools\vulkan-installer.exe
if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'O="?LunarG, Inc\.') {
    throw 'Vulkan SDK installer signature is not a valid LunarG signature'
}
$installer = Start-Process C:\Tools\vulkan-installer.exe -WindowStyle Hidden -Wait -PassThru -ArgumentList @(
    '--root', 'C:\VulkanSDK', '--accept-licenses', '--default-answer', '--confirm-command', 'install', 'copy_only=1')
if ($installer.ExitCode -ne 0) { throw "Vulkan SDK installation failed: $($installer.ExitCode)" }
Remove-Item -LiteralPath C:\Tools\vulkan-installer.exe
foreach ($required in @('Include/vulkan/vulkan.h', 'Lib/vulkan-1.lib', 'Bin/glslc.exe', 'Bin/dxc.exe', 'Bin/spirv-val.exe', 'Source/SPIRV-Reflect/spirv_reflect.c')) {
    if (-not (Test-Path -LiteralPath (Join-Path C:\VulkanSDK $required))) { throw "Missing Vulkan SDK component: $required" }
}
