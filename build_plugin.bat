@echo off
setlocal

:: --- INFO ---
:: Esegui questo script da un "Developer Command Prompt for VS 2022"
:: Lo trovi nel menu Start cercando: "Developer Command Prompt"
:: --- CONFIGURATION ---
:: Path to your Qt installation (e.g., C:\Qt\6.8.3\msvc2022_64)
:: If you have Qt in the PATH, you can leave this empty.
set QT_DIR=

:: Path to the DLT Viewer SDK (already set in CMakeLists.txt, but can be overridden here)
set DLT_SDK="C:\Users\andrea.franco\Desktop\DLTViewer\DLTViewer-2.30.0-STABLE-qt6.8.3-r1318_msvc2022_x64-win64"

echo [1/3] Creazione directory di build...
if not exist build mkdir build
cd build

echo [2/3] Configurazione del progetto con CMake...
:: If you have multiple Qt versions, specify -DCMAKE_PREFIX_PATH=%QT_DIR%
cmake .. -G "Visual Studio 17 2022" -A x64

if %errorlevel% neq 0 (
    echo.
    echo [ERRORE] La configurazione CMake e' fallita.
    echo Assicurati di avere CMake installato e di eseguire questo script da un "Developer Command Prompt for VS 2022".
    pause
    exit /b %errorlevel%
)

echo [3/3] Compilazione del plugin...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo.
    echo [ERRORE] La compilazione e' fallita.
    pause
    exit /b %errorlevel%
)

echo.
echo [SUCCESSO] Plugin compilato con successo!
echo Il file DLL si trova in: build\Release\dltchatplugin.dll
echo.
echo Puoi copiarlo nella cartella 'plugins' di DLT Viewer.
echo Percorso consigliato: %%LOCALAPPDATA%%\dlt-viewer\plugins\
echo.
pause
