# Chat Log Assistant Plugin

[![License: MPL 2.0](https://img.shields.io/badge/License-MPL_2.0-blue.svg)](LICENSE)
[![Qt Version](https://img.shields.io/badge/Qt-5.15+%2F6.x-green.svg)](https://www.qt.io/)
[![DLT Viewer](https://img.shields.io/badge/DLT_Viewer-2.30.0+-orange.svg)](https://github.com/COVESA/dlt-viewer)
[![Version](https://img.shields.io/badge/Version-0.7.0-blue.svg)](.)
[![CI](https://img.shields.io/github/actions/workflow/status/WillyTv9/dlt-chat-plugin/build.yml?branch=main&label=CI)](https://github.com/WillyTv9/dlt-chat-plugin/actions)
[![Presentation](https://img.shields.io/badge/📊-Presentation-blue?style=flat&labelColor=555)](https://WillyTv9.github.io/dlt-chat-plugin/)

A chat-based log analysis plugin for COVESA DLT Viewer with rule-based analysis and optional AI (LLM) integration.

Plugin per l'analisi dei log DLT con interfaccia chat, analisi rule-based e supporto AI opzionale.

---

## Build & Installation

See [INSTALL.md](INSTALL.md) for complete step-by-step instructions covering:

| Platform | Toolchain | Qt | DLT Viewer SDK |
|----------|-----------|-----|-----------------|
| **Windows** | VS 2019/2022 BuildTools MSVC | Qt 6.8.3 MSVC (via aqtinstall) | Pre-built binary from GitHub Releases |
| **Linux** | GCC 9+ / Clang 10+ | System Qt6 (`apt`) | Built from source (v2.30.0) |
| **macOS** | Xcode Command Line Tools (Clang 14+) | Homebrew Qt6 | Built from source (v2.30.0) |

Three build options available:
- **Option A** — Build within DLT Viewer (recommended)
- **Option B** — Build standalone
- **Option C** (Windows) — Quick batch script
- **Option D** (macOS) — SDK from source

## Releases

Official releases are available on [GitHub Releases](https://github.com/WillyTv9/dlt-chat-plugin/releases).
Artifacts included:
- `dltchatplugin-vX.Y.Z-win64.zip`
- `dltchatplugin-vX.Y.Z-linux64.tar.gz`

---

## Installation

Copy the plugin binary to the DLT Viewer plugins folder. See [INSTALL.md](INSTALL.md)
for platform-specific paths (Windows, Linux, macOS) and enabling instructions.

Then launch DLT Viewer, enable the plugin via **Settings > Plugin Settings > Chat Log Assistant**, and the panel via **View > Panels > Chat Log Assistant**.

---

## Usage

### Quick Action Buttons (76)

The plugin provides 76 quick action buttons arranged in 13 rows (Row 0–12) in a scrollable grid.
See [USER_MANUAL.md](USER_MANUAL.md) for the complete button reference table.

| Row | Count | Purpose |
|-----|-------|---------|
| 0 | 6 | Severity levels (Fatal, Error, Warn, Info, Debug, Verbose) |
| 1 | 7 | Core & Boot (System, Diag, GPS, Startup, Power, Session, Security) |
| 2 | 6 | Connectivity (Network, WiFi, Ethernet, BT, USB Stack, USB Media) |
| 3 | 5 | Hardware & Vehicle (Hardware, Vehicle, CAN, Sensors, Storage) |
| 4 | 8 | Media & HMI (FM, DAB, Audio, Video, Routing, Meta, UI, Voice) |
| 5 | 6 | Performance & Maps (Perf, Stats, Time, OTA, Maps, Location) |
| 6 | 8 | Smartphone Projection (CarPlay, AndroidAuto, Projection, …) |
| 7 | 8 | Smart Filters (Critical, Err Only, GPS Err, …) |
| 8 | 6 | Advanced Projection (CP Err, AA Err, Wireless, …) |
| 9 | 6 | Analysis & Help (Summary, Timeline, Pattern, Categorizza, Help, Categories) |
| 10 | 4 | Managers (SysMgr, Launcher, MsgBus, ConMgr) |
| 11 | 2 | Subsystems (TTS, Haptic) |
| 12 | 4 | Special Filters (GPS Epoch, Sys Fatal, All Fatal, Auth Err) |

### Chat Commands / Comandi chat

| Command | Description |
|---------|-------------|
| `error` / `warn` / `info` / `debug` / `verbose` | Filter by log level |
| `carplay` / `androidauto` | Filter by automotive domain |
| `can` / `security` / `memory` / `performance` / `diagnostic` / `gps` | Filter by category |
| `summary` / `riassumi` | Show log statistics |
| `timeline` / `cronologia` | Show chronological list |
| `pattern` | Show repeated message groups |
| `categorizza` / `categorize` / `classifica` | Classify errors into 7 categories |
| `keywords` / `categorie` | Show available keywords |
| `help` / `aiuto` | Show available commands |

**Combined queries:** `error can`, `warn carplay`, `info security timeout`

### AI Query

1. Click the gear icon (⚙) to open configuration
2. Select provider (Ollama, OpenAI, LocalAI, Custom)
3. Enter endpoint, API key (if needed), model name
4. Click **Test Connection** to verify
5. Save — green indicator confirms AI is ready

Type questions in the **AI** section:
- *"What caused the timeout?"*
- *"Show me the sequence of errors before the crash"*
- *"Quali sono i pattern di errore piu frequenti?"*

If AI is unavailable, falls back to rule-based analysis automatically.

### Automotive Domain Classification & Error Categorization

See [ARCHITECTURE.md](ARCHITECTURE.md) for domain detection details and [USER_MANUAL.md](USER_MANUAL.md)
for the complete error categorization table (7 categories).

---

## Configuration

The plugin reads `dlt_chat_plugin.ini` at startup. See the annotated example
[`dlt_chat_plugin.ini.example`](dlt_chat_plugin.ini.example) for all configuration keys.

**Key sections:**
- `[Analyzer]` — LLM provider, endpoint, model, tokens, temperature, timeout, bulk analysis
- `[Behavior]` — max results, highlight color, user filters path

Load custom regex-based highlight filters via the **Filters** button. See
[`automotive_filters_example.json`](automotive_filters_example.json) for schema.

---

## CI / CD

GitHub Actions builds and tests on every push/PR:

- **Windows** (MSVC 2022, Qt 6.7, DLT Viewer SDK 2.30.0)
- **Linux** (GCC, Qt 6.7, qdlt built from source)

Test, build, and upload artifacts. Release workflow also creates distribution bundles (`win64.zip` / `linux64.tar.gz`).

---

## Architecture

See [ARCHITECTURE.md](ARCHITECTURE.md) for detailed technical documentation.

---

## License

Mozilla Public License 2.0 (MPL-2.0). See [LICENSE](LICENSE).
