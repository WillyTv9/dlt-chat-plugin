# Chat Log Assistant Plugin

[![License: MPL 2.0](https://img.shields.io/badge/License-MPL_2.0-blue.svg)](LICENSE)
[![Qt Version](https://img.shields.io/badge/Qt-5.15+%2F6.x-green.svg)](https://www.qt.io/)
[![DLT Viewer](https://img.shields.io/badge/DLT_Viewer-2.30.0+-orange.svg)](https://github.com/COVESA/dlt-viewer)
[![Version](https://img.shields.io/badge/Version-0.3.0-blue.svg)](.)

A chat-based log analysis plugin for COVESA DLT Viewer with rule-based analysis and optional AI (LLM) integration.

## Installation

Copy `dltchatplugin.dll` (or `libdltchatplugin.so`) to the DLT Viewer plugins folder:

- **Windows:** `%LOCALAPPDATA%\Programs\dlt-viewer\plugins\`
- **Linux:** `~/.local/share/dlt-viewer/plugins/`

Launch DLT Viewer, enable the plugin via **Settings > Plugin Settings > Chat Log Assistant**, then enable the panel via **View > Panels > Chat Log Assistant**.

## Usage

### Quick Action Buttons

14 pre-configured buttons for common log analysis tasks:

| Button | Description |
|--------|-------------|
| Errori | Find errors and fatal messages |
| Warnings | Find warning messages |
| Info | Find informational messages |
| Debug | Find debug messages |
| CAN | Find CAN bus messages |
| Security | Find auth/security issues |
| Memory | Find memory-related problems |
| Performance | Find timeout and delay issues |
| Diagnostic | Find diagnostic trouble codes |
| Patterns | Find repetitive message patterns |
| Summary | Show log statistics |
| Timeline | Show chronological view |
| Navigation | Find GPS/navigation messages |
| Help | Show available commands |

### Rule-Based Query

Type any query in the input field and press Enter or click "Invia":
- `error` — shows all error-level messages
- `warn can` — shows warnings containing "can"
- `riassumi` / `summary` — log statistics
- `indice 42` — context around index 42
- `perche` — shows context around matches

Results appear in the list; click any result to navigate to that log entry.

### AI Query

Powered by Ollama, OpenAI, or any OpenAI-compatible endpoint:

1. Click the gear icon (⚙) to open the AI configuration dialog
2. Select provider (Ollama, OpenAI, LocalAI, or Custom)
3. Enter endpoint, API key (if needed), and model name
4. Click **Test Connessione** to verify
5. Save — the AI status indicator turns green

Then type questions in the **Analisi AI** section:
- *"What caused the engine timeout?"*
- *"Show me the sequence of errors before the system crash"*
- *"Quali sono i pattern di errore piu frequenti?"*

The AI returns analysis with clickable `[index:N]` references. If the AI is unavailable, the plugin automatically falls back to the rule-based analyzer.

## Configuration

The plugin reads/writes settings via the DLT Viewer INI file. You can also place `dlt_chat_plugin.ini` next to the plugin DLL.

Example configuration:

```ini
[Analyzer]
type=rule-based
llmEndpoint=http://localhost:11434/api/generate
llmApiKey=
llmModel=qwen3.5:4b

[Behavior]
maxResults=200
llmTimeout=30000
highlightColor=#FFE680
```

## CSV Export

Two export modes:
- **Esporta CSV** — exports the current query results with full metadata (Time, Level, ECU, APID, CTID)
- **Esporta Tutto** — exports all loaded log entries

## Build

### Prerequisites
- CMake 3.16+, C++17 compiler
- Qt 5.15+ or Qt 6.x (Core, Gui, Widgets, Network)
- DLT Viewer SDK (qdlt library)

### Windows (MSVC/MinGW)
```cmd
build_plugin.bat
```

### Linux
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

The plugin is also built automatically when included in the DLT Viewer CMake project under `plugin/dlt-chat-plugin/`.

## License

Mozilla Public License 2.0 (MPL-2.0). See [LICENSE](LICENSE).
