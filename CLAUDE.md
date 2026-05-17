# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A C++17/Qt plugin for [COVESA DLT Viewer](https://github.com/COVESA/dlt-viewer) that adds an interactive chat interface for analyzing automotive diagnostic logs. Supports both deterministic rule-based analysis and AI-powered analysis via LLM providers (Ollama, OpenAI, LocalAI, Custom).

## Build Commands

```bash
# Standalone build (set QDLT_ROOT first)
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel

# Windows quick build (from Developer Command Prompt for VS 2022)
build_plugin.bat

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

**Build outputs:** `build/src/host_interface/Release/dltchatplugin.dll` (Windows MSVC), `build/libdltchatplugin.so` (Linux)

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

GitHub Actions (`.github/workflows/build.yml`) builds on Windows and Linux, downloads the SDK, runs tests, and uploads artifacts. Do not skip the test step when modifying core logic.
