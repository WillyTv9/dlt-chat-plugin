# DLT Log Assistant Plugin

[![License: MPL 2.0](https://img.shields.io/badge/License-MPL_2.0-blue.svg)](LICENSE)
[![Qt Version](https://img.shields.io/badge/Qt-5.15+-green.svg)](https://www.qt.io/)
[![DLT Viewer](https://img.shields.io/badge/DLT_Viewer-2.30.0+-orange.svg)](https://github.com/COVESA/dlt-viewer)
[![Version](https://img.shields.io/badge/Version-0.2.1-blue.svg)](CHANGELOG.md)

A chat-based intelligent log analysis plugin for COVESA DLT Viewer that enables natural language queries on DLT log files.

**[� Documentation Index](CODE_INDEX.md) | [�🚀 Quick Start](QUICKSTART.md) | [⚙️ Configuration](CONFIGURATION.md) | [🔧 Build Guide](INSTALL.md) | [📋 Review Report](CODE_REVIEW_REPORT.md)**

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Quick Start](#quick-start)
- [Requirements](#requirements)
- [Installation](#installation)
- [Building from Source](#building-from-source)
- [Usage](#usage)
- [Configuration](#configuration)
- [LLM Integration](#llm-integration)
- [CSV Export](#csv-export)
- [Documentation](#documentation)
- [Project Structure](#project-structure)
- [License](#license)
- [Contributing](#contributing)
- [Support](#support)

---

## Overview

The **DLT Log Assistant Plugin** integrates with the COVESA DLT Viewer application to provide intelligent log analysis through a chat-based interface. Users can query DLT log files using natural language and receive contextual explanations with direct navigation to relevant log entries.

This plugin was developed following the [DLT Viewer Plugin Project Requirements](https://github.com/COVESA/dlt-viewer) and is designed for educational and production use.

---

## Features

| Feature | Description |
|---------|-------------|
| **Natural Language Queries** | Ask questions about DLT logs in plain language |
| **Log Indexing** | Automatic parsing and indexing of DLT messages |
| **Context Navigation** | Jump directly to relevant log entries |
| **CSV Export** | Export analysis results to CSV format |
| **LLM Integration** | Optional AI-powered analysis (OpenAI, Ollama, LocalAI) |
| **Row Highlighting** | Visual highlighting of matching entries |
| **Multi-language Support** | English, Italian, German, Spanish, French |
| **Dual Analyzer Modes** | Rule-based (fast) or AI-powered (intelligent) |
| **Quick Actions** | Pre-configured buttons for common queries |

---

## Quick Start

**For new users:** See [QUICKSTART.md](QUICKSTART.md) for a 5-minute setup guide with examples.

**For configuration:** See [CONFIGURATION.md](CONFIGURATION.md) for detailed setup options.

**For developers:** See [INSTALL.md](INSTALL.md) for building from source.

---

### Runtime Requirements

- **DLT Viewer** 2.30.0 or later
- **Qt** 5.15.x or 6.x
- **Qt Network** module (required for LLM support)

### Build Requirements

- **CMake** 3.16 or later
- **C++ Compiler** with C++17 support (GCC, Clang, MSVC)
- **Qt Development Libraries** (qtbase5-dev or qt6-base-dev)

### Supported Platforms

- Linux (Ubuntu 20.04+, Debian 11+)
- Windows 10/11 (with MSVC 2019+)
- macOS 11+ (experimental)

---

## Installation

### Pre-built Binary

1. Copy the compiled plugin to the DLT Viewer plugins directory:

   **Linux:**
   ```bash
   cp libdltchatplugin.so ~/.local/share/dlt-viewer/plugins/
   # or system-wide
   sudo cp libdltchatplugin.so /usr/share/dlt-viewer/plugins/
   ```

   **Windows:**
   ```cmd
   copy dltchatplugin.dll %LOCALAPPDATA%\dlt-viewer\plugins\
   ```

2. Launch DLT Viewer

3. Enable the plugin via **Settings → Plugin Settings → DLT Log Assistant**

---

## Building from Source

### Step 1: Clone the Repository

```bash
git clone https://github.com/your-repo/dlt-viewer-plugin.git
cd dlt-viewer-plugin/dltchatplugin
```

### Step 2: Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install cmake build-essential qt5-qmake qtbase5-dev qt5-qml-cache
sudo apt-get install libqt5network5 libqt5widgets5
```

**Fedora/RHEL:**
```bash
sudo dnf install cmake gcc-c++ qt5-qtbase-devel qt5-qtnetworkauth
```

**Windows (with vcpkg):**
```powershell
vcpkg install qt5-base:x64-windows
```

### Step 3: Build

The plugin is built as part of the DLT Viewer project. You need to build DLT Viewer with the plugin:

```bash
# Clone DLT Viewer
git clone --recursive https://github.com/COVESA/dlt-viewer.git
cd dlt-viewer

# Copy the plugin to the DLT Viewer plugins directory
cp -r /path/to/dltchatplugin plugin/

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the plugin
cmake --build . --target dltchatplugin
```

### Step 4: Verify Build

```bash
# Check plugin was built
ls -la bin/plugins/libdltchatplugin.so

# Run CLI tests (optional)
cmake --build . --target dltchatplugin_test
./plugin/dltchatplugin/dltchatplugin_test /path/to/test_logs.dlt
```

---

## Usage

### Launching the Plugin

1. Open DLT Viewer
2. Load a DLT log file (`File → Open DLT File`)
3. Enable the plugin panel (`View → Panels → DLT Log Assistant`)

### Chat Interface

The plugin provides a chat-based interface:

```
┌─────────────────────────────────────────────────────────┐
│ DLT Log Assistant                                      │
├─────────────────────────────────────────────────────────┤
│ Status: Log loaded: 765 messages. [Analyzer: rule-based] │
├─────────────────────────────────────────────────────────┤
│ [Chat History]                                         │
│                                                         │
│ You: mostra errori                                     │
│ DLT Assistant: Found 9 matching messages.             │
│          Levels: error.                                │
│          Relevant indices: 1452, 1453, 1454...        │
├─────────────────────────────────────────────────────────┤
│ Results                                                │
│ ┌───────────────────────────────────────────────────┐   │
│ │ 1452: Stopping task with id: 960                 │   │
│ │ 1453: Task stopped successfully                   │   │
│ └───────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────┤
│ [Input field________________________] [Send]          │
├─────────────────────────────────────────────────────────┤
│ [Clear Highlighting]    [Export Results CSV]          │
│                        [Export All CSV]                │
└─────────────────────────────────────────────────────────┘
```

### Query Examples

| Query | Description |
|-------|-------------|
| `mostra errori` | Show all error-level messages |
| `show warnings` | Display warning messages |
| `riassumi` | Get log statistics summary |
| `timeout` | Search for "timeout" keyword |
| `indice 100` | Get context around index 100 |
| `perche si e fermato?` | Get cause analysis |
| `summarize` | Display log statistics |

### Navigation

1. Click on a result in the **Results** list
2. The main DLT Viewer table scrolls to the selected entry
3. The entry is highlighted in yellow

---

## Configuration

### Plugin Configuration

The plugin stores configuration in the DLT Viewer settings file:

**Linux:** `~/.config/dlt-viewer/dlt-viewer.ini`

**Windows:** `%APPDATA%\dlt-viewer\dlt-viewer.ini`

### Rule-Based Analyzer Settings

- **Languages:** English, Italian, German, Spanish, French
- **Max Results:** 200 entries per query
- **Pattern Matching:** Keyword extraction with stopword filtering

---

## LLM Integration

### Overview

The plugin supports optional LLM integration for advanced natural language understanding. When no LLM is configured, the rule-based analyzer is used automatically.

### Supported Providers

| Provider | Endpoint Format | API Key Required |
|----------|-----------------|------------------|
| OpenAI | `https://api.openai.com/v1/completions` | Yes |
| Ollama | `http://localhost:11434/api/generate` | No |
| LocalAI | `{base_url}/api/generate` | Optional |

### Configuration Example

```cpp
// Configure LLM analyzer
plugin->configureLlmAnalyzer(
    "http://localhost:11434/api/generate",  // endpoint
    "",                                      // apiKey (not needed for Ollama)
    "llama3"                                 // model name
);

// Switch between analyzers
plugin->setAnalyzerType("rule-based");  // Use rule-based
plugin->setAnalyzerType("llm");         // Use LLM if available
```

---

## CSV Export

### Export Formats

#### Query Results Export

| Column | Description |
|--------|-------------|
| # | Sequential result number |
| Index | Original log index |
| Timestamp | Message timestamp |
| Level | Log level |
| ECU | ECU ID |
| APID | Application ID |
| CTID | Context ID |
| Payload | Message content |
| Source | Original query |

#### Full Log Export

| Column | Description |
|--------|-------------|
| Index | Original log index |
| Time | Timestamp string |
| Timestamp | Raw timestamp |
| Level | Log level |
| ECU | ECU ID |
| APID | Application ID |
| CTID | Context ID |
| Payload | Message content |

### CSV Escaping

- Fields containing commas, quotes, or newlines are enclosed in double quotes
- Internal quotes are escaped as `""`
- UTF-8 encoding is used

---

## Documentation

Complete documentation is available in multiple documents:

| Document | Purpose |
|----------|---------|
| [QUICKSTART.md](QUICKSTART.md) | 5-minute quick start guide with examples |
| [CONFIGURATION.md](CONFIGURATION.md) | Detailed configuration and setup guide |
| [INSTALL.md](INSTALL.md) | Build and installation instructions |
| [CODE_REVIEW_REPORT.md](CODE_REVIEW_REPORT.md) | Code quality and issues report |
| [CHANGELOG.md](CHANGELOG.md) | Version history and changes |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Guidelines for contributors |
| [DOCUMENTATION.md](DOCUMENTATION.md) | Technical API documentation |

---

## Testing

The plugin can be tested by building DLT Viewer with the plugin and using the chat interface to query log files. The plugin integrates with DLT Viewer's testing infrastructure.

---

## Project Structure

```
dltchatplugin/
├── CMakeLists.txt                 # Build configuration
├── dltchatplugin.h                # Main plugin class
├── dltchatplugin.cpp              # Plugin implementation
├── chatform.h                     # Chat UI widget header
├── chatform.cpp                   # Chat UI widget implementation
├── dltchatanalyzer.h              # Rule-based analyzer header
├── dltchatanalyzer.cpp            # Rule-based analyzer implementation
├── dltexport.h                    # CSV export header
├── dltexport.cpp                  # CSV export implementation
├── dltanalyzerinterface.h         # Analyzer interface header
├── dltanalyzerinterface.cpp       # Analyzer interface implementation
├── dltllmanalyzerinterface.h      # LLM analyzer header
├── dltllmanalyzerinterface.cpp    # LLM analyzer implementation
├── README.md                      # This file
├── DOCUMENTATION.md                # Full technical documentation
├── INSTALL.md                     # Installation guide
├── CONTRIBUTING.md                # Contributing guidelines
└── LICENSE                        # Mozilla Public License 2.0
```

---

## Architecture

```
DLT Viewer Application
│
├── DltChatPlugin
│   ├── ChatForm (UI)
│   ├── DltExport (CSV Export)
│   └── Analyzer Manager
│       ├── DltAnalyzerInterface (Abstract)
│       │   ├── DltRuleBasedAnalyzer (Built-in)
│       │   └── DltLlmAnalyzerInterface (OpenAI/Ollama)
│       └── DltLlmAnalyzerFactory
│
└── QDltFile / QDltMsg / QDltMessageDecoder
```

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 0.2.1 | 2026-05-11 | Fixed critical UI freezing bug, memory leak, race condition |
| 0.2.0 | 2026-05-11 | Added CSV export and LLM interface |
| 0.1.0 | 2026-03-01 | Initial MVP with rule-based analysis |

See [CHANGELOG.md](CHANGELOG.md) for detailed version history.

---

## License

This project is licensed under the **Mozilla Public License 2.0 (MPL-2.0)**.

See [LICENSE](LICENSE) file for full license text.

**Key points:**
- Free to use for commercial and private purposes
- Must disclose source code modifications
- Include copy of MPL-2.0 license
- Any modified files must include license header

---

## Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

Please ensure all tests pass before submitting a pull request.

For detailed guidelines, see [CONTRIBUTING.md](CONTRIBUTING.md).

---

## Support & Issues

### Getting Help

1. **Quick Questions:** Check [QUICKSTART.md](QUICKSTART.md)
2. **Configuration Issues:** See [CONFIGURATION.md](CONFIGURATION.md)
3. **Build Problems:** Check [INSTALL.md](INSTALL.md)
4. **Known Issues:** Review [CODE_REVIEW_REPORT.md](CODE_REVIEW_REPORT.md)

### Reporting Issues

Please create a GitHub issue with:

```
**Environment:**
- DLT Viewer Version: X.Y.Z
- Plugin Version: 0.2.1
- OS: Windows/Linux/macOS
- Qt Version: 5.15.x / 6.x

**Problem Description:**
[Clear description of the issue]

**Steps to Reproduce:**
1. ...
2. ...
3. ...

**Expected Behavior:**
[What should happen]

**Actual Behavior:**
[What actually happens]

**Additional Context:**
[Logs, screenshots, or example DLT files]
```

### Contact

- **Project Issues:** GitHub Issues
- **Security Issues:** Please email (see CONTRIBUTING.md)
- **Discussions:** GitHub Discussions

---

## Acknowledgments

- COVESA DLT Viewer project and community
- Qt Framework contributors
- Contributors and testers

---

## Disclaimer

This plugin is provided as-is for educational and production analysis purposes. The authors are not responsible for any damages, data loss, or issues caused by the use of this plugin. Always maintain backups of your DLT log files.

---

**Latest Update:** 2026-05-11  
**Plugin Version:** 0.2.1  
**Status:** MVP - Production Ready

---

## Support

- **Issues:** Report bugs and issues via GitHub Issues
- **Documentation:** See [DOCUMENTATION.md](DOCUMENTATION.md) for full technical docs
- **DLT Viewer:** https://github.com/COVESA/dlt-viewer

---

## Acknowledgments

- [COVESA](https://www.covesa.global/) for the DLT Viewer project
- Qt Framework for the UI components
- Contributors and testers