@echo off
setlocal

:: Build script for DLT Chat Log Assistant Plugin
:: Supports MSVC (Visual Studio 2022) and MinGW (MSYS2 ucrt64)

echo ************************************
echo ***  Chat Log Assistant Plugin   ***
echo ************************************

if not exist build mkdir build
cd build

:: Detect available generators
where ninja >nul 2>nul
if %errorlevel% equ 0 (
    echo [BUILD] Using Ninja generator
    cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
    if %errorlevel% neq 0 (
        echo [ERROR] CMake configuration failed
        pause
        exit /b 1
    )
    cmake --build . --parallel
) else (
    echo [BUILD] Using Visual Studio 2022 generator
    cmake .. -G "Visual Studio 17 2022" -A x64
    if %errorlevel% neq 0 (
        echo [ERROR] CMake configuration failed
        echo Ensure you run from "Developer Command Prompt for VS 2022"
        pause
        exit /b 1
    )
    cmake --build . --config Release
)

if %errorlevel% neq 0 (
    echo [ERROR] Build failed
    pause
    exit /b 1
)

echo.
echo [SUCCESS] Plugin compiled successfully!
echo Output: build\dltchatplugin.dll (MSVC) / build\libdltchatplugin.dll (MinGW)
echo.
echo Copy to DLT Viewer plugins folder:
echo   %%LOCALAPPDATA%%\Programs\dlt-viewer\plugins\
echo.
pause
