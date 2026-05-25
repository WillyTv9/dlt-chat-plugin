# Chat Log Assistant — Installation Guide

## Prerequisites

- **DLT Viewer** — custom **ART ARTIST8 2.28** fork (runtime target); CI validates against the
  interface-identical public [COVESA/dlt-viewer](https://github.com/COVESA/dlt-viewer) **2.28.1**
- **Qt** 5.15.2 (canonical target) or 6.x (with Core, Gui, Widgets, Network, Xml, Concurrent modules)
- **CMake** 3.16 or later
- **C++17** compatible compiler:
  - Windows: MSVC 2019 or 2022 (**not** MinGW/GCC — ABI incompatible with ARTIST8 DLT Viewer)
  - Linux: GCC 9+ or Clang 10+
  - macOS: Clang 14+ (Xcode Command Line Tools, install via `xcode-select --install`)
- **DLT Viewer SDK (qdlt)** — produced from the ARTIST8 fork (`DLT_INSTALL_SDK=ON`) or from the
  public COVESA 2.28.1 source

---

## Runtime Dependencies

The plugin requires several shared libraries at runtime. These must be findable by the DLT Viewer
host process when loading the plugin.

### Windows

| Dependency | Qt5 (5.15.2) | Qt6 (6.x) | Source |
|---|---|---|---|
| Plugin binary | `dltchatplugin.dll` | `dltchatplugin.dll` | Plugin build |
| DLT SDK | `qdlt.dll` | `qdlt.dll` | DLT Viewer SDK (`sdk/bin/`) |
| Qt Core | `Qt5Core.dll` | `Qt6Core.dll` | `windeployqt` / Qt installation |
| Qt GUI | `Qt5Gui.dll` | `Qt6Gui.dll` | `windeployqt` / Qt installation |
| Qt Widgets | `Qt5Widgets.dll` | `Qt6Widgets.dll` | `windeployqt` / Qt installation |
| Qt Network | `Qt5Network.dll` | `Qt6Network.dll` | `windeployqt` / Qt installation |
| Qt XML | `Qt5Xml.dll` | `Qt6Xml.dll` | `windeployqt` / Qt installation |
| Qt Concurrent | `Qt5Concurrent.dll` | `Qt6Concurrent.dll` | `windeployqt` / Qt installation |
| Qt platform plugin | `platforms/qwindows.dll` | `platforms/qwindows.dll` | `windeployqt` / Qt installation |
| OpenSSL | `libssl-1_1-x64.dll` | `libssl-3-x64.dll` | [slproweb.com/products/Win32OpenSSL.html](http://slproweb.com/products/Win32OpenSSL.html) |
| OpenSSL | `libcrypto-1_1-x64.dll` | `libcrypto-3-x64.dll` | [slproweb.com/products/Win32OpenSSL.html](http://slproweb.com/products/Win32OpenSSL.html) |
| MSVC runtime | `msvcp140.dll` | `msvcp140.dll` | Visual C++ Redistributable |
| MSVC runtime | `vcruntime140.dll` | `vcruntime140.dll` | Visual C++ Redistributable |
| MSVC runtime | `vcruntime140_1.dll` | `vcruntime140_1.dll` | Visual C++ Redistributable |

> If DLT Viewer ships with the same Qt version used to build the plugin, many of these DLLs
> are already available in the viewer's own directory and do not need to be duplicated.
> The OpenSSL DLLs are required by `Qt5Network.dll` / `Qt6Network.dll` for HTTPS connections
> to LLM API endpoints. If you only use rule-based analysis, QtNetwork can function without
> them, but AI features (Ollama, OpenAI, etc.) will fail.

#### Gathering DLLs with `windeployqt`

From a Visual Studio Developer Command Prompt:

```cmd
windeployqt --release --compiler-runtime build\src\host_interface\Release\dltchatplugin.dll
```

This copies all required Qt DLLs, the `platforms/qwindows.dll` platform plugin, and the MSVC
runtime DLLs into the target directory.

> **Warning:** If the DLT Viewer host uses a **different** Qt version than the one found by
> `windeployqt`, DLL conflicts will occur. Always match the Qt version used at build time.

### Linux

On Linux, shared libraries are resolved through the system dynamic linker (`ld.so`). Qt and other
dependencies are provided by system packages — only the plugin `.so` needs to be placed in the
plugins directory.

| Dependency | Qt5 | Qt6 | Source |
|---|---|---|---|
| Plugin binary | `libdltchatplugin.so` | `libdltchatplugin.so` | Plugin build |
| DLT SDK | `libqdlt.so` | `libqdlt.so` | DLT Viewer SDK installation |
| Qt Core | `libQt5Core.so.5` | `libQt6Core.so.6` | System package (`qtbase5-dev` / `qt6-base-dev`) |
| Qt GUI | `libQt5Gui.so.5` | `libQt6Gui.so.6` | System package |
| Qt Widgets | `libQt5Widgets.so.5` | `libQt6Widgets.so.6` | System package |
| Qt Network | `libQt5Network.so.5` | `libQt6Network.so.6` | System package |
| Qt XML | `libQt5Xml.so.5` | `libQt6Xml.so.6` | System package |
| Qt Concurrent | `libQt5Concurrent.so.5` | `libQt6Concurrent.so.6` | System package |
| OpenSSL | `libssl.so.1.1` | `libssl.so.3` | System package |

To verify dependencies are met:

```bash
ldd /path/to/libdltchatplugin.so | grep "not found"
```

Any library listed as "not found" indicates a missing system package.

### macOS

On macOS, dependencies are bundled in the `.dylib` or resolved via `@rpath`. Use `macdeployqt`
to gather Qt frameworks:

```bash
macdeployqt build/src/host_interface/libdltchatplugin.dylib
```

---

## Qt Version Differences

The plugin supports both Qt5 and Qt6, auto-detecting Qt6 first then falling back to Qt5.
You can force a version with `-DQT_PREFIX=Qt5` or `-DQT_PREFIX=Qt6`.

### Naming conventions

| Resource | Qt5 | Qt6 |
|---|---|---|
| CMake target prefix | `Qt5::` | `Qt6::` |
| Windows DLL | `Qt5*.dll` | `Qt6*.dll` |
| Linux shared object | `libQt5*.so.5` | `libQt6*.so.6` |
| macOS framework | `Qt5*.framework` | `Qt6*.framework` |

### OpenSSL version

| Qt5 (5.15.2) | Qt6 (6.x) |
|---|---|
| OpenSSL 1.1 (`libssl-1_1-x64.dll`) | OpenSSL 3.x (`libssl-3-x64.dll`) |
| `libcrypto-1_1-x64.dll` | `libcrypto-3-x64.dll` |

The OpenSSL version mismatch is the most common runtime error when switching between Qt5 and Qt6
builds on Windows. The wrong OpenSSL DLLs will cause `Qt5Network.dll` / `Qt6Network.dll` to fail
to load.

### Compatibility rule

**The plugin's Qt version must match the DLT Viewer host's Qt version.**

- ARTIST8 2.28 / COVESA 2.28.1 ships with **Qt 5.15.2 (MSVC 2019)**
- If you build the plugin with Qt6, DLT Viewer must also be a Qt6 build
- A Qt5-built plugin **will not load** in a Qt6 DLT Viewer and vice versa — the Qt DLLs
  have different C++ ABIs and symbol versions

### Module availability

All required modules (Core, Gui, Widgets, Network, Xml, Concurrent) exist in both Qt5 and Qt6.
The plugin does not use any deprecated Qt5 API removed in Qt6, so the same source compiles
against both without changes.

---

## Building on Windows

### Step 1: Install Visual Studio Build Tools

The ARTIST8 DLT Viewer is built with MSVC. The plugin **must** use the same toolchain —
MinGW/GCC produces an incompatible C++ ABI and will not load.

**Option A — Visual Studio 2019 Build Tools** (recommended, matches ARTIST8 exactly):

Download from [Visual Studio 2019 Build Tools](https://my.visualstudio.com/Downloads?q=visual%20studio%202019%20build%20tools).
During installation, select:
- **MSVC v142 — VS 2019 C++ x64/x86 build tools**
- **Windows 10 SDK**

**Option B — Visual Studio 2022 Build Tools** (also works):

```cmd
winget install Microsoft.VisualStudio.2022.BuildTools
```

Then install the C++ workload:

```cmd
"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vs_installer.exe" modify ^
  --installPath "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools" ^
  --add Microsoft.VisualStudio.Workload.VCTools
```

**Option C — Full Visual Studio 2019/2022 Community Edition:**
The full IDE works as well — any edition with the "Desktop development with C++" workload.

### Step 2: Install Ninja

```cmd
winget install Ninja-build.Ninja
```

Or download `ninja.exe` from [github.com/ninja-build/ninja/releases](https://github.com/ninja-build/ninja/releases)
and place it in a directory listed in `PATH`.

### Step 3: Install Qt 5.15.2 (MSVC 2019)

Use [aqtinstall](https://github.com/miurahr/aqtinstall) — no Qt account required:

```cmd
pip install aqtinstall
python -m aqt install-qt windows desktop 5.15.2 win64_msvc2019_64 --outputdir C:\Qt
```

This installs Qt 5.15.2 for MSVC 2019 to `C:\Qt\5.15.2\msvc2019_64`.

> **For Qt6:** Replace `5.15.2 win64_msvc2019_64` with your desired version and architecture,
> e.g. `6.5.0 win64_msvc2019_64`. Ensure DLT Viewer also uses Qt6.

### Step 4: Build the DLT Viewer SDK (qdlt)

The plugin links against the `qdlt` library. Build the SDK from the local ARTIST8 fork or the
public COVESA 2.28.1 source.

**From the ARTIST8 fork** (if you have the source tree):

```cmd
set QTDIR=C:\Qt\5.15.2\msvc2019_64

cmake -G Ninja -S ART-DLT-viewer-ARTIST8-2.28 -B ART-DLT-viewer-ARTIST8-2.28\build ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH="%QTDIR%" ^
  -DDLT_INSTALL_SDK=ON ^
  -DCMAKE_INSTALL_PREFIX="C:\DltViewerSDK"

cmake --build ART-DLT-viewer-ARTIST8-2.28\build --config Release --parallel
cmake --install ART-DLT-viewer-ARTIST8-2.28\build
```

This produces:
- `C:\DltViewerSDK\sdk\include\qdlt\*.h` — headers
- `C:\DltViewerSDK\sdk\lib\qdlt.lib` — import library
- `C:\DltViewerSDK\sdk\bin\qdlt.dll` — runtime DLL

**From COVESA 2.28.1** (public CI-compatible alternative):

```cmd
git clone --depth 1 --branch 2.28.1 https://github.com/COVESA/dlt-viewer.git C:\dlt-viewer-src

cmake -G Ninja -S C:\dlt-viewer-src -B C:\dlt-viewer-src\build ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH="C:\Qt\5.15.2\msvc2019_64" ^
  -DCMAKE_INSTALL_PREFIX="C:\DltViewerSDK"

cmake --build C:\dlt-viewer-src\build --config Release --parallel
cmake --install C:\dlt-viewer-src\build
```

### Step 5: Build the plugin

Open a **Visual Studio 2019/2022 Developer Command Prompt** (or run `vcvars64.bat` to set up
the environment), then:

```cmd
:: Set paths
set QTDIR=C:\Qt\5.15.2\msvc2019_64
set QDLT_ROOT=C:\DltViewerSDK\sdk
set SRC=C:\path\to\dlt-chat-plugin
set BUILD=%SRC%\build_msvc

:: Strip MinGW from PATH to prevent Vulkan header contamination
set PATH=%PATH:C:\msys64=%

:: Configure
cmake -G Ninja -S "%SRC%" -B "%BUILD%" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DQT_PREFIX=Qt5 ^
  -DCMAKE_PREFIX_PATH="%QTDIR%" ^
  -DQDLT_ROOT="%QDLT_ROOT%" ^
  -DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=ON

:: Build
cmake --build "%BUILD%" --config Release --parallel
```

**Build output:** `%BUILD%\src\host_interface\dltchatplugin.dll`

> **Why `-DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=ON`?** Without it, CMake may detect Vulkan headers
> from the MinGW include directory (e.g. `C:\msys64\ucrt64\include`) and add that path as a
> system include, causing MSVC to consume MinGW's C headers (`corecrt.h`, `math.h`, `stdio.h`)
> which use GCC-specific extensions and fail to compile.

### Step 6: Gather runtime DLLs (recommended)

```cmd
windeployqt --release --compiler-runtime "%BUILD%\src\host_interface\dltchatplugin.dll"
```

If you use AI features (HTTPS connections to LLM endpoints), also copy OpenSSL DLLs:

```cmd
copy "%QTDIR%\bin\libssl-1_1-x64.dll" "%BUILD%\src\host_interface\"
copy "%QTDIR%\bin\libcrypto-1_1-x64.dll" "%BUILD%\src\host_interface\"
```

---

## Building on Linux

### Step 1: Install build dependencies

**Ubuntu / Debian:**

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build git

# Qt5 (canonical)
sudo apt-get install -y qtbase5-dev libqt5serialport5-dev libcups2-dev

# Qt6 (optional — install alongside Qt5 or instead)
# sudo apt-get install -y qt6-base-dev libqt6serialport6-dev
```

**Fedora / RHEL:**

```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake ninja-build git

# Qt5
sudo dnf install qt5-qtbase-devel

# Qt6
# sudo dnf install qt6-qtbase-devel
```

**Arch Linux:**

```bash
sudo pacman -S base-devel cmake ninja git

# Qt5
sudo pacman -S qt5-base

# Qt6
# sudo pacman -S qt6-base
```

### Step 2: Build the DLT Viewer SDK (qdlt)

The plugin requires the `qdlt` library. Build it from the public COVESA 2.28.1 source:

```bash
git clone --depth 1 --branch 2.28.1 \
  https://github.com/COVESA/dlt-viewer.git /tmp/dlt-viewer-src

cmake -B /tmp/dlt-viewer-src/build \
  -S /tmp/dlt-viewer-src \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/opt/dlt-viewer

cmake --build /tmp/dlt-viewer-src/build --parallel $(nproc)
sudo cmake --install /tmp/dlt-viewer-src/build
```

This produces:
- `/opt/dlt-viewer/include/qdlt/qdlt.h` — headers
- `/opt/dlt-viewer/lib/libqdlt.so` — shared library
- `/opt/dlt-viewer/lib/cmake/qdlt/` — CMake config

If you need Qt6 for DLT Viewer (because your DLT Viewer build uses Qt6), pass the Qt6 prefix:

```bash
cmake -B /tmp/dlt-viewer-src/build \
  -S /tmp/dlt-viewer-src \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6 \
  -DCMAKE_INSTALL_PREFIX=/opt/dlt-viewer
```

### Step 3: Build the plugin

```bash
cd dlt-chat-plugin

cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DQDLT_ROOT=/opt/dlt-viewer

cmake --build build --parallel $(nproc)
```

> **Qt5 vs Qt6:** The root `CMakeLists.txt` auto-detects Qt6 first, then falls back to Qt5.
> To force Qt5 when Qt6 is also installed:
> ```bash
> cmake -B build -DCMAKE_BUILD_TYPE=Release -DQT_PREFIX=Qt5 -DQDLT_ROOT=/opt/dlt-viewer
> ```

**Build output:** `build/src/host_interface/libdltchatplugin.so`

### Step 4: Verify dependencies

```bash
ldd build/src/host_interface/libdltchatplugin.so | grep "not found"
```

If any dependency is listed as "not found", install the corresponding system package.
Common missing packages on minimal systems: `libgl1-mesa-dev`, `libxkbcommon-dev`.

---

## Building on macOS

macOS is not covered by CI (which runs Windows and Linux only), but the plugin builds cleanly
with Clang. A pre-built DLT Viewer SDK is not distributed for macOS; build it from source.

### Step 1: Install dependencies

```bash
brew install cmake ninja qt6
```

### Step 2: Build the DLT Viewer SDK

```bash
git clone --depth 1 --branch 2.28.1 \
  https://github.com/COVESA/dlt-viewer.git /tmp/dlt-viewer-src

cmake -B /tmp/dlt-viewer-src/build \
  -S /tmp/dlt-viewer-src \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$(brew --prefix qt6) \
  -DCMAKE_INSTALL_PREFIX=/opt/dlt-viewer

cmake --build /tmp/dlt-viewer-src/build --parallel
sudo cmake --install /tmp/dlt-viewer-src/build
```

### Step 3: Build the plugin

```bash
cd dlt-chat-plugin

cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$(brew --prefix qt6) \
  -DQDLT_ROOT=/opt/dlt-viewer

cmake --build build --parallel
```

**Build output:** `build/src/host_interface/libdltchatplugin.dylib`

> **Note:** If you have Qt5 installed via Homebrew (`brew install qt@5`), unlink it first:
> ```bash
> brew unlink qt@5 && brew link qt6
> ```

---

## Building within DLT Viewer (Alternative)

You can also build the plugin as part of the DLT Viewer source tree. This ensures the plugin
uses the same Qt version and compiler flags as the host.

### Step 1: Clone DLT Viewer

```bash
git clone https://github.com/COVESA/dlt-viewer.git
cd dlt-viewer
```

### Step 2: Copy the plugin source

```bash
# Linux / macOS
cp -r /path/to/dlt-chat-plugin plugin/dlt-chat-plugin

# Windows (PowerShell)
Copy-Item -Recurse C:\path\to\dlt-chat-plugin .\plugin\dlt-chat-plugin
```

### Step 3: Build DLT Viewer with the plugin

```bash
mkdir build && cd build

# Linux
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel

# macOS (Homebrew Qt6)
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$(brew --prefix qt6)
cmake --build . --parallel

# Windows (MSVC with Ninja)
cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

### Step 4: Output location

| Platform | Path |
|----------|------|
| Linux | `build/bin/plugins/libdltchatplugin.so` |
| macOS | `build/bin/plugins/libdltchatplugin.dylib` |
| Windows (MSVC) | `build/bin/plugins/dltchatplugin.dll` |

---

## Installation

### Windows

Copy the DLL to the DLT Viewer plugins folder:

```cmd
copy build\src\host_interface\Release\dltchatplugin.dll "%LOCALAPPDATA%\Programs\dlt-viewer\plugins\"
```

If you gathered runtime DLLs with `windeployqt`, copy them alongside the plugin:

```cmd
xcopy build\src\host_interface\* "%LOCALAPPDATA%\Programs\dlt-viewer\plugins\" /E
```

### Linux

```bash
cp build/src/host_interface/libdltchatplugin.so ~/.local/share/dlt-viewer/plugins/
```

### macOS

```bash
cp build/src/host_interface/libdltchatplugin.dylib \
   ~/Library/Application\ Support/dlt-viewer/plugins/
```

(Path may vary depending on the DLT Viewer package. Check DLT Viewer → Preferences for the correct
plugins directory on your installation.)

---

## Enabling the Plugin in DLT Viewer

1. Launch DLT Viewer
2. Go to **Settings → Plugin Settings**
3. Check **Chat Log Assistant** to enable
4. Go to **View → Panels → Chat Log Assistant**
5. The chat panel appears docked in the main window

---

## Verifying the Installation

1. Open a DLT log file in DLT Viewer
2. The status bar in the plugin should show:
   `Loaded N msgs | CarPlay: X | AA: Y`
3. Click the **Help** quick action button
4. You should see the list of available commands

If the plugin does not appear in the Plugin Settings menu, verify:
- The DLL/SO/dylib is in the correct plugins directory
- The plugin matches the DLT Viewer version (ARTIST8 2.28 / 2.28.1)
- Qt libraries are available in the system PATH (Windows) or linker path (Linux)

---

## Running Tests

```bash
# Configure with tests enabled
cmake -B build -DCMAKE_BUILD_TYPE=Release -DDLTCHAT_BUILD_TESTS=ON

# Build
cmake --build build --config Release

# Run all tests
ctest --test-dir build -C Release --output-on-failure

# Run a single test suite
ctest --test-dir build -C Release -R rulebased --output-on-failure
```

> **Windows:** When running tests from a non-developer shell, set `PATH` to include the Qt and
> MSVC runtime DLL directories:
> ```cmd
> set PATH=C:\Qt\5.15.2\msvc2019_64\bin;%PATH%
> ctest --test-dir build -C Release --output-on-failure
> ```

Note: Tests require the Qt Test module.

---

## Distribution Bundle

### Building the bundle

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DDLTCHAT_BUILD_DIST=ON
cmake --build build --config Release
cmake --build build --target dist
```

Output in `build/dist/dltchatplugin/` containing:

| File | Description |
|------|-------------|
| `dltchatplugin.dll` / `.so` / `.dylib` | Plugin binary |
| `dlt_chat_plugin.ini` | Configuration file |
| `automotive_filters_example.json` | Example user filters |
| `README.md` | Build and usage guide |
| `ARCHITECTURE.md` | Technical architecture |
| `LICENSE` | MPL 2.0 license |
| `category_registry.json` | Quick-action button definitions |
| `presets/` | Preset filter definitions |

### Adding runtime dependencies

**Windows:**

```cmd
cd build\dist\dltchatplugin
windeployqt --release --compiler-runtime dltchatplugin.dll
```

This adds all required Qt DLLs, the `platforms\qwindows.dll` platform plugin, and MSVC runtime
DLLs. For AI features, also add OpenSSL:

```cmd
copy "C:\Qt\5.15.2\msvc2019_64\bin\libssl-1_1-x64.dll" .
copy "C:\Qt\5.15.2\msvc2019_64\bin\libcrypto-1_1-x64.dll" .
```

Copy the configuration file (rename from example):

```cmd
copy dlt_chat_plugin.ini.example dlt_chat_plugin.ini
```

**Linux:**

System packages handle Qt dependencies. Verify with `ldd`:

```bash
ldd build/dist/dltchatplugin/libdltchatplugin.so | grep "not found"
```

Copy and rename the configuration:

```bash
cp dlt_chat_plugin.ini.example build/dist/dltchatplugin/dlt_chat_plugin.ini
```

### Configuration reference

The plugin reads `dlt_chat_plugin.ini` at startup. See the annotated example file for all
available keys. Key sections:

| Section | Key | Description |
|---|---|---|
| `[Analyzer]` | `type` | `rule-based` (default) or `llm` |
| `[Analyzer]` | `llmEndpoint` | URL of the LLM API endpoint |
| `[Analyzer]` | `llmApiKey` | API key for the LLM provider |
| `[Analyzer]` | `llmModel` | Model name (e.g. `llama3.2:1b`, `gpt-4o-mini`) |
| `[Analyzer]` | `bulkAnalysisEnabled` | Classify all entries on load via LLM |
| `[Behavior]` | `maxResults` | Maximum results per query (default: 1000) |
| `[Behavior]` | `highlightColor` | Highlight color in hex (default: `#FFE680`) |
| `[Behavior]` | `userFiltersPath` | Path to custom JSON filter file |
| `[Filters]` | `dlpPath` | Path to DLT Viewer filter file for Quick Actions |
| `[Live]` | `aiRefreshSec` | AI digest refresh interval in live mode (default: 30, 0 disables) |

---

## CMake Options Reference

| Option | Description | Default |
|---|---|---|
| `-DQT_PREFIX=Qt5` or `=Qt6` | Force Qt version | Auto-detected (Qt6 → Qt5) |
| `-DQDLT_ROOT=/path` | DLT Viewer SDK path | Env `QDLT_ROOT` |
| `-DDLTCHAT_BUILD_TESTS=ON` | Enable unit tests | OFF |
| `-DDLTCHAT_BUILD_DIST=ON` | Enable distribution target | OFF |
| `-DDLTCHAT_BUILD_TOOLS=ON` | Build audit tools | OFF |
| `-DDLT_ENABLE_ASAN=ON` | AddressSanitizer (Debug, non-MSVC) | OFF |
| `-DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=ON` | Prevent MinGW header contamination (MSVC) | OFF |

---

## Troubleshooting

| Problem | Solution |
|---|---|
| `Could not find qdlt` | Set `QDLT_ROOT` to the DLT Viewer SDK directory containing `include/qdlt/qdlt.h` |
| `Could not find Qt5Config` / `Could not find Qt6Config` | Install Qt or set `-DQT_PREFIX=Qt6`/`=Qt5` and point `CMAKE_PREFIX_PATH` to the Qt installation |
| `fatal error` with MSVC C headers (Windows) | Ensure you are in a **VS Developer Command Prompt**, not a MinGW/MSYS2 shell. Add `-DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=ON` |
| Link errors on Windows | Use MSVC, not MinGW/GCC. MSVC and MinGW have incompatible C++ ABIs |
| Plugin not loading | The plugin and viewer must use the **same Qt version and compiler ABI**. A Qt5-MSVC-plugin will not load in a Qt6-viewer or a MinGW-viewer |
| Plugin not visible in menu | Verify the DLL is in the correct plugins directory. Check DLT Viewer → Preferences for the path |
| `The code execution cannot proceed because Qt5Core.dll was not found` (Windows) | Qt DLLs are missing from `PATH`. Run `windeployqt` on the plugin, or copy Qt DLLs to the viewer directory |
| `The code execution cannot proceed because libssl-1_1-x64.dll was not found` (Windows) | OpenSSL is missing. Copy `libssl-1_1-x64.dll` and `libcrypto-1_1-x64.dll` from the Qt `bin/` directory |
| `libdltchatplugin.so: undefined symbol` (Linux) | The plugin was built with a different Qt version than DLT Viewer. Rebuild with matching Qt version |
| Build fails on Linux with `cannot find -lGL` | Install `libgl1-mesa-dev` (Debian/Ubuntu) or `libglvnd-devel` (Fedora/RHEL) |
| `brew link qt6` fails on macOS | Run `brew unlink qt@5 && brew link qt6` |
| Plugin not loading on macOS | Remove the quarantine flag: `xattr -d com.apple.quarantine libdltchatplugin.dylib` |
| MSVC runtime errors on Windows (missing `msvcp140.dll`) | Install [Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) |

---

## Uninstalling

Remove the plugin file from the DLT Viewer plugins directory:

- **Windows**: Delete `dltchatplugin.dll` from `%LOCALAPPDATA%\Programs\dlt-viewer\plugins\`
- **Linux**: Delete `libdltchatplugin.so` from `~/.local/share/dlt-viewer/plugins/`
- **macOS**: Delete `libdltchatplugin.dylib` from `~/Library/Application Support/dlt-viewer/plugins/`
