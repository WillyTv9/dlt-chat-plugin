@echo off
set "DLT_DIR=%USERPROFILE%\AppData\Local\Programs\dlt-viewer"
set "TEST_DIR=C:\Users\alessio.brillo\Desktop\dlt-chat-plugin\build_tests\tests\Release"
set "PATH=%DLT_DIR%;%PATH%"
cd /d "%TEST_DIR%"
echo Running tests from: %CD%
echo PATH includes: %DLT_DIR%
tests.exe -v1
echo EXIT CODE: %ERRORLEVEL%
