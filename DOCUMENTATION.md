# DLT Log Assistant Plugin

## Technical Documentation

**Version:** 0.2.0
**Date:** 2026-05-11
**Author:** DLT Viewer Plugin Project
**License:** MPL-2.0

---

## Table of Contents

1. [Overview](#1-overview)
2. [Architecture](#2-architecture)
3. [Installation](#3-installation)
4. [Building from Source](#4-building-from-source)
5. [Usage](#5-usage)
6. [API Reference](#6-api-reference)
7. [Configuration](#7-configuration)
8. [LLM Integration](#8-llm-integration)
9. [CSV Export](#9-csv-export)
10. [Testing](#10-testing)
11. [Troubleshooting](#11-troubleshooting)
12. [Future Enhancements](#12-future-enhancements)

---

## 1. Overview

The **DLT Log Assistant Plugin** is a plugin for the COVESA DLT Viewer application that provides intelligent log analysis through a chat-based interface. The plugin enables users to:

- Query DLT log files using natural language
- Get contextual explanations of log events
- Navigate directly to relevant log entries
- Export analysis results to CSV format
- Integrate with LLM/AI backends for advanced analysis

### Features

| Feature | Description |
|---------|-------------|
| Chat Interface | Natural language query interface |
| Log Indexing | Automatic indexing and parsing of DLT messages |
| Rule-Based Analysis | Built-in pattern matching and keyword search |
| LLM Integration | Optional AI-powered analysis via OpenAI/Ollama-compatible APIs |
| CSV Export | Export query results or entire log to CSV |
| Row Highlighting | Visual highlighting of relevant entries in DLT Viewer |

---

## 2. Architecture

### Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      DLT Viewer Application                      │
├─────────────────────────────────────────────────────────────────┤
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                  DltChatPlugin                             │  │
│  │  ┌─────────────┐  ┌──────────────┐  ┌─────────────────┐   │  │
│  │  │ ChatForm   │  │ DltExport    │  │ AnalyzerManager │   │  │
│  │  │ (UI)       │  │ (CSV Export) │  │                │   │  │
│  │  └─────────────┘  └──────────────┘  └─────────────────┘   │  │
│  │                                              │             │  │
│  │                          ┌───────────────────┴───────┐     │  │
│  │                          │  DltAnalyzerInterface     │     │  │
│  │                          │  (Abstract Base)          │     │  │
│  │                          └───────────────────┬───────┘     │  │
│  │                                          │               │  │
│  │              ┌───────────────────────────┴───────────┐   │  │
│  │              │                                     │     │  │
│  │    ┌─────────┴──────────┐         ┌────────────────┴──┐ │  │
│  │    │ DltRuleBasedAnalyzer│         │DltLlmAnalyzer       │ │  │
│  │    │                    │         │Interface            │ │  │
│  │    │ (Built-in)         │         │(OpenAI/Ollama)     │ │  │
│  │    └────────────────────┘         └─────────────────────┘ │  │
│  └───────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│  QDltFile          QDltMsg          QDltMessageDecoder         │
│  (Log Access)     (Message Parse)  (Decode Messages)            │
└─────────────────────────────────────────────────────────────────┘
```

### Class Hierarchy

```
QObject
└── DltChatPlugin (Plugin Implementation)
    ├── DltChat::Form (UI Component)
    ├── DltExport (CSV Export Utility)
    └── Analyzer Manager
        ├── DltAnalyzerInterface (Abstract Interface)
        │   ├── DltRuleBasedAnalyzer (Built-in Rule Engine)
        │   └── DltLlmAnalyzerInterface (LLM Backend)
        └── DltLlmAnalyzerFactory (Factory for LLM Providers)

Supporting:
├── DltChatAnalyzer (Legacy compatibility)
├── DltChatAnalyzer::LogEntry
└── TableModel (Highlighting integration)
```

### File Structure

```
plugin/dltchatplugin/
├── CMakeLists.txt                 # Build configuration
├── dltchatplugin.h                # Main plugin class
├── dltchatplugin.cpp              # Plugin implementation
├── dltchatplugin_test.cpp         # CLI test executable
├── chatform.h                     # Chat UI widget header
├── chatform.cpp                   # Chat UI widget implementation
├── dltchatanalyzer.h              # Legacy analyzer header
├── dltchatanalyzer.cpp            # Legacy analyzer implementation
├── dltexport.h                    # CSV export header
├── dltexport.cpp                  # CSV export implementation
├── dltanalyzerinterface.h         # Analyzer interface header
├── dltanalyzerinterface.cpp       # Analyzer interface implementation
├── dltllmanalyzerinterface.h      # LLM analyzer header
├── dltllmanalyzerinterface.cpp     # LLM analyzer implementation
└── README.md                      # User documentation
```

---

## 3. Installation

### Prerequisites

- **DLT Viewer** 2.30.0 or later
- **Qt** 5.15.x or 6.x
- **Qt Network** module (for LLM support)

### Pre-built Installation

1. Copy `libdltchatplugin.so` to the DLT Viewer plugins directory:
   ```
   Linux:   ~/.local/share/dlt-viewer/plugins/
             /usr/share/dlt-viewer/plugins/
   Windows: %LOCALAPPDATA%\dlt-viewer\plugins\
   ```

2. Launch DLT Viewer

3. Enable the plugin via **Settings → Plugin Settings**

---

## 4. Building from Source

### Dependencies

```bash
# Ubuntu/Debian
sudo apt-get install cmake build-essential qt5-qmake qtbase5-dev qt5-qml-cache
sudo apt-get install libqt5network5 libqt5widgets5

# Optional for LLM support
sudo apt-get install libqt5network5
```

### Build Steps

```bash
# 1. Navigate to DLT Viewer root
cd /path/to/dlt-viewer

# 2. Create build directory
mkdir -p build
cd build

# 3. Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# 4. Build the plugin
cmake --build . --target dltchatplugin

# 5. Build test executable (optional)
cmake --build . --target dltchatplugin_test
```

### Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `CMAKE_BUILD_TYPE` | Build type (Debug/Release) | Release |
| `DLT_PLUGIN_INSTALLATION_PATH` | Plugin install location | `DLTViewer/usr/bin/plugins` |

### Verify Build

```bash
# Check plugin was built
ls -la bin/plugins/libdltchatplugin.so

# Run CLI tests
./plugin/dltchatplugin/dltchatplugin_test /path/to/logs.txt
```

---

## 5. Usage

### Launching the Plugin

1. Open DLT Viewer
2. Load a DLT log file (`File → Open DLT File`)
3. Enable the plugin panel (`View → Panels → DLT Log Assistant`)

### Chat Interface

The plugin provides a chat-based interface with the following elements:

```
┌─────────────────────────────────────────────────────────┐
│ DLT Log Assistant                                      │
├─────────────────────────────────────────────────────────┤
│ Status: Log caricato: 765 messaggi. [Analyzer: rule-based] │
├─────────────────────────────────────────────────────────┤
│ [Chat History Area]                                    │
│                                                       │
│ Tu: mostra errori                                      │
│ DLT Assistant: Trovati 9 messaggi corrispondenti.     │
│              Livelli: error.                          │
│              Indici rilevanti: 1452, 1453, 1454...    │
├─────────────────────────────────────────────────────────┤
│ Risultati                                             │
│ ┌───────────────────────────────────────────────────┐ │
│ │ 1452: Stopping task with id: 960                  │ │
│ │ 1453: Stopping task with id: 960                  │ │
│ │ ...                                               │ │
│ └───────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────┤
│ [Input field________________________] [Invia]          │
├─────────────────────────────────────────────────────────┤
│ [Pulisci evidenziazioni]     [Esporta Risultati CSV] │
│                             [Esporta Tutto CSV]       │
└─────────────────────────────────────────────────────────┘
```

### Query Examples

| Query | Description | Example |
|-------|-------------|---------|
| Level search | Show messages by log level | `mostra errori`, `show warnings` |
| Keyword search | Find messages containing text | `timeout`, `PCTS` |
| Summary | Get log statistics | `riassumi`, `summarize` |
| Index lookup | Get specific message context | `indice 1452`, `index 100` |
| Context query | Get cause analysis | `perche si e fermato?` |

### Navigation

1. Click on a result in the **Risultati** list
2. The main DLT Viewer table scrolls to the selected entry
3. The entry is highlighted in yellow

---

## 6. API Reference

### DltAnalyzerInterface

Abstract interface for all analyzer implementations.

```cpp
struct LogEntry {
    int index;      // Log entry index
    QString time;   // Timestamp string
    QString timestamp;
    QString ecu;    // ECU ID
    QString apid;   // Application ID
    QString ctid;   // Context ID
    QString level;  // Log level (error, warn, info, debug)
    QString payload;// Message content
};

struct QueryResult {
    QString responseHtml;      // HTML formatted response
    QList<int> indices;       // Relevant log indices
    QStringList snippets;      // Context snippets
    bool success;             // Operation success
    QString errorMessage;      // Error description if failed
    qint64 processingTimeMs;  // Processing duration
};
```

#### Methods

| Method | Returns | Description |
|--------|---------|-------------|
| `name()` | QString | Analyzer name |
| `version()` | QString | Analyzer version |
| `interfaceVersion()` | QString | Interface version |
| `isAvailable()` | bool | Check if analyzer is ready |
| `supportsStreaming()` | bool | Check streaming support |
| `analyzeQuery(query, entries)` | QueryResult | Execute analysis |
| `configurationInfo()` | QString | Get configuration details |
| `supportedLanguages()` | QStringList | Supported languages |
| `configure(config)` | bool | Apply configuration |
| `currentConfiguration()` | QVariantMap | Current settings |

### DltExport

CSV export utility class.

```cpp
class DltExport {
public:
    // Export query results to CSV
    static bool exportToCsv(const QString &filePath,
                           const QList<int> &indices,
                           const QStringList &snippets,
                           const QString &query);

    // Export all log entries to CSV
    static bool exportAllEntries(const QString &filePath,
                                 const QVector<DltChatAnalyzer::LogEntry> &entries);

    // Generate CSV row with proper escaping
    static QString generateCsvRow(const QStringList &fields);
};
```

---

## 7. Configuration

### Plugin Configuration

The plugin stores configuration in the DLT Viewer settings file:

```json
{
  "dltchatplugin": {
    "analyzerType": "rule-based",
    "version": "0.2.0"
  }
}
```

### Rule-Based Analyzer

The rule-based analyzer supports:

- **Languages:** English, Italian, German, Spanish, French
- **Max Results:** 200 entries per query
- **Pattern Matching:** Keyword extraction with stopword filtering

### Configuration File Location

- **Linux:** `~/.config/dlt-viewer/dlt-viewer.ini`
- **Windows:** `%APPDATA%\dlt-viewer\dlt-viewer.ini`
- **macOS:** `~/Library/Preferences/dlt-viewer.ini`

---

## 8. LLM Integration

### Overview

The plugin supports optional LLM integration for advanced natural language understanding. When no LLM is configured, the rule-based analyzer is used automatically.

### Supported Providers

| Provider | Endpoint Format | API Key Required |
|----------|-----------------|------------------|
| OpenAI | `https://api.openai.com/v1/completions` | Yes |
| Ollama | `http://localhost:11434/api/generate` | No |
| LocalAI | `{base_url}/api/generate` | Optional |

### Configuration

```cpp
// Configure LLM analyzer programmatically
plugin->configureLlmAnalyzer(
    "http://localhost:11434/api/generate",  // endpoint
    "",                                      // apiKey (not needed for Ollama)
    "llama3"                                 // model name
);

// Switch between analyzers
plugin->setAnalyzerType("rule-based");  // Use rule-based
plugin->setAnalyzerType("llm");         // Use LLM if available
```

### LLM Prompt Format

```
Sei un assistente per l'analisi di log DLT (Diagnostic Log and Trace).
Rispondi in italiano.
Il tuo compito e' rispondere alla domanda dell'utente basandoti sui log forniti.

LOG (totale %1 entries):
[index] timestamp LEVEL APP/CTID - payload
...

DOMANDA: %query

RISPOSTA (includi gli indici dei messaggi rilevanti nel formato [index:N]):
```

### Adding a New LLM Provider

1. Implement `DltAnalyzerInterface`
2. Create factory method in `DltLlmAnalyzerFactory`
3. Register the provider in the plugin

---

## 9. CSV Export

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

### Example

```bash
# Query results
echo "#,Index,Timestamp,Level,ECU,APID,CTID,Payload,Source" > results.csv
echo "1,1452,2026-03-03 08:53:38,debug,,main,can - Stopping task" >> results.csv
```

---

## 10. Testing

### CLI Test Executable

The plugin includes a standalone test executable for validation:

```bash
# Build the test
cmake --build . --target dltchatplugin_test

# Run tests
./plugin/dltchatplugin/dltchatplugin_test /path/to/logfile.txt
```

### Test Scenarios

| Test | Description | Expected Result |
|------|-------------|-----------------|
| Parse log | Parse standard log file | Entries extracted correctly |
| Error search | Query "mostra errori" | Returns error-level entries |
| Summary | Query "riassumi" | Returns statistics |
| Index lookup | Query "indice 999999" | Returns "not found" message |
| Empty query | Query "   " | Returns "invalid query" message |
| Keyword search | Query "timeout" | Returns matching entries |
| Stress test | 20,000 entries | Completes under 10 seconds |

### Example Test Output

```
Parsed entries=765 error_level=9
Stress test entries=20000 duration_ms=2
All tests passed.
```

---

## 11. Troubleshooting

### Common Issues

#### Plugin Not Loading

**Symptom:** Plugin does not appear in plugin list

**Solution:**
```bash
# Check library dependencies
ldd bin/plugins/libdltchatplugin.so

# Verify Qt version
strings bin/plugins/libdltchatplugin.so | grep Qt
```

#### Chat Not Responding

**Symptom:** Queries do not produce responses

**Solution:**
1. Verify DLT file is loaded in main viewer
2. Check status bar for load errors
3. Enable debug logging in DLT Viewer settings

#### LLM Connection Failed

**Symptom:** "LLM non configurato" message

**Solution:**
1. Verify Ollama is running: `curl http://localhost:11434/api/tags`
2. Check endpoint URL in configuration
3. For OpenAI, verify API key is set correctly

#### Export Failed

**Symptom:** CSV export produces empty file

**Solution:**
1. Check file write permissions
2. Verify disk space is available
3. Try with absolute path instead of relative

### Debug Logging

Enable verbose logging in DLT Viewer:
```
Settings → Preferences → General → Enable Debug Logging
```

Check log output for plugin messages:
```
[DLT Log Assistant] Query executed in 2ms
[DLT Log Assistant] Found 45 matching entries
```

---

## 12. Future Enhancements

### Planned Features

| Feature | Priority | Description |
|---------|----------|-------------|
| Anomaly Detection | High | AI-powered detection of unusual patterns |
| Auto-classification | Medium | Automatic categorization of errors |
| Export Reports | Medium | Generate formatted analysis reports |
| Multi-log Correlation | Low | Compare logs from multiple sources |
| Real-time Streaming | Low | Live log analysis as new entries arrive |
| Statistics Dashboard | Low | Visual statistics and charts |
| Search Suggestions | Low | Autocomplete for common queries |

### Contributing

Contributions are welcome. Please:

1. Fork the repository
2. Create a feature branch
3. Add tests for new features
4. Submit a pull request

---

## Appendix A: CMakeLists.txt Reference

```cmake
# Build the plugin library
add_library(dltchatplugin MODULE
    dltchatplugin.cpp
    dltchatanalyzer.cpp
    chatform.cpp
    dltexport.cpp
    dltanalyzerinterface.cpp
    dltllmanalyzerinterface.cpp)

# Link required libraries
target_link_libraries(dltchatplugin qdlt 
    ${QT_PREFIX}::Widgets 
    ${QT_PREFIX}::Network)

# Register as DLT Viewer plugin
add_plugin(dltchatplugin)

# Build CLI test executable
add_executable(dltchatplugin_test
    dltchatplugin_test.cpp
    dltchatanalyzer.cpp
    dltexport.cpp
    dltanalyzerinterface.cpp)
```

## Appendix B: Plugin Metadata

```cpp
#define DLT_CHAT_PLUGIN_VERSION "0.2.0"

// Qt Plugin Metadata
Q_PLUGIN_METADATA(IID "org.genivi.DLT.DltChatPlugin")

// Plugin Interfaces
Q_INTERFACES(QDLTPluginInterface)
Q_INTERFACES(QDltPluginViewerInterface)
Q_INTERFACES(QDltPluginControlInterface)
```

## Appendix C: Version History

| Version | Date | Changes |
|---------|------|---------|
| 0.1.0 | 2026-05-11 | Initial MVP with rule-based analysis |
| 0.2.0 | 2026-05-11 | Added CSV export and LLM interface |

---

**End of Documentation**
