# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A C++17/Qt plugin for [COVESA DLT Viewer](https://github.com/COVESA/dlt-viewer) that adds an interactive chat interface for analyzing automotive diagnostic logs. Supports both deterministic rule-based analysis and AI-powered analysis via LLM providers (Ollama, OpenAI, LocalAI, Custom).

## Build Prerequisites (Windows)

**Critical — DLT Viewer's Qt6 is MSVC-built** (depends on `MSVCP140.dll`, `VCRUNTIME140.dll`).  
The plugin **must** be compiled with the same toolchain — MinGW/GCC produces an incompatible C++ ABI and cannot load.

Available toolchains:
- **VS 2019 BuildTools** (`C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools`) — confirmed working
- **VS 2022 Enterprise** at non-standard path `C:\Program Files\Microsoft Visual Studio\18\Enterprise` — VC++ workload NOT installed

**Qt 6.8.3 for MSVC** must be installed separately. The MinGW Qt6 from MSYS2 (`C:\msys64\ucrt64`) cannot be used.

```bash
# Install Qt6 MSVC via aqtinstall (no Qt account required)
pip install aqtinstall
python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 --outputdir C:\Qt6
```

MSVC Qt6 is now at `C:\Qt\6.8.3\msvc2022_64`.

## Build Commands (Windows MSVC)

Run from a **Visual Studio 2019 Developer Command Prompt** (`VsDevCmd.bat -arch=amd64`):

```bash
# Full build (configure + compile)
set PATH=%PATH:C:\msys64=%
cmake -G Ninja -S . -B build_msvc_qt ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH="C:\Qt\6.8.3\msvc2022_64" ^
  -DQDLT_ROOT="%LOCALAPPDATA%\Programs\dlt-viewer\sdk" ^
  -DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=ON
cmake --build build_msvc_qt --config Release --parallel
```

> **Why `-DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=ON`?** Without it, cmake detects Vulkan headers in the MinGW include directory (`C:\msys64\ucrt64\include`) and adds that path as a system include, causing the MSVC compiler to consume MinGW's C headers (`corecrt.h`, `math.h`, `stdio.h`, etc.) which use GCC-specific extensions and fail to compile.

**Build output:** `build_msvc_qt/src/host_interface/dltchatplugin.dll` (669 KB, MSVC runtime)

### Quick build (batch script)

```batch
:: build_msvc.bat — run from any shell
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64
if errorlevel 1 exit /b 1
set PATH=%PATH:C:\msys64=%
cmake -G Ninja -S "%~dp0." -B "%~dp0build_msvc_qt" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH="C:\Qt\6.8.3\msvc2022_64" ^
  -DQDLT_ROOT="%LOCALAPPDATA%\Programs\dlt-viewer\sdk" ^
  -DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=ON
if errorlevel 1 exit /b 1
cmake --build "%~dp0build_msvc_qt" --config Release --parallel
```

### Other build commands

```bash
# Build with tests enabled
cmake -B build_tests -S tests -DQT_PREFIX=Qt6
cmake --build build_tests --config Release

# Run all tests
cd build_tests && ctest --output-on-failure

# Run a single test suite
cd build_tests && ctest -R rulebased --output-on-failure

# Build distribution bundle
cmake --build build --target dist
```

**Key CMake options:**
- `-DDLTCHAT_BUILD_TESTS=ON` — enable unit tests
- `-DDLTCHAT_BUILD_DIST=ON` — enable distribution packaging
- `-DDLT_ENABLE_ASAN=ON` — AddressSanitizer for Debug builds
- `-DQT_PREFIX=...` — override Qt version (auto-detects Qt6 then Qt5)
- `-DQDLT_ROOT=...` — DLT Viewer SDK path
- `-DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=ON` — **required for MSVC builds**; prevents MinGW include path contamination

## Build Commands (Linux / Ubuntu)

Install system dependencies:

```bash
sudo apt-get update
sudo apt-get install -y libqt6serialport6-dev libcups2-dev cmake ninja-build
```

Build DLT Viewer SDK from source (required; no pre-built binary for Linux):

```bash
git clone --depth 1 --branch v2.30.0 \
  https://github.com/COVESA/dlt-viewer.git /tmp/dlt-viewer-src

cmake -B /tmp/dlt-viewer-src/build \
  -S /tmp/dlt-viewer-src \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/opt/dlt-viewer

cmake --build /tmp/dlt-viewer-src/build --parallel $(nproc)
sudo cmake --install /tmp/dlt-viewer-src/build
```

Build the plugin:

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DQDLT_ROOT=/opt/dlt-viewer

cmake --build build --parallel
```

**Build output:** `build/src/host_interface/dltchatplugin.so`

## Build Commands (macOS)

Install dependencies via Homebrew:

```bash
brew install cmake ninja qt6
```

Build DLT Viewer SDK from source (same flow as Linux, with macOS-specific paths):

```bash
git clone --depth 1 --branch v2.30.0 \
  https://github.com/COVESA/dlt-viewer.git /tmp/dlt-viewer-src

cmake -B /tmp/dlt-viewer-src/build \
  -S /tmp/dlt-viewer-src \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$(brew --prefix qt6) \
  -DCMAKE_INSTALL_PREFIX=/opt/dlt-viewer

cmake --build /tmp/dlt-viewer-src/build --parallel
sudo cmake --install /tmp/dlt-viewer-src/build
```

Build the plugin:

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$(brew --prefix qt6) \
  -DQDLT_ROOT=/opt/dlt-viewer

cmake --build build --parallel
```

**Build output:** `build/src/host_interface/libdltchatplugin.dylib`

## Architecture

The codebase uses a **Bridge Pattern** that cleanly separates the DLT Viewer host interface from the core application logic.

```
DLT Viewer (host)
    └── DltChatPlugin  [plugin_entry.h/.cpp]
            ├── ChatForm (UI)  [chatform.h/.cpp]
            └── App Logic  [src/app_logic/]
                    ├── LogStore + LogIndex          (data layer)
                    ├── DltRuleBasedAnalyzer         (command dispatch)
                    ├── DltLlmAnalyzerInterface      (AI integration)
                    ├── BulkAnalyzer                 (background classification)
                    ├── ContextualExtractor          (context window around hits)
                    ├── ConversationManager          (multi-turn AI history)
                    ├── TemporalCorrelator           (time-window correlation)
                    ├── FibexEnricher                (FIBEX XML metadata)
                    ├── UserFilterManager            (regex highlight filters)
                    └── ExportEngine                 (CSV export)
```

### Directory layout

```
src/
  app_logic/
    include/dltchat/    ← 13 public headers (all core classes)
    src/                ← implementations
  host_interface/       ← plugin_entry, chatform, options dialog, results model
  resources/presets/    ← preset filter JSON definitions
tests/
  test_*.cpp/.h         ← 10 Qt Test classes (one per module)
  test_data/            ← sample FIBEX XML, filter JSON
cmake/                  ← compiler_warnings, GetGitHash, version template
dist/                   ← distribution packaging
```

### Key data flow

1. **Ingestion**: `initFileStart` → `initMsg` (×N) → `initFileFinish`  
   Each message is decoded into a `LogEntry`, classified by `AutomotiveLogParser`, stored in `LogStore`, and indexed in `LogIndex`.

2. **Query routing** (in `plugin_entry.cpp`):  
   - Preset match / special command → `DltRuleBasedAnalyzer`  
   - Level/category filter → rule-based  
   - Free text → inverted index lookup (`LogIndex`)  
   - AI query → async `DltLlmAnalyzerInterface` (falls back to rule-based if unavailable)

3. **Result delivery**: All results arrive as `QueryResult` (contains HTML, matched indices, snippets, timing). The plugin highlights those indices in DLT Viewer.

### LLM subsystem

`DltLlmAnalyzerInterface` (`llm_analyzer_interface.cpp`, ~750 lines) handles:
- Async and sync analysis paths
- Circuit breaker: 5 failures → 60 s open window
- Token-bucket rate limiter (10 tokens, 1/s refill)
- LRU response cache (1000 entries in analyzer, 10 000 in plugin)
- Retry with exponential backoff + jitter

### Rule-based analyzer commands

Handled keywords in `DltRuleBasedAnalyzer`: `error`, `warn`, `info`, `debug`, `verbose`, `CAN`, `security`, `memory`, `performance`, `diagnostic`, `GPS`, `pattern`, `summary`, `timeline`, `categorizza`, `help`. Error categorization groups into 7 Italian-named categories: Comunicazione, Memoria, Sicurezza, Configurazione, Hardware, Timeout, Protocollo.

### Namespace

All app-logic classes live in the `dltchat::` namespace.

## Testing

Tests use **Qt Test** framework. Each module has its own test class in `tests/`. Test data lives in `tests/test_data/`.

To add a test: create `tests/test_<module>.cpp`, register it in `tests/CMakeLists.txt`, and follow the existing pattern (inherit `QObject`, use `QTEST_MAIN`).

## Configuration

The plugin reads `dlt_chat_plugin.ini` at startup. The example file `dlt_chat_plugin.ini.example` documents all keys. User highlight filters are loaded from a JSON file whose schema is shown in `automotive_filters_example.json`.

## Dependencies

- Qt 5.15+ or Qt 6.x (Core, Gui, Widgets, Network, Xml; Test for tests)
- DLT Viewer SDK (`qdlt`) ≥ 2.30.0 — located via `Findqdlt.cmake`
- No other external libraries

## CI

GitHub Actions (`.github/workflows/build.yml`) builds on Windows and Linux in the CI matrix. macOS is not in the CI matrix but builds cleanly via the manual steps above (Homebrew + source SDK build). Do not skip the test step when modifying core logic.
