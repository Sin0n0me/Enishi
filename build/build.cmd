@echo off
setlocal EnableExtensions

set "PROJECT_ROOT=%~dp0.."
set "SOURCE_DIR=%PROJECT_ROOT%\src"
set "BACKEND=%~1"
set "CONFIG=%~2"
set "TOOLCHAIN=%~3"

if "%BACKEND%"=="" goto :usage
if "%CONFIG%"=="" set "CONFIG=Debug"
if "%TOOLCHAIN%"=="" set "TOOLCHAIN=auto"

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
if /I "%TOOLCHAIN%"=="auto" goto :auto
if /I "%TOOLCHAIN%"=="visualstudio" goto :visualstudio
if /I "%TOOLCHAIN%"=="clang" goto :clang
if /I "%TOOLCHAIN%"=="gcc" goto :gcc
goto :usage

:auto
set "AUTO_FALLBACK=ON"
goto :visualstudio

:visualstudio
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :visualstudio_unavailable

"%VSWHERE%" -all -products * -format json | findstr.exe /r /c:"productLineVersion.*18" >nul
if not errorlevel 1 goto :visualstudio_2026

"%VSWHERE%" -all -products * -format json | findstr.exe /r /c:"productLineVersion.*17" >nul
if not errorlevel 1 goto :visualstudio_2022
goto :visualstudio_unavailable

:visualstudio_2026
set "GENERATOR=Visual Studio 18 2026"
goto :build_visualstudio

:visualstudio_2022
set "GENERATOR=Visual Studio 17 2022"
goto :build_visualstudio

:visualstudio_unavailable
if /I "%AUTO_FALLBACK%"=="ON" goto :clang
echo Visual Studio 18 2026 or Visual Studio 17 2022 was not found.
exit /b 1

:build_visualstudio
set "BUILD_DIR=%~dp0%BACKEND%\visualstudio"
cmake -S "%SOURCE_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%" %BACKEND_OPTIONS%
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%" --config "%CONFIG%"
exit /b %errorlevel%

:clang
where.exe ninja >nul 2>nul
if errorlevel 1 goto :clang_unavailable

where.exe clang >nul 2>nul
if errorlevel 1 goto :clang_unavailable

set "BUILD_DIR=%~dp0%BACKEND%\clang"
cmake -S "%SOURCE_DIR%" -B "%BUILD_DIR%" -G Ninja %BACKEND_OPTIONS% -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=%CONFIG%
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%"
exit /b %errorlevel%

:clang_unavailable
if /I "%AUTO_FALLBACK%"=="ON" goto :gcc
echo Clang or Ninja was not found. Install LLVM and Ninja and add them to PATH.
exit /b 1

:gcc
where.exe ninja >nul 2>nul
if errorlevel 1 goto :gcc_unavailable

where.exe gcc >nul 2>nul
if errorlevel 1 goto :gcc_unavailable

where.exe g++ >nul 2>nul
if errorlevel 1 goto :gcc_unavailable

set "BUILD_DIR=%~dp0%BACKEND%\gcc"
cmake -S "%SOURCE_DIR%" -B "%BUILD_DIR%" -G Ninja %BACKEND_OPTIONS% -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=%CONFIG%
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%"
exit /b %errorlevel%

:gcc_unavailable
echo GCC or Ninja was not found. Install GCC and Ninja and add them to PATH.
exit /b 1

:vulkan_not_available
echo Vulkan build support has not been added to CMake yet.
echo Add a :vulkan section with its CMake options when the renderer is ready.
exit /b 1

:usage
echo Usage: %~nx0 ^<opengl^|directx^> [Debug^|Release^|RelWithDebInfo^|MinSizeRel] [auto^|visualstudio^|clang^|gcc]
echo Example: %~nx0 opengl Debug
echo Example: %~nx0 directx Release clang
exit /b 1
