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
│  │  │ Chat UI  │  │  Analyzer    │  │  LogStore +   │  │  │
│  │  │ (Form)   │◄─┤  (Strategy)  │◄─┤  LogIndex     │  │  │
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
- **Token Bucket**: Rate limiter for LLM API calls (10 tokens, 1/s refill)
- **LRU Cache**: Response caching in both plugin (10k) and LLM analyzer (1k)
- **Worker Thread**: `DltBulkAnalyzerWorker` runs bulk analysis in QThread
- **Factory**: `DltLlmAnalyzerFactory` creates provider-specific analyzers

---

## 2. Component Breakdown

### 2.1 Core Plugin (`plugin_entry.h/.cpp`)

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

### 2.1bis Native filter Quick Actions (`native_filter_catalog.*`, `highlight_delegate.*`)

**Purpose**: Drive the Quick Actions from the native DLT-Viewer filters of a `.dlp`
project file instead of hardcoded text queries.

- **`NativeFilterCatalog`** parses every `<pfilter>` of the `.dlp` via the SDK's own
  `QDltFilter::LoadFilterItem`, so matching is byte-for-byte faithful to DLT-Viewer
  (App ID / Context ID / header / payload / regex / case-sensitivity). Positive
  filters become Quick Actions; paired negative (`type=1`) filters are subtracted.
  The macro-category grouping and the positive→negative pairing come from the
  embedded `native_filter_groups.json`. Source selection: explicit `.dlp` from
  `[Filters] dlpPath` in the ini, else the embedded `:/dltchat/default_filters.dlp`.
- **`HighlightDelegate`** is a `QStyledItemDelegate` installed on the host main
  table view (`initMainTableView`). It paints each matched row with the filter's own
  `<filterColour>`, giving distinct multicolour highlighting even on ARTIST8 2.28
  (which lacks `QDltFile::setManualMarkerIndices()`). It chains the host's original
  delegate for untouched rows.

**Flow**: `Form::nativeFilterTriggered(name)` → `onNativeFilterTriggered()` →
`NativeFilterCatalog::match()` over `QDltFile` → `form->setResults()` +
`highlightIndicesColored()`.

### 2.2 Chat UI (`chatform.h/.cpp`)

**Purpose**: User-facing chat interface.

**Components**:
- Status bar (file info, domain stats, AI status)
- 76 quick action buttons (13-row scrollable grid with per-button colors,
  loaded from category_registry.json)
- Chat history (`QTextBrowser`)
- Results list (`QListView` + `ResultsModel`) with clickable items
- Rule-based query input + Send button
- AI query input + Ask AI button + progress bar
- Bottom bar: Clear, Filters (load), CSV, CSV All

**Palette-aware**: Automatically adjusts colors for dark/light themes.

### 2.3 Results Model (`results_model.h/.cpp`)

**Purpose**: Qt Model/View backing for the results list.

Provides color-coded items with level-appropriate highlighting
(error → red, warn → orange, others → gray).

### 2.4 AI Options Dialog (`dltaioptionsdialog.h/.cpp`)

**Purpose**: Configuration UI for LLM provider settings.

Fields: Provider selector (Ollama/OpenAI/LocalAI/Custom), endpoint URL,
API key, model name, max tokens, temperature, timeout.
Includes "Test Connection" button.

### 2.5 Analyzer Interface (`analyzer_interface.h/.cpp`)

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
- Error categorization: classifies error/fatal entries into 7 categories
  (Comunicazione, Memoria, Sicurezza, Configurazione, Hardware, Timeout,
  Protocollo)
- Statistics: level counts, top contexts, repeated messages, domains

### 2.6 LLM Analyzer (`llm_analyzer_interface.h/.cpp`)

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

### 2.7 Bulk Analyzer (`bulk_analyzer.h/.cpp`)

**Purpose**: Batch log classification using LLM.

**Architecture**: Worker thread processing logs in chunks (default 100/chunk).

**Features**:
- Chunked processing with pause/resume/cancel
- Chunk classification query: category, summary, tags per entry
- Search results by `tag:` or `category:` filters
- Progress reporting via signals

### 2.8 Export Engine (`export_engine.h/.cpp`)

**Purpose**: CSV export of query results and all entries.

**Features**:
- Proper CSV escaping (quotes, commas, newlines)
- UTF-8 encoding (Qt5: QTextStream codec, Qt6: QStringConverter)
- Filtered export: headers + index, time, timestamp, level, ECU, APID,
  CTID, domain, payload, source query
- Full export: all entries with same metadata

### 2.9 Automotive Log Parser (`automotive_log_parser.h/.cpp`)

**Purpose**: Domain-specific classification for automotive protocols.

**Domains**:
- **CarPlay**: Detected via APID (`com.apple.carplay`), payload keywords
  (`iap2`, `AirPlay`, `CARSIM`)
- **Android Auto**: Detected via APID (`CarAppService`, `AOAP`,
  `AndroidAuto`), payload keywords (`USB_ACCESSORY`, `AOA`)

**Events** (9 total):
- CarPlay: `video_focus_lost`, `audio_ducking`, `mdns_handshake`,
  `hid_event`, `auth_tls`
- Android Auto: `sensor_data`, `audio_focus`, `mdns_handshake`,
  `session_start`, `session_stop`

**Preset rules**: 8 quick filters for common use cases
(carplay, androidauto, video_focus, audio_ducking, mdns, sensor_data,
auth_errors, session).

### 2.10 Log Store (`log_store.h/.cpp`)

**Purpose**: Thread-safe storage for decoded log entries.

Manages the `entries` vector with mutex-guarded access (`entriesMutex`).
Provides snapshot mechanism for safe concurrent reads during analysis.

### 2.11 Log Index (`log_index.h/.cpp`)

**Purpose**: Inverted index for O(K) keyword search.

Builds a `QHash<QString, QSet<int>>` mapping each keyword to the set
of entry indices where it appears. Supports AND-intersection queries.

### 2.12 AI Cache Manager (`ai_cache_manager.h/.cpp`)

**Purpose**: LRU response cache for LLM queries.

Maintains 1000-entry LRU cache with half-flush when full.

### 2.13 Contextual Extractor (`contextual_extractor.h/.cpp`)

**Purpose**: Extracts context windows around matched log entries.

Enables "show me what happened before/after index N" queries.

### 2.14 Conversation Manager (`conversation_manager.h/.cpp`)

**Purpose**: Multi-turn AI conversation history.

Maintains recent conversation context for follow-up AI queries.

### 2.15 Temporal Correlator (`temporal_correlator.h/.cpp`)

**Purpose**: Time-window based event correlation.

Correlates events within configurable time windows.

### 2.16 Fibex Enricher (`fibex_enricher.h/.cpp`)

**Purpose**: Enriches log entries with FIBEX XML metadata.

Parses FIBEX (Field Bus Exchange) XML files to provide additional
context for CAN/FlexRay bus signals.

### 2.17 User Filter Manager (`user_filter_manager.h/.cpp`)

**Purpose**: User-defined regex-based log highlighting.

**Filter structure**: label, regex pattern, fields (payload, apid, ctid,
ecu), color, levels, domain. Loaded from JSON file.

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
    ├── Quick button / Preset match? → AutomotiveLogParser::filterByPreset()
    │   └── m_ruleBasedAnalyzer->analyzeQuery()
    ├── Special command? (summary, timeline, help, pattern, keywords, categorizza)
    │   └── m_ruleBasedAnalyzer->analyzeQuery()
    ├── Level/category/domain query? → m_ruleBasedAnalyzer->analyzeQuery()
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
| Inverted index | `log_index.cpp` | O(K) keyword search |
| Mutex locks | `entriesMutex` | Thread-safe entry access |
| Display cap (1000) | `kMaxDisplayResults` | UI responsiveness |
| Entry limit (500k) | `kMaxEntries` | Memory bound |
| AI pre-filter (100) | `kAIPreFilterMax` | LLM prompt size limit |
| Async LLM | `analyzeQueryAsync` | Non-blocking UI |
| LRU cache | Plugin + LLM analyzer | Repeated query speedup |
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
| Qt Xml | Yes | 5.15+ / 6.x |
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
runtime). The app logic is factored into a static library (`dltchat_app_logic`)
for testability.

### Directory layout

```
src/
  app_logic/
    include/dltchat/    ← 13 public headers (all core classes)
    src/                ← implementations
  host_interface/       ← plugin_entry, chatform, options dialog, results model
  resources/presets/    ← preset filter JSON definitions
tests/
  test_*.cpp/.h         ← 10 Qt Test classes
  test_data/            ← sample FIBEX XML, filter JSON
cmake/                  ← compiler_warnings, GetGitHash, version template
dist/                   ← distribution packaging
```

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
maxResults = 1000
llmTimeout = 120000
highlightColor = #FFE680
userFiltersPath =
```

**User Filters** (`*.json`):
```json
{
  "version": "1.0",
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

---

## 9. Test Architecture

Tests use the **Qt Test** framework with 10 test suites:

| Suite | Tests | Area |
|-------|-------|------|
| `test_rulebasedanalyzer` | 17 | Commands, filters, simplifyPayload |
| `test_automotivelogparser` | 14 | CarPlay/AA classification, events, presets |
| `test_llmutils` | 9 | Response parsing, prompt building, request body |
| `test_dltexport` | 7 | CSV escaping, sanitization, full export |
| `test_userfiltermanager` | 8 | Load, match domain/level, enabled count |
| `test_contextualextractor` | — | Context window extraction |
| `test_conversationmanager` | — | Multi-turn conversation history |
| `test_fibexenricher` | — | FIBEX XML metadata enrichment |
| `test_temporalcorrelator` | — | Time-window correlation |
| `test_main` | — | Test harness / main entry |


Run tests:
```bash
cmake -B build_tests -S tests
cmake --build build_tests --config Release
cd build_tests && ctest --output-on-failure
```
