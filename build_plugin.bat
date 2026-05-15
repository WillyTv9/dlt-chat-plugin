@echo off
setlocal

echo ************************************
echo ***  Chat Log Assistant Plugin   ***
echo ************************************

if not exist build mkdir build
if not exist build (
    echo [ERROR] Failed to create build directory
    pause
    exit /b 1
)

pushd build

where ninja >nul 2>nul
if %errorlevel% equ 0 (
    echo [BUILD] Using Ninja generator
    cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DDLTCHAT_BUILD_DIST=ON
    if %errorlevel% neq 0 (
        echo [ERROR] CMake configuration failed
        popd
        pause
        exit /b 1
    )
    cmake --build . --parallel
) else (
    echo [BUILD] Using Visual Studio 2022 generator
    cmake .. -G "Visual Studio 17 2022" -A x64 -DDLTCHAT_BUILD_DIST=ON
    if %errorlevel% neq 0 (
        echo [ERROR] CMake configuration failed
        echo Ensure you run from "Developer Command Prompt for VS 2022"
        popd
        pause
        exit /b 1
    )
    cmake --build . --config Release
)

if %errorlevel% neq 0 (
    popd
    echo [ERROR] Build failed
    pause
    exit /b 1
)

popd
echo.
echo [SUCCESS] Plugin compiled successfully!
echo Output: build\src\host_interface\Release\dltchatplugin.dll
echo.
echo.
echo [BUILD] Creating distribution bundle...
cmake --build build --target dist --config Release
echo Distribution ready at: build\dist\dltchatplugin\
echo.
echo Copy to DLT Viewer plugins folder:
echo   %%LOCALAPPDATA%%\Programs\dlt-viewer\plugins\
echo.
pause
exit /b 0
