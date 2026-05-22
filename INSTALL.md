# Chat Log Assistant — Installation Guide

## Prerequisites

- **DLT Viewer** — custom **ART ARTIST8 2.28** fork (runtime target); CI validates against the
  interface-identical public [COVESA/dlt-viewer](https://github.com/COVESA/dlt-viewer) **2.28.1**
- **Qt** 5.15.2 (canonical target) or 6.x (with Core, Gui, Widgets, Network, Xml modules)
- **CMake** 3.16 or later
- **C++17** compatible compiler (MSVC 2019, GCC 9+, Clang 10+)
  - macOS: Xcode Command Line Tools (`xcode-select --install`) provide Clang 14+; install Homebrew from https://brew.sh
- **DLT Viewer SDK** (qdlt) — produced by the ARTIST8 fork build (`DLT_INSTALL_SDK=ON`) or taken
  from a 2.28.1 source/binary distribution

---

## Option A: Building within DLT Viewer (Recommended)

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
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$(brew --prefix qt6) ..
cmake --build . --parallel

# Windows (MSVC)
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --parallel

# Windows (MinGW / MSYS2)
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel
```

The plugin will be built automatically as part of DLT Viewer.

### Step 4: Output location

| Platform | Path |
|----------|------|
| Linux | `build/bin/plugins/libdltchatplugin.so` |
| macOS | `build/bin/plugins/libdltchatplugin.dylib` |
| Windows (MSVC) | `build/bin/plugins/Release/dltchatplugin.dll` |
| Windows (MinGW) | `build/bin/plugins/libdltchatplugin.dll` |

---

## Option B: Building Standalone

### Step 1: Install dependencies

Ensure Qt and the DLT Viewer SDK (qdlt) are installed and discoverable.

Set the `QDLT_ROOT` environment variable to the DLT Viewer installation
directory (where `include/qdlt/qdlt.h` and `lib/qdlt.lib` reside).

**macOS**:
```bash
brew install cmake ninja qt6
```

### Step 2: Configure and build

```bash
cd dlt-chat-plugin
mkdir build && cd build

# Linux
export QDLT_ROOT=/path/to/dlt-viewer
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel

# macOS
export QDLT_ROOT=/opt/dlt-viewer
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$(brew --prefix qt6) \
  -DQDLT_ROOT=$QDLT_ROOT
cmake --build . --parallel

# Windows (Visual Studio 2022)
set QDLT_ROOT=C:\path\to\dlt-viewer\sdk
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --parallel --config Release
```

### Step 3: Build output

| Platform | Path |
|----------|------|
| Linux | `build/src/host_interface/libdltchatplugin.so` |
| macOS | `build/src/host_interface/libdltchatplugin.dylib` |
| Windows (MSVC) | `build/src/host_interface/Release/dltchatplugin.dll` |

---

## Option C: Windows Quick Build

1. Open a **Developer Command Prompt for VS 2019**
2. Navigate to the plugin directory
3. Run: `build_plugin.bat`

This script auto-detects Ninja or Visual Studio and compiles the plugin.

---

## Option D: macOS — Build DLT Viewer SDK from Source

macOS is not covered by CI (which runs Windows and Linux only), but the plugin
builds cleanly with Clang. A pre-built DLT Viewer SDK is not distributed for
macOS; build it from source.

### Step 1: Install dependencies

```bash
brew install cmake ninja qt6
```

### Step 2: Build DLT Viewer SDK

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

### Step 4: Output

`build/src/host_interface/libdltchatplugin.dylib`

---

## Installation

### Windows

Copy the DLL to the DLT Viewer plugins folder:

```cmd
copy build\src\host_interface\Release\dltchatplugin.dll "%LOCALAPPDATA%\Programs\dlt-viewer\plugins\"
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

(Path may vary depending on the DLT Viewer package. Check DLT Viewer → Preferences for the correct plugins directory on your installation.)

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
- Qt libraries are available in the system PATH

---

## Running Tests

```bash
# Configure with tests enabled
cmake -B build -DCMAKE_BUILD_TYPE=Release -DDLTCHAT_BUILD_TESTS=ON

# Build
cmake --build build --config Release

# Run tests
ctest --test-dir build -C Release --output-on-failure
```

### Running a single test

```bash
cd build_tests && ctest -R rulebased --output-on-failure
```

Note: Tests require the Qt Test module.

---

## Distribution Bundle

```bash
# Build the plugin first
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Build the distribution target
cmake --build build --target dist

# Output in build/dist/dltchatplugin/ containing:
#   dltchatplugin.dll / .so   — Plugin binary
#   dlt_chat_plugin.ini       — Configuration file
#   automotive_filters_example.json — Example user filters
#   README.md                 — Build and usage guide
#   ARCHITECTURE.md           — Technical architecture
#   LICENSE                   — MPL 2.0 license
#   presets/                  — Preset filter definitions
```

---

## CMake Options Reference

| Option | Description | Default |
|--------|-------------|---------|
| `-DQT_PREFIX=Qt5` or `=Qt6` | Force Qt version | Auto-detected (Qt6 → Qt5) |
| `-DQDLT_ROOT=/path` | DLT Viewer SDK path | Env `QDLT_ROOT` |
| `-DDLTCHAT_BUILD_TESTS=ON` | Enable unit tests | OFF |
| `-DDLTCHAT_BUILD_DIST=ON` | Enable distribution target | OFF |
| `-DDLT_ENABLE_ASAN=ON` | AddressSanitizer (Debug, non-MSVC) | OFF |

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| `Could not find qdlt` | Set `QDLT_ROOT` to DLT Viewer SDK directory |
| `Could not find Qt5Config` | Install Qt or set `-DQT_PREFIX=Qt6` |
| Link errors on Windows | Ensure you use the same compiler as the DLT Viewer SDK |
| Plugin not loading | Check DLT Viewer version (ARTIST8 2.28 / 2.28.1) and that it was built with the same Qt+compiler |
| Plugin not visible in menu | Verify DLL is in the correct plugins directory |
| Build fails on Linux | Install `libgl1-mesa-dev` or equivalent |
| `brew link qt6` fails on macOS | Run `brew unlink qt@5 && brew link qt6` |
| Plugin not loading on macOS | Remove the quarantine flag: `xattr -d com.apple.quarantine libdltchatplugin.dylib` |

---

## Uninstalling

Remove the plugin file from the DLT Viewer plugins directory:
- **Windows**: Delete `dltchatplugin.dll` from `%LOCALAPPDATA%\Programs\dlt-viewer\plugins\`
- **Linux**: Delete `libdltchatplugin.so` from `~/.local/share/dlt-viewer/plugins/`
- **macOS**: Delete `libdltchatplugin.dylib` from `~/Library/Application Support/dlt-viewer/plugins/`
