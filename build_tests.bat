@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64
if errorlevel 1 exit /b 1

set PATH=%PATH:C:\Users\alessio.brillo\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.MCF.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin=%

cmake -G "Visual Studio 17 2022" -A x64 -S "%~dp0." -B "%~dp0build_tests" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH="C:\Qt\6.8.3\msvc2022_64" ^
  -DQDLT_ROOT="%USERPROFILE%\AppData\Local\Programs\dlt-viewer\sdk" ^
  -DDLTCHAT_BUILD_TESTS=ON ^
  -DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=ON
if errorlevel 1 exit /b 1

cmake --build "%~dp0build_tests" --config Release --parallel
if errorlevel 1 exit /b 1
