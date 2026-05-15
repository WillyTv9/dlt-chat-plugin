@echo off
setlocal EnableExtensions

rem Requires a MinGW GCC 11.x toolchain matching Qt 6.7 (see cmake warning).
rem WinLibs GCC 15.x will crash tests at runtime (ABI mismatch with Qt DLLs).

set "ROOT=%~dp0"
set "QT_BIN=%ROOT%6.7.3\mingw_64\bin"
if not exist "%QT_BIN%\Qt6Core.dll" (
    echo [ERROR] Qt not found at %QT_BIN%
    exit /b 1
)

set "PATH=%QT_BIN%;%PATH%"
set "QT_PLUGIN_PATH=%ROOT%6.7.3\mingw_64\plugins"

if not exist "%ROOT%build\tests\tests.exe" (
    echo [ERROR] Build tests first: cmake -B build -DDLTCHAT_BUILD_TESTS=ON ^&^& cmake --build build --target tests
    exit /b 1
)

"%ROOT%6.7.3\mingw_64\bin\windeployqt.exe" --no-translations --no-system-d3d-compiler --no-opengl-sw "%ROOT%build\tests\tests.exe" >nul 2>&1

cd /d "%ROOT%build"
ctest --output-on-failure
exit /b %ERRORLEVEL%
