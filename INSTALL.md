# Installation Guide

## Table of Contents

- [Prerequisites](#prerequisites)
- [Installation Methods](#installation-methods)
- [Building from Source](#building-from-source)
- [Post-Installation](#post-installation)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### System Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| OS | Ubuntu 20.04 / Windows 10 / macOS 11 | Latest stable |
| RAM | 4 GB | 8 GB+ |
| Disk | 500 MB | 1 GB |
| Display | 1024x768 | 1920x1080 |

### Required Software

- **DLT Viewer** 2.30.0 or later ([Download](https://github.com/COVESA/dlt-viewer/releases))
- **Qt Framework** 5.15.x or 6.x

---

## Installation Methods

### Method 1: Pre-built Binary (Linux)

1. Download the latest release from the GitHub releases page
2. Copy the plugin to the DLT Viewer plugins directory:
   ```bash
   cp libdltchatplugin.so ~/.local/share/dlt-viewer/plugins/
   ```
3. Make it executable (optional):
   ```bash
   chmod +x ~/.local/share/dlt-viewer/plugins/libdltchatplugin.so
   ```

### Method 2: Pre-built Binary (Windows)

1. Download the latest release from the GitHub releases page
2. Copy the DLL to the plugins directory:
   ```cmd
   copy dltchatplugin.dll %LOCALAPPDATA%\dlt-viewer\plugins\
   ```

### Method 3: Build from Source

See the [Building from Source](#building-from-source) section below.

---

## Building from Source

### Step 1: Install Build Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install cmake build-essential qt5-qmake qtbase5-dev
sudo apt-get install libqt5network5 libqt5widgets5 libqt5xmlpatterns5
```

**Fedora/RHEL:**
```bash
sudo dnf install cmake gcc-c++ qt5-qtbase-devel qt5-qtnetworkauth
```

**Windows:**
- Install [Visual Studio 2019+](https://visualstudio.microsoft.com/)
- Install [Qt](https://www.qt.io/download-qt-installer)
- Install [CMake](https://cmake.org/download/)

### Step 2: Obtain DLT Viewer Source

```bash
git clone --recursive https://github.com/COVESA/dlt-viewer.git
cd dlt-viewer
```

### Step 3: Copy Plugin Source

```bash
# Copy plugin to DLT Viewer plugins directory
cp -r /path/to/dltchatplugin plugin/
```

### Step 4: Build

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the plugin
cmake --build . --target dltchatplugin

# Optional: Build tests
cmake --build . --target dltchatplugin_test
```

### Step 5: Install

**Linux:**
```bash
# Copy plugin to user plugins directory
cp bin/plugins/libdltchatplugin.so ~/.local/share/dlt-viewer/plugins/

# Or system-wide
sudo cp bin/plugins/libdltchatplugin.so /usr/share/dlt-viewer/plugins/
```

**Windows:**
```cmd
copy bin\plugins\dltchatplugin.dll %LOCALAPPDATA%\dlt-viewer\plugins\
```

---

## Post-Installation

### Enable Plugin in DLT Viewer

1. Launch DLT Viewer
2. Go to **Settings** → **Plugin Settings**
3. Find **DLT Log Assistant** in the list
4. Check the **Enabled** checkbox
5. Click **OK**

### Configure Plugin

1. Go to **View** → **Panels**
2. Enable **DLT Log Assistant** panel
3. The plugin panel will appear on the right side

### Verify Installation

- Status bar should show "DLT Log Assistant: Connected"
- Panel should display "Log loaded: X messages"

---

## Troubleshooting

### Plugin Not Appearing in Plugin List

**Cause:** Plugin not compatible with installed DLT Viewer version

**Solution:**
1. Verify DLT Viewer version (≥ 2.30.0)
2. Check Qt version compatibility
3. Rebuild plugin from source for your DLT Viewer version

### Plugin Loads But Doesn't Show Panel

**Cause:** Panel not enabled in view settings

**Solution:**
1. Go to **View** → **Panels**
2. Check **DLT Log Assistant**
3. Panel should now appear

### No Response to Queries

**Cause:** DLT file not loaded or analyzer issue

**Solution:**
1. Load a DLT file in the main viewer
2. Check status bar for log count
3. Try "riassumi" or "summarize" command first
4. Check plugin logs for errors

### CSV Export Not Working

**Cause:** Permission or path issue

**Solution:**
1. Try absolute path instead of relative
2. Check write permissions for target directory
3. Verify disk space available

### LLM Integration Fails

**Cause:** LLM endpoint not accessible

**Solution:**
1. Verify Ollama/API is running
2. Check endpoint URL configuration
3. For OpenAI, verify API key is valid
4. Test endpoint with curl:
   ```bash
   curl http://localhost:11434/api/tags
   ```

---

## Uninstall

### Linux

```bash
rm ~/.local/share/dlt-viewer/plugins/libdltchatplugin.so
```

### Windows

```cmd
del %LOCALAPPDATA%\dlt-viewer\plugins\dltchatplugin.dll
```

---

## Support

For additional help:
- Check [DOCUMENTATION.md](DOCUMENTATION.md) for detailed technical docs
- Open an issue on GitHub for bugs or questions
- See [README.md](README.md) for more resources