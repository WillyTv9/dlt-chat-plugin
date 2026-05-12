# Chat Log Assistant — Installation Guide

## Prerequisites

- **DLT Viewer** 2.30.0 or later
  ([COVESA/dlt-viewer](https://github.com/COVESA/dlt-viewer))
- **Qt** 5.15+ or 6.x (with Core, Gui, Widgets, Network modules)
- **CMake** 3.16 or later
- **C++17** compatible compiler (MSVC 2019+, GCC 9+, Clang 10+)
- **DLT Viewer SDK** (qdlt) — included in DLT Viewer source

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
| Windows (MinGW) | `build/bin/plugins/libdltchatplugin.dll` |
| Windows (MSVC) | `build/bin/plugins/Release/dltchatplugin.dll` |

---

## Option B: Building Standalone

### Step 1: Install dependencies

Ensure Qt and the DLT Viewer SDK (qdlt) are installed and discoverable.

Set the `QDLT_ROOT` environment variable to the DLT Viewer installation
directory, or to the DLT Viewer source directory.

### Step 2: Configure and build

```bash
cd dlt-chat-plugin
mkdir build && cd build

# Linux
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel

# Windows (Visual Studio 2022)
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --parallel --config Release
```

### Step 3: Build output

The compiled plugin appears as `dltchatplugin.dll` (Windows) or
`libdltchatplugin.so` (Linux) in the `build/Release/` directory.

---

## Option C: Windows Quick Build

1. Open a **Developer Command Prompt for VS 2022**
2. Navigate to the plugin directory
3. Run: `build_plugin.bat`

This script auto-detects the build environment and compiles the plugin.

---

## Installation

### Windows

Copy the DLL to the DLT Viewer plugins folder:

```cmd
copy dltchatplugin.dll "%LOCALAPPDATA%\Programs\dlt-viewer\plugins\"
```

### Linux

```bash
cp libdltchatplugin.so ~/.local/share/dlt-viewer/plugins/
```

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
- The DLL/SO is in the correct plugins directory
- The plugin matches the DLT Viewer version (2.30.0+)
- Qt libraries are available in the system PATH

---

## Running Tests

```bash
cd tests
mkdir build && cd build
cmake ..
cmake --build .
ctest
```

Note: Tests require Qt Test module and can be built independently
of the DLT Viewer SDK (they test the analysis logic, not the DLT
file access).

---

## Uninstalling

Remove the plugin file from the DLT Viewer plugins directory:
- **Windows**: Delete `dltchatplugin.dll` from `%LOCALAPPDATA%\Programs\dlt-viewer\plugins\`
- **Linux**: Delete `libdltchatplugin.so` from `~/.local/share/dlt-viewer/plugins/`
