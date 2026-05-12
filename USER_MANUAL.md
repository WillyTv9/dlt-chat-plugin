# Chat Log Assistant — User Manual

## 1. Introduction

Chat Log Assistant is a plugin for COVESA DLT Viewer that provides
interactive chat-based log analysis. You can ask questions about your
DLT log files in natural language and receive contextual explanations
with references to relevant log entries.

---

## 2. Installation

See [INSTALL.md](INSTALL.md) for detailed installation instructions.

Quick summary:
1. Copy `dltchatplugin.dll` (Windows) or `libdltchatplugin.so` (Linux)
   to the DLT Viewer plugins directory
2. Launch DLT Viewer
3. Enable the plugin: **Settings → Plugin Settings → Chat Log Assistant**
4. Enable the panel: **View → Panels → Chat Log Assistant**

---

## 3. User Interface Overview

```
┌──────────────────────────────────────────────┐
│ Chat Log Assistant     AI: llama3.2:1b [⚙] │  ← Title bar
│ Loaded 15000 msgs | CarPlay: 230 | AA: 150  │  ← Status bar
├──────────────────────────────────────────────┤
│ [Errors] [Warnings] [Info] [Debug] [CAN]     │
│ [Security] [Memory] [Performance][Diag]      │  ← Quick Actions
│ [Pattern] [Summary] [Timeline] [GPS] [Categorizza]   │
│ [Help] [CarPlay] [AndroidAuto] [Focus] [Ducking]     │
│ [mDNS] [Sensor]                              │
├──────────────────────────────────────────────┤
│                                              │
│  Tu: show errors                             │  ← Chat History
│  Chat Assistant: Found 23 error messages...  │
│                                              │
│  Tu (AI): What caused the timeout?           │
│  AI Assistant: The timeout occurred at...    │
│                                              │
├──────────────────────────────────────────────┤
│ Index | Snippet                       ▲      │  ← Results List
│ 1452  | Error: timeout ECU_XYZ        ▐      │
│ 1453  | Warning: retry attempt 1/3    ▐      │
│ 1460  | Fatal: watchdog triggered      ▼      │
├──────────────────────────────────────────────┤
│ [ Ask about logs...          ] [Send]        │  ← Rule-based Input
├──────────────────────────────────────────────┤
│ [ Ask the AI...              ] [Ask AI] ████ │  ← AI Input + Progress
│ AI performance depends on hardware           │  ← Disclaimer
├──────────────────────────────────────────────┤
│ [Clear]            [Filters] [CSV] [CSV All] │  ← Action Buttons
└──────────────────────────────────────────────┘
```

---

## 4. Using Quick Action Buttons

Click any button to instantly analyze logs:

| Button | Description |
|--------|-------------|
| **Errors** | Find all error and fatal messages |
| **Warnings** | Find all warning messages |
| **Info** | Find informational messages |
| **Debug** | Find debug messages |
| **CAN** | Find CAN bus related messages |
| **Security** | Find auth/security issues |
| **Memory** | Find memory-related problems |
| **Performance** | Find timeout/delay issues |
| **Diagnostic** | Find diagnostic trouble codes (DTC) |
| **Pattern** | Find repeated/duplicate messages |
| **Summary** | Show log statistics (level counts, top contexts, etc.) |
| **Timeline** | Show chronological entry list |
| **GPS** | Find GPS/navigation messages |
| **Categorizza** | Classify errors by category (Comunicazione, Memoria, Sicurezza, Configurazione, Hardware, Timeout, Protocollo) |
| **Help** | Show available commands |
| **CarPlay** | Filter CarPlay domain messages |
| **AndroidAuto** | Filter Android Auto domain messages |
| **Focus** | Find video focus lost events |
| **Ducking** | Find audio ducking events |
| **mDNS** | Find mDNS handshake events |
| **Sensor** | Find vehicle sensor data |

---

## 5. Using the Chat (Rule-based)

Type queries in the "Ask about logs..." input box and click **Send**.

### Examples

**By log level:**
- `error` — show all error/fatal messages
- `warn` — show all warnings
- `info` — show all informational messages
- `debug` — show all debug messages

**By domain:**
- `carplay` — show CarPlay messages
- `androidauto` — show Android Auto messages

**By category:**
- `can` — CAN bus related messages
- `security` — auth/security issues
- `memory` — memory problems
- `performance` — timeout/delay issues
- `diagnostic` — diagnostic trouble codes
- `gps` — GPS/navigation messages

**Combined queries:**
- `error can` — CAN bus errors
- `warn carplay` — CarPlay warnings
- `error security` — security errors

**Special commands:**
- `summary` or `riassumi` — log statistics
- `timeline` or `cronologia` — chronological list
- `pattern` — duplicate message detection
- `categorizza` or `categorize` or `classifica` — classify errors by category
- `keywords` or `categorie` — available categories
- `help` or `aiuto` — available commands

**Free text search:**
Any other text performs a keyword search across all log entries.

---

## 6. Using the AI (LLM)

### 6.1 Configuration

1. Click the gear icon (⚙) next to the AI status
2. Select a provider:
   - **Ollama** (recommended for local use)
   - **OpenAI**
   - **LocalAI**
   - **Custom**
3. Fill in the fields:
   - **Endpoint**: API URL (auto-filled for standard providers)
   - **API Key**: required for OpenAI only
   - **Model**: e.g., `llama3.2:1b`, `llama3`, `gpt-4`
   - **Max Tokens**: maximum response length (default: 1000)
   - **Temperature**: creativity level (default: 0.3)
   - **Timeout**: request timeout in ms (default: 30000)
4. Click **Test Connection** to verify
5. Click **OK** to save

When the AI is ready, the status shows green with the model name.

### 6.2 Asking Questions

Type natural language questions in the "Ask the AI..." input box:

- *"What caused the timeout?"*
- *"Show me the sequence of errors before the crash"*
- *"Why did the service restart?"*
- *"Summarize all critical errors"*
- *"Show all CAN communication failures"*
- *"What happened between index 100 and 200?"*

The AI will:
1. Analyze the log context (up to 100 relevant entries)
2. Provide a contextual explanation
3. Reference specific log entries as `[index:N]`
4. Optionally suggest root causes

### 6.3 AI Status Indicators

| Status | Meaning |
|--------|---------|
| `AI: llama3.2:1b` (green) | AI ready and online |
| `AI: offline` (orange) | AI configured but unreachable |
| `AI: -` (gray) | AI not configured |

### 6.4 Fallback Behavior

If the AI is unavailable (network issue, server down, etc.), the plugin
automatically falls back to rule-based analysis and adds a `[fallback]`
label to the response.

---

## 7. Working with Results

### Viewing Results

Search results appear in two places:
1. **Chat History** — shows the query and response
2. **Results List** — shows indexed results with snippets

### Navigating to Log Entries

Double-click any result in the Results List to navigate to that entry
in the DLT Viewer's main table view.

### Clearing Results

Click **Clear** to remove all highlights and clear the results list.

---

## 8. Export

### Export Filtered Results (CSV)

Click **CSV** to export the current results list to a CSV file.
Columns: `#, Index, Time, Timestamp, Level, ECU, APID, CTID, Domain,
Payload, Source Query`.

### Export All Logs (CSV)

Click **CSV All** to export all loaded log entries to a CSV file.
Columns: `Index, Time, Timestamp, Level, ECU, APID, CTID, Domain,
Payload`.

---

## 9. User Filters

User-defined filters allow regex-based highlighting of log entries.

### Loading Filters

1. Create a JSON filter file (see `automotive_filters_example.json`)
2. Click **Filters** and select the file
3. Matching entries will be highlighted in the viewer

### Filter File Format

```json
{
  "version": "1.0.0",
  "filters": [
    {
      "label": "CAN Errors",
      "pattern": "CAN.*error|arbitration|bus-off",
      "fields": ["payload"],
      "color": "#FF0000",
      "level": ["error", "fatal"],
      "enabled": true
    }
  ]
}
```

**Fields**:
- `label` (required): Display name
- `pattern` (required): Regex pattern
- `fields` (required): Fields to search (`payload`, `apid`, `ctid`, `ecu`)
- `color` (required): Hex color for highlights
- `level` (optional): Filter by log level
- `domain` (optional): Filter by domain
- `enabled` (optional): Whether filter is active (default: true)

---

## 10. Configuration File

The plugin saves configuration to an INI file. Example:

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

---

## 11. Troubleshooting

| Problem | Solution |
|---------|----------|
| Plugin not visible in menu | Ensure DLL is in the correct plugins directory |
| No logs loaded | Open a DLT file first |
| Results list empty | Try different keywords or check log levels |
| AI not connecting | Verify endpoint URL and model name in configuration |
| AI connection timeout | Increase timeout value or check network/server |
| CSV export fails | Ensure destination path is writable |
| Dark mode issues | Plugin supports system theme automatically |

---

## 12. Tips & Best Practices

- Start with **Summary** to get an overview of log contents
- Use level filters (`error`, `warn`) for quick issue detection
- For AI, be specific: *"Why did ECU_XYZ disconnect at 10:05:30?"*
  instead of *"What's wrong?"*
- Combine domain + level: `error carplay` for CarPlay-specific errors
- Use **Pattern** to identify recurring issues
- Export filtered results for external analysis
