@echo off
setlocal EnableExtensions

set "PROJECT_ROOT=%~dp0.."
set "SOURCE_DIR=%PROJECT_ROOT%\src"
set "GENERATOR=Visual Studio 17 2022"
set "BACKEND=%~1"
set "CONFIG=%~2"

if "%BACKEND%"=="" goto :usage
if "%CONFIG%"=="" set "CONFIG=Debug"

if /I "%BACKEND%"=="opengl" goto :opengl
if /I "%BACKEND%"=="directx" goto :directx
if /I "%BACKEND%"=="vulkan" goto :vulkan_not_available
goto :usage

:opengl
set "BUILD_DIR=%~dp0opengl"
set "BACKEND_OPTIONS=-DUSE_OPENGL40=ON"
goto :build

:directx
set "BUILD_DIR=%~dp0directx"
set "BACKEND_OPTIONS=-DUSE_OPENGL40=OFF"
goto :build

:vulkan_not_available
echo Vulkan build support has not been added to CMake yet.
echo Add a :vulkan section with its CMake options when the renderer is ready.
exit /b 1

:build
cmake -S "%SOURCE_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%" %BACKEND_OPTIONS%
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%" --config "%CONFIG%"
exit /b %errorlevel%

:usage
echo Usage: %~nx0 ^<opengl^|directx^> [Debug^|Release^|RelWithDebInfo^|MinSizeRel]
echo Example: %~nx0 opengl Debug
exit /b 1
