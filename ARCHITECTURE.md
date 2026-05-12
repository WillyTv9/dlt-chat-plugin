# Chat Log Assistant — Technical Architecture

## Overview

Chat Log Assistant is a Qt-based plugin for COVESA DLT Viewer that provides
chat-based log analysis. It supports both rule-based (local) and LLM/AI
(remote) analysis strategies.

---

## 1. High-Level Architecture

```
┌──────────────────────────────────────────────────────────┐
│                   DLT Viewer Application                  │
│  ┌────────────────────────────────────────────────────┐  │
│  │              DltChatPlugin (Plugin)                 │  │
│  │                                                     │  │
│  │  ┌──────────┐  ┌──────────────┐  ┌──────────────┐  │  │
│  │  │ Chat UI  │  │  Analyzer    │  │  DLT File    │  │  │
│  │  │ (Form)   │◄─┤  (Strategy)  │◄─┤  Access      │  │  │
│  │  └──────────┘  └──────────────┘  └──────────────┘  │  │
│  │       │               │                              │  │
│  │       ▼               ▼                              │  │
│  │  ┌──────────┐  ┌──────────────┐                     │  │
│  │  │ Results  │  │  Rule/LLM   │                     │  │
│  │  │ List     │  │  Analyzer   │                     │  │
│  │  └──────────┘  └──────────────┘                     │  │
│  └────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────┘
```

### Key Design Patterns

- **Plugin Pattern**: Qt plugin system (`Q_PLUGIN_METADATA`, `Q_INTERFACES`)
- **Strategy Pattern**: `DltAnalyzerInterface` base with `DltRuleBasedAnalyzer`
  and `DltLlmAnalyzerInterface` implementations
- **Observer Pattern**: Signals/slots for UI updates, async results
- **Circuit Breaker**: For LLM failure handling (5 failures → 60s open)
- **Token Bucket**: Rate limiter for LLM API calls
- **LRU Cache**: Response caching in both plugin and LLM analyzer

---

## 2. Component Breakdown

### 2.1 Core Plugin (`dltchatplugin.h/.cpp`)

**Purpose**: Entry point, DLT Viewer lifecycle, query routing.

**Key Responsibilities**:
- Implements `QDLTPluginInterface`, `QDltPluginViewerInterface`,
  `QDltPluginControlInterface`
- Lifecycle: `initFileStart` → `initMsg` (×N) → `initFileFinish`
- Message ingestion with inverted index building
- Query routing: presets → special commands → liveSearch → analyzer
- AI availability checks with exponential backoff
- Index highlighting via `dltFile->setManualMarkerIndices()`
- User filter integration and CSV export coordination

**Data Flow**:
```
DLT Viewer → initMsg() → ingestMessage()
  ↓
LogEntry + Inverted Index ← entries[]
  ↓
onQuerySubmitted() → route → liveSearch() | m_analyzer->analyzeQuery()
  ↓
form->appendMessage() + form->setResults() + highlightIndices()
```

### 2.2 Chat UI (`chatform.h/.cpp`)

**Purpose**: User-facing chat interface.

**Components**:
- Status bar (file info, domain stats, AI status)
- 20 quick action buttons (3×7 grid with per-button colors)
- Chat history (`QTextBrowser`)
- Results list (`QListWidget`) with clickable items
- Rule-based query input + Send button
- AI query input + Ask AI button + progress bar
- Bottom bar: Clear, Filters (load), CSV, CSV All

**Palette-aware**: Automatically adjusts colors for dark/light themes.

### 2.3 Analyzer Interface (`dltanalyzerinterface.h/.cpp`)

**Core Data Structures**:
- `LogEntry`: index, time, timestamp, ecu, apid, ctid, level, payload,
  domain, event
- `QueryResult`: responseHtml, indices, snippets, success, errorMessage,
  processingTimeMs, usedAi

**DltRuleBasedAnalyzer**:
- Commands: help, timeline, summary, pattern, keywords, categorizza
- Level filtering: error/fatal, warn, info, debug, verbose
- Domain filtering: carplay, androidauto
- Category filtering: CAN, security, memory, performance, diagnostic, GPS
- Combined queries: "error can", "warn carplay"
- Pattern detection: duplicate message grouping
- Error categorization: classifies error/fatal entries into 7 categories (Comunicazione, Memoria, Sicurezza, Configurazione, Hardware, Timeout, Protocollo)
- Statistics: level counts, top contexts, repeated messages, domains

### 2.4 LLM Analyzer (`dltllmanalyzerinterface.h/.cpp`)

**Purpose**: AI-powered log analysis via HTTP API.

**Supported Providers**: Ollama, OpenAI, LocalAI, Custom

**Features**:
- Async analysis (`analyzeQueryAsync`) with signal/slot result delivery
- Sync analysis (`analyzeQuery`) with retry + exponential backoff + jitter
- Circuit breaker: 5 failures → 60s open state
- Rate limiter: token bucket (10 tokens, 1 token/sec refill)
- LRU cache: 1000 entries in analyzer, 10000 in plugin
- Prompt building with log context and optional user filter context
- JSON response parsing for Ollama and OpenAI formats
- Index extraction from LLM responses (`[index:N]` syntax + fallback)

### 2.5 Bulk Analyzer (`dltbulkanalyzer.h/.cpp`)

**Purpose**: Batch log classification using LLM.

**Architecture**: Worker thread (`QThread`) processing logs in chunks.

**Features**:
- Chunked processing with pause/resume/cancel
- Chunk classification query: category, summary, tags per entry
- Search by tag or category
- Progress reporting via signals

### 2.6 Export (`dltexport.h/.cpp`)

**Purpose**: CSV export of query results and all entries.

**Features**:
- Proper CSV escaping (quotes, commas, newlines)
- UTF-8 encoding (Qt5: QTextStream codec, Qt6: QStringConverter)
- Filtered export: headers + index, time, timestamp, level, ECU, APID,
  CTID, domain, payload, source query
- Full export: all entries with same metadata

### 2.7 Automotive Log Parser (`automotivelogparser.h/.cpp`)

**Purpose**: Domain-specific classification for automotive protocols.

**Domains**:
- **CarPlay**: Detected via APID (`com.apple.carplay`), payload keywords
  (`iap2`, `AirPlay`)
- **Android Auto**: Detected via APID (`CarAppService`, `AOAP`,
  `AndroidAuto`), payload keywords (`USB_ACCESSORY`, `AOA`)

**Events**:
- CarPlay: `video_focus_lost`, `audio_ducking`, `mdns_handshake`,
  `hid_event`, `auth_tls`
- Android Auto: `sensor_data`, `audio_focus`, `mdns_handshake`,
  `session_start`, `session_stop`

**Preset rules**: 8 quick filters for common use cases.

### 2.8 User Filter Manager (`userfiltermanager.h/.cpp`)

**Purpose**: User-defined regex-based log highlighting.

**Filter structure**: label, regex pattern, fields (payload, apid, ctid,
ecu), color, levels, domain.

### 2.9 AI Configuration Dialog (`dltaioptionsdialog.h/.cpp`)

**Purpose**: Configuration UI for LLM provider settings.

**Fields**: Provider selector, endpoint URL, API key, model name, max
tokens, temperature, timeout. Includes "Test Connection" button.

---

## 3. Log Ingestion Pipeline

```
QDltFile ←── DLT Viewer
    │
    ▼
initFileStart(QDltFile *file)
    │
    ▼  (for each message)
ingestMessage(index, msg)
    │
    ├── Decode via messageDecoder
    ├── Build LogEntry (index, time, level, APID, CTID, payload, ...)
    ├── AutomotiveLogParser::classify() → domain + event
    ├── indexToPos[index] = entries.size()
    ├── entries.append(entry)
    ├── Extract keywords from payload
    ├── Add to invertedIndex[keyword] → set of indices
    │
    ▼
initFileFinish()
    ├── rebuildFilterRowMap()
    ├── updateDomainStatus()
    └── startBulkAnalysis() (if enabled)
```

### Inverted Index Structure

```
QHash<QString, QSet<int>> invertedIndex
  "error"     → {0, 1, 5}
  "timeout"   → {1}
  "can"       → {5}
  "memory"    → {2}
  ...
```

This enables O(K) keyword lookups (K = number of keywords).

---

## 4. Query Routing Logic

```
onQuerySubmitted(query)
    │
    ├── Preset match? → filterByPreset + rule-based analysis
    ├── Special command? (summary, timeline, help, pattern, keywords, categorizza)
    │   └── m_ruleBasedAnalyzer->analyzeQuery()
    ├── Level/category query? → m_ruleBasedAnalyzer->analyzeQuery()
    └── Free text → liveSearch() (inverted index + regex fallback)
```

```
onAiQuerySubmitted(query)
    │
    ├── AI available? → prefilter → async LLM → parse response → display
    └── AI unavailable? → fallback to rule-based + "[fallback]" label
```

---

## 5. Performance Considerations

| Technique | Location | Benefit |
|-----------|----------|---------|
| Inverted index | `dltchatplugin.cpp` | O(K) keyword search |
| Mutex locks | `entriesMutex` | Thread-safe entry access |
| Display cap (1000) | `kMaxDisplayResults` | UI responsiveness |
| AI pre-filter (100) | `kAIPreFilterMax` | LLM prompt size limit |
| Async LLM | `analyzeQueryAsync` | Non-blocking UI |
| LRU cache | Both plugin + LLM | Repeated query speedup |
| Circuit breaker | `dltllmanalyzerinterface` | Prevent cascading failures |
| Rate limiter | `dltllmanalyzerinterface` | API quota compliance |

---

## 6. Dependencies

| Dependency | Required | Version |
|------------|----------|---------|
| Qt Core | Yes | 5.15+ / 6.x |
| Qt GUI | Yes | 5.15+ / 6.x |
| Qt Widgets | Yes | 5.15+ / 6.x |
| Qt Network | Yes | 5.15+ / 6.x |
| DLT Viewer SDK (qdlt) | Yes | 2.30.0+ |
| C++ Compiler | Yes | C++17 |
| CMake | Yes | 3.16+ |

---

## 7. Build System

```
CMakeLists.txt
  ├── Auto-detect Qt5/Qt6
  ├── Find qdlt via Findqdlt.cmake
  ├── AUTOMOC enabled for Qt meta-object compilation
  ├── Build target: MODULE library (dltchatplugin.dll/.so)
  └── PLUGIN_INTERFACE_VERSION="1.0.1"
```

The plugin is built as a Qt MODULE library (shared library loadable at
runtime).

---

## 8. Configuration

**INI file** (`dlt_chat_plugin.ini`):
```ini
[Analyzer]
type = rule-based
llmEndpoint = http://localhost:11434/api/generate
llmApiKey =
llmModel = llama3.2:1b
bulkAnalysisEnabled = false

[Behavior]
highlightColor = #FFE680
```

**User Filters** (`*.json`):
```json
{
  "version": "1.0.0",
  "filters": [
    {
      "label": "CAN Errors",
      "pattern": "CAN.*error|bus-off",
      "fields": ["payload"],
      "color": "#FF0000",
      "level": ["error", "fatal"],
      "enabled": true
    }
  ]
}
```
