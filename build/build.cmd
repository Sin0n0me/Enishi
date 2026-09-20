@echo off
setlocal EnableExtensions

set "PROJECT_ROOT=%~dp0.."
set "SOURCE_DIR=%PROJECT_ROOT%\src"
set "BACKEND=%~1"
set "CONFIG=%~2"
set "TOOLCHAIN=%~3"

if "%BACKEND%"=="" goto :usage
if "%CONFIG%"=="" set "CONFIG=Debug"
if "%TOOLCHAIN%"=="" set "TOOLCHAIN=visualstudio"

if /I "%BACKEND%"=="opengl" goto :opengl
if /I "%BACKEND%"=="directx" goto :directx
if /I "%BACKEND%"=="vulkan" goto :vulkan_not_available
goto :usage

:opengl
set "BACKEND_OPTIONS=-DUSE_OPENGL40=ON"
goto :select_toolchain

:directx
set "BACKEND_OPTIONS=-DUSE_OPENGL40=OFF"
goto :select_toolchain

:select_toolchain
if /I "%TOOLCHAIN%"=="visualstudio" goto :visualstudio
if /I "%TOOLCHAIN%"=="clang" goto :clang
goto :usage

:visualstudio
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :visualstudio_2022

for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -version "[18.0,19.0^)" -property installationPath`) do set "VS18_PATH=%%I"
if defined VS18_PATH (
    set "GENERATOR=Visual Studio 18 2026"
) else (
    set "GENERATOR=Visual Studio 17 2022"
)
goto :build_visualstudio

:visualstudio_2022
set "GENERATOR=Visual Studio 17 2022"

:build_visualstudio
set "BUILD_DIR=%~dp0%BACKEND%\visualstudio"
cmake -S "%SOURCE_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%" %BACKEND_OPTIONS%
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%" --config "%CONFIG%"
exit /b %errorlevel%

:clang
where.exe ninja >nul 2>nul
if errorlevel 1 (
    echo Ninja was not found. Install Ninja and add it to PATH to use the clang toolchain.
    exit /b 1
)

where.exe clang >nul 2>nul
if errorlevel 1 (
    echo Clang was not found. Install LLVM and add it to PATH to use the clang toolchain.
    exit /b 1
)

set "BUILD_DIR=%~dp0%BACKEND%\clang"
cmake -S "%SOURCE_DIR%" -B "%BUILD_DIR%" -G Ninja %BACKEND_OPTIONS% -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=%CONFIG%
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%"
exit /b %errorlevel%

:vulkan_not_available
echo Vulkan build support has not been added to CMake yet.
echo Add a :vulkan section with its CMake options when the renderer is ready.
exit /b 1

:usage
echo Usage: %~nx0 ^<opengl^|directx^> [Debug^|Release^|RelWithDebInfo^|MinSizeRel] [visualstudio^|clang]
echo Example: %~nx0 opengl Debug
echo Example: %~nx0 directx Release clang
exit /b 1
