# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A C++17/Qt plugin for [COVESA DLT Viewer](https://github.com/COVESA/dlt-viewer) that adds an interactive chat interface for analyzing automotive diagnostic logs. Supports both deterministic rule-based analysis and AI-powered analysis via LLM providers (Ollama, OpenAI, LocalAI, Custom).

## Runtime target

The plugin targets the custom **ART DLT Viewer ARTIST8 2.28** fork (`PACKAGE_VERSION 2.28.1`,
plugin interface `1.0.1`), which ships with **Qt 5.15.2 / MSVC 2019**. The plugin's interface
classes already match this SDK exactly. CI validates against the interface-identical public
**COVESA 2.28.1** SDK.

> The ARTIST8 source tree (`ART-DLT-viewer-ARTIST8-*/`) is **gitignored** — it lives locally only,
> to build the qdlt SDK the plugin links against. It must never be committed.

## Build Prerequisites (Windows)

**Critical — the ARTIST8 DLT Viewer is MSVC-built** (depends on `MSVCP140.dll`, `VCRUNTIME140.dll`).  
The plugin **must** be compiled with the same toolchain — MinGW/GCC produces an incompatible C++ ABI and cannot load.

Available toolchains:
- **VS 2019 BuildTools** (`C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools`) — confirmed working, matches the ARTIST8 Qt 5.15.2 build

**Qt 5.15.2 for MSVC 2019** must be installed separately. The MinGW Qt from MSYS2 (`C:\msys64\ucrt64`) cannot be used.

```bash
# Install Qt 5.15.2 MSVC via aqtinstall (no Qt account required)
pip install aqtinstall
python -m aqt install-qt windows desktop 5.15.2 win64_msvc2019_64 --outputdir C:\Qt
```

MSVC Qt is now at `C:\Qt\5.15.2\msvc2019_64`. (A Qt6 build still works via the CMake fallback, but
Qt 5.15.2 is the canonical target since it matches the ARTIST8 viewer.)

### Building the qdlt SDK from the ARTIST8 fork

The plugin links against the `qdlt` library/headers. Produce an SDK from the local ARTIST8 tree:

```bash
cmake -G Ninja -S ART-DLT-viewer-ARTIST8-2.28 -B ART-DLT-viewer-ARTIST8-2.28\build ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH="C:\Qt\5.15.2\msvc2019_64" ^
  -DDLT_INSTALL_SDK=ON ^
  -DCMAKE_INSTALL_PREFIX="C:\DltViewerSDK"
cmake --build ART-DLT-viewer-ARTIST8-2.28\build --config Release --parallel
cmake --install ART-DLT-viewer-ARTIST8-2.28\build
```

This yields `C:\DltViewerSDK\sdk\include\qdlt\*.h` and `C:\DltViewerSDK\sdk\lib\qdlt.lib` — point
`QDLT_ROOT` at `C:\DltViewerSDK\sdk`.

## Build Commands (Windows MSVC)

Run from a **Visual Studio 2019 Developer Command Prompt** (`VsDevCmd.bat -arch=amd64`):

```bash
# Full build (configure + compile)
set PATH=%PATH:C:\msys64=%
cmake -G Ninja -S . -B build_msvc_qt ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DQT_PREFIX=Qt5 ^
  -DCMAKE_PREFIX_PATH="C:\Qt\5.15.2\msvc2019_64" ^
  -DQDLT_ROOT="C:\DltViewerSDK\sdk" ^
  -DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=ON
cmake --build build_msvc_qt --config Release --parallel
```

> **Why `-DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=ON`?** Without it, cmake detects Vulkan headers in the MinGW include directory (`C:\msys64\ucrt64\include`) and adds that path as a system include, causing the MSVC compiler to consume MinGW's C headers (`corecrt.h`, `math.h`, `stdio.h`, etc.) which use GCC-specific extensions and fail to compile.

**Build output:** `build_msvc_qt/src/host_interface/dltchatplugin.dll` (669 KB, MSVC runtime)

### Other build commands

```bash
# Build with tests enabled
cmake -B build_tests -S tests -DQT_PREFIX=Qt5
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
- `-DQT_PREFIX=...` — override Qt version (auto-detects Qt6 then Qt5; use `Qt5` for the ARTIST8 target)
- `-DQDLT_ROOT=...` — DLT Viewer SDK path
- `-DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=ON` — **required for MSVC builds**; prevents MinGW include path contamination

## Build Commands (Linux / Ubuntu)

Install system dependencies:

```bash
sudo apt-get update
sudo apt-get install -y qtbase5-dev libqt5serialport5-dev libcups2-dev cmake ninja-build
```

Build DLT Viewer SDK from source (required; no pre-built binary for Linux). CI uses the public
COVESA `2.28.1` tag, interface-identical to the ARTIST8 fork:

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
    include/dltchat/    ← 14 public headers (all core classes)
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
- `buildEnhancedPrompt` accepts an optional `hierarchicalDigest` arg
  (and a matching `setHierarchicalDigest` setter) that gets injected
  as a `[HIERARCHICAL_DIGEST]` section before the raw entries

### AI global-context pipeline

To let the AI reason about multi-million-entry DLT files without
overflowing any context window (and without freezing the GUI), the
plugin layers a precompute + retrieval + map-reduce pipeline on top
of the LLM subsystem. All entirely in-memory, no extra dependencies,
no UI widgets — every output flows through the existing chat channel.

Components (all in `dltchat::`, `src/app_logic/{include/dltchat,src}/`):

- **`LogStatistics`, `BlockSummary`, `EcuSummary`, `TimeWindowSummary`**
  + **`HierarchicalSummaryStore`** — thread-safe container of
  pre-computed log views. `compactDigest(maxChars)` produces a
  budget-respecting panoramic textual digest fed to the LLM.

- **`LogIngestionPipeline`** — off-main-thread (QtConcurrent::run)
  worker triggered by `initFileFinish`. Stage A computes global
  statistics; Stage B partitions the log into contiguous blocks
  (default 5000 entries) and uses `DltRuleBasedAnalyzer` to
  generate a deterministic one-line summary per block.

- **`ModelProfileRegistry`** — static table mapping
  `(provider, model)` → `(maxContextTokens, reservedForResponse,
  maxConcurrent)`. Copilot is hard-capped at concurrency=2.

- **`ContextBudgetPlanner`** — sizes per-query budgets in CHARS
  (decoupled from any tokenizer). Splits 70/20/10 for global
  queries, 25/65/10 for specific ones. Supports a `deep:` prefix
  that forces the map-reduce path without any UI element.

- **`EnhancedRetriever`** — TF-IDF (over the existing inverted
  index) + recency + temporal-correlation boost + MMR-style
  diversity penalty keyed on `(category|apid|ctid)`. Drop-in
  successor to `ContextualExtractor`, still expands picked seeds
  with ±window.

- **`AiQueryPipeline`** — orchestrator that moves all per-query
  preparation (retrieval, enrichment, correlation, digest, cache
  key) off the GUI thread. `prepared()` signal hands the result
  back to `plugin_entry`, which then issues the actual
  `analyzeQueryAsync`.

- **`MapReduceAnalyzer`** — for global queries: shards the log
  along block boundaries, fans out one LLM request per shard
  (concurrency from `ModelProfileRegistry`), then reduces the
  per-shard outputs in a final consolidation request. Reuses the
  same `DltLlmAnalyzerInterface` (same rate limit, same cache,
  same OAuth bearer); distinguishes its traffic from single-shot
  via a `<<MR:nonce:idx/total>>` marker in `originalQuery`.

- **`AiErrorReporter`** — pure helper producing structured HTML
  error blocks (stage, cause, provider/model, diagnostic trace,
  actionable hint). The plugin is used by technicians: no error
  is ever generic; every failure points at a next step.

Dispatch in `plugin_entry.cpp::onAiQuerySubmitted` decides between
map-reduce and single-shot based on
`ContextBudgetPlanner::isGlobalQuery()` and the digest readiness.

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

- Qt 5.15.2 (canonical) or Qt 6.x (Core, Gui, Widgets, Network, Xml, Concurrent; Test for tests)
- DLT Viewer SDK (`qdlt`) — ARTIST8 2.28 / 2.28.x (plugin interface `1.0.1`) — located via `Findqdlt.cmake`
- No other external libraries

> **2.28 vs 2.30 API note:** `QDltFile::setManualMarkerIndices()` is a 2.30 API absent in ARTIST8
> 2.28. CMake feature-detects it (`QDLT_HAS_SET_MANUAL_MARKER_INDICES`); on 2.28, `highlightIndices()`
> falls back to selecting the matched rows in the host main table via `QItemSelectionModel`.

## CI

GitHub Actions (`.github/workflows/build.yml`) builds and tests on Windows and Linux against Qt
5.15.2 and the COVESA 2.28.1 SDK (interface-identical to the ARTIST8 fork). It is the only workflow;
the legacy GitHub Pages deployment has been retired. macOS is not in the CI matrix but builds
cleanly via the manual steps above. Do not skip the test step when modifying core logic.
