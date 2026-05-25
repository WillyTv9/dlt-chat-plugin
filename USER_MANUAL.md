# Chat Log Assistant — User Manual

## 1. Introduction

Chat Log Assistant is a plugin for COVESA DLT Viewer that provides
interactive chat-based log analysis. You can ask questions about your
DLT log files in natural language and receive contextual explanations
with references to relevant log entries.

---

## 2. Installation

See [INSTALL.md](INSTALL.md) for detailed installation instructions including runtime dependencies
(Qt DLLs, OpenSSL, MSVC redistributable) and cross-platform build steps.

Quick summary:
1. Copy the plugin binary (`dltchatplugin.dll` / `libdltchatplugin.so` / `libdltchatplugin.dylib`)
   to the DLT Viewer plugins directory
2. On Windows, run `windeployqt` on the DLL to gather Qt and MSVC runtime dependencies; copy
   OpenSSL DLLs from the Qt `bin/` directory if using AI features
3. Launch DLT Viewer
4. Enable the plugin: **Settings → Plugin Settings → Chat Log Assistant**
5. Enable the panel: **View → Panels → Chat Log Assistant**

---

## 3. User Interface Overview

```
┌──────────────────────────────────────────────────────────┐
│ Chat Log Assistant          AI: llama3.2:1b [online] [⚙]│  ← Title bar
│ Loaded 15000 msgs | CarPlay: 230 | AA: 150               │  ← Status bar
├──────────────────────────────────────────────────────────┤
│ Quick Actions (filtri .dlp)                              │
│ [Bluetooth ▼] [Smartphone Projection ▼] [Audio/Media ▼] │  ← Macro-category menus
│ [Navigazione/GPS ▼] [Sistema/Diagnostica ▼] [HMI/UI ▼]  │     with per-filter colours
│ [Sicurezza ▼] [Radio ▼] [Altri ▼]                       │
│ ─────────────────────────────────────────────────────── │
│ Quick Actions (built-in)                                 │
│ [Errors] [Warns] [Info] [Debug] [Verbose]              │  ← Level shortcuts
├──────────────────────────────────────────────────────────┤
│ Chat History                                            │
│  Tu: show errors                                        │
│  Chat Assistant: Found 23 error messages...              │
│                                                          │
│  Tu (AI): What caused the timeout?                       │
│  AI Assistant: The timeout occurred at...                │
├──────────────────────────────────────────────────────────┤
│ Results List                                            │
│ Index | Snippet                                    ▲    │
│ 1452  | Error: timeout ECU_XYZ                     ▐    │
│ 1453  | Warning: retry attempt 1/3                 ▐    │
│ 1460  | Fatal: watchdog triggered                   ▼    │
├──────────────────────────────────────────────────────────┤
│ [ Ask about logs...                  ] [Send]           │  ← Rule-based Input
├──────────────────────────────────────────────────────────┤
│ [ Ask the AI...                      ] [Ask AI] ████   │  ← AI Input + Progress
│ AI performance depends on hardware                       │  ← Disclaimer
├──────────────────────────────────────────────────────────┤
│ [Clear]    [Filters]    [CSV]    [CSV All]               │  ← Action Buttons
└──────────────────────────────────────────────────────────┘
```

---

## 4. Using Quick Action Buttons (filtri nativi .dlp)

Le Quick Actions non usano più query testuali libere: ogni azione corrisponde a un
**filtro nativo** definito in un file di progetto DLT-Viewer (`.dlp`). Il plugin
carica il file, mappa ogni filtro positivo (`<pfilter>` con `type=0`) su un'azione,
sottrae l'eventuale filtro negativo abbinato (`type=1`) e applica esattamente i
criteri di matching di DLT-Viewer (App ID, Context ID, header/payload, regex e
case-sensitivity così come configurati nel `.dlp`).

### Dove trovare i filtri
I filtri sono raggruppati in poche **macro-categorie**, ciascuna esposta come un
pulsante con menu a tendina nel riquadro *Quick Actions (filtri .dlp)*:

| Menu | Contenuto tipico |
|------|------------------|
| **Bluetooth** | BluetoothManager, BluettothStack, BT Database, BtDataLink+, BtAv+, BtHf+, BtPbap+ |
| **Smartphone Projection** | AndroidAuto, AaReceiverLib, CarPlay, CarPlayStack, Carbit, Alexa, Ferrite, … |
| **Audio / Media** | AudioManager, MuliMediaPlayer, VideoManager, TtsManager |
| **Navigazione / GPS** | GPS+, NaviManager |
| **Sistema / Diagnostica** | SYS+, SystemManagerLI/LC, MessagManager, ConnectionManager, DiagnosticManager, … |
| **HMI / UI** | HmiApp, HMIM+, LicenceManager |
| **Sicurezza** | SecurityManager |
| **Radio** | RadioApp |
| **Altri** | qualsiasi filtro del `.dlp` non assegnato a un gruppo |

Ogni voce di menu mostra un quadratino del **colore del filtro** (il campo
`<filterColour>` del `.dlp`): è lo stesso colore con cui le righe corrispondenti
vengono evidenziate nella tabella principale.

### Come si usa
1. Apri un log DLT.
2. Clicca un pulsante macro-categoria e scegli il filtro dal menu.
3. Il plugin esegue il matching nativo su tutti i messaggi, mostra il numero di
   righe colpite nella chat e le **evidenzia con il colore del filtro** nella
   tabella principale di DLT-Viewer.
4. Usa **Clear** per rimuovere l'evidenziazione.

### Da dove provengono i filtri
- Per default il plugin usa il file **incorporato** `MY_ARTIST8.dlp`.
- Per usare un `.dlp/.dlf` personale, imposta `dlpPath` nella sezione `[Filters]`
  di `dlt_chat_plugin.ini` (vedi §13). Il file esterno ha priorità su quello
  incorporato e può essere aggiornato senza ricompilare il plugin.

> I livelli di log (error/warn/info…) e i comandi speciali (`summary`, `timeline`,
> `help`, `categorizza`…) restano disponibili digitandoli nella casella di chat
> (vedi §5).

---

## 5. Using the Chat (Rule-based)

Type queries in the "Ask about logs..." input box and click **Send**.

### By log level
- `error` — show all error/fatal messages
- `warn` — show all warnings
- `info` — show all informational messages
- `debug` — show all debug messages
- `verbose` — show all verbose messages

### By domain
- `carplay` — show CarPlay messages
- `androidauto` — show Android Auto messages

### By category
- `can` — CAN bus related messages
- `security` — auth/security issues
- `sensors` — sensor-related data (accel, gyro, temp)
- `wifi` — WiFi/Hotspot issues
- `power` — power management (sleep, wake, battery)
- `diagnostic` — diagnostic trouble codes (DTC)
- `gps` — GPS/navigation messages

### Combined queries
- `error can` — CAN bus errors
- `warn carplay` — CarPlay warnings
- `error security` — security errors
- `info carplay timeout` — informational CarPlay messages mentioning timeout

### Special commands
| Command | Aliases | Description |
|---------|---------|-------------|
| `summary` | `riassumi` | Log statistics |
| `timeline` | `cronologia` | Chronological list |
| `pattern` | — | Duplicate message detection |
| `categorizza` | `categorize`, `classifica` | Classify errors by category |
| `keywords` | `categorie` | Available categories |
| `help` | `aiuto` | Available commands |

### Free text search
Any other text performs a keyword search across all log entries using
the inverted index (O(K) lookup). If no keywords match, falls back to
regex search.

---

## 6. Using the AI (LLM)

### 6.1 Configuration

1. Click the gear icon (⚙) next to the AI status
2. Select a provider:
   - **Ollama** (recommended for local use, default endpoint: `localhost:11434/api/generate`)
   - **OpenAI** (endpoint: `https://api.openai.com/v1/chat/completions`)
   - **LocalAI** (endpoint: `http://localhost:8080`)
   - **Custom** (user-defined endpoint)
3. Fill in the fields:
   - **Endpoint**: API URL (auto-filled when changing provider)
   - **API Key**: required for OpenAI only
   - **Model**: e.g., `llama3.2:1b` (default), `llama3`, `gpt-4o-mini`
   - **Max Tokens**: maximum response length (default: 4096)
   - **Temperature**: creativity level (default: 0.7)
   - **Timeout**: request timeout in ms (default: 120000)
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
| `AI: llama3.2:1b` (green dot) | AI ready and online |
| `AI: offline` (orange dot) | AI configured but unreachable |
| `AI: -` (gray dot) | AI not configured |

The AI availability is checked every 30 seconds.

### 6.4 AI Query Flow

When you submit an AI query:
1. AI availability check — if offline, fallback to rule-based immediately
2. Cache lookup — if the same query was answered recently, return cached result
3. Rate limit check — token bucket (10 tokens, 1/s refill); waits if empty
4. HTTP POST to LLM provider with structured prompt containing:
   - Log file description + total entry count
   - Up to 100 filtered log entries with indices
   - User filter context (if any active filters)
   - The original question
5. Response parsing — extracts text, `[index:N]` references, and index numbers
6. Cache update — stores response for future identical queries

### 6.5 AI Safety & Limits

- **Circuit breaker**: 5 consecutive failures → 60 seconds cooldown
- **Rate limiter**: token bucket (10 tokens, 1 token/sec refill)
- **Response cache**: 1000 entries in LLM analyzer, 10000 in plugin
- **Retry policy**: exponential backoff with jitter
- **Fallback**: if AI is unavailable, falls back to rule-based analysis
  with a `[fallback]` label in the response

---

## 7. Bulk Analysis

Classify an entire log file using AI in background.

### 7.1 Starting Bulk Analysis

Enable `bulkAnalysisEnabled=true` in `dlt_chat_plugin.ini`. On the next
file load, entries are divided into chunks (default 100 per chunk) and
each chunk is sent to the LLM for classification.

### 7.2 Bulk Classification Format

Each entry receives: `[INDEX]|CATEGORY|SUMMARY|TAGS`

```
[0]|CONNECT|Connection established|connection,handshake
[1]|AUDIO|Audio focus gained|audio,focus,carplay
[2]|ERROR|Timeout on iAP2 handshake|timeout,carplay,error
```

### 7.3 Searching Bulk Results

After bulk analysis, search using:
- `tag:handshake` — find entries tagged "handshake"
- `category:ERROR` — find entries classified as ERROR

### 7.4 States

Idle → Processing → Paused (optional) → Completed / Error

Progress is shown as a progress bar with chunk count.

---

## 8. Automotive Log Classification

The plugin automatically classifies log entries into automotive domains
and events.

### 8.1 Domains

**Apple CarPlay** — detected by:
- **Payload keywords**: `carplay`, `iap2`, `airplay`, `siri`, `appledevice`, `eaProtocol`, `CP*`
- **Technical identifiers**: ExternalAccessory framework, Apple USB identifiers, `com.apple.carplay`

**Android Auto** — detected by:
- **Payload keywords**: `androidauto`, `aa`, `projection`, `headunit`, `aaprotocol`, `gms`, `AOA`
- **Technical identifiers**: Google Projection, Android Open Accessory (AOA), Google Play Services, `CarAppService`

### 8.2 Recognized Events

| Event | Domain | Detection Keywords |
|-------|--------|-------------------|
| `video_focus_lost` | CarPlay | video, focus |
| `audio_ducking` | CarPlay | ducking, audio duck |
| `mdns_handshake` | Both | `_apple-mobdev2`, mdns, dns-sd |
| `hid_event` | CarPlay | hid, keyboard, touch |
| `auth_tls` | CarPlay | tls, auth, certificate, pairing |
| `sensor_data` | Android Auto | sensor, gyro, gps, accelerometer |
| `audio_focus` | Android Auto | audio focus, audio manager |
| `session_start` | Both | session start, link established |
| `session_stop` | Both | session stop, disconnect, link down |

### 8.3 Preset Filters

All categories and smart filters are available via the **Quick Action** buttons. Common combinations include:
- `gps_errors` — Navigation specific errors
- `network_timeouts` — Connectivity issues
- `wireless_projection` — WiFi/Wireless link problems
- `carplay_errors` / `androidauto_errors` — Domain-specific failures

---

## 9. Error Categorization

The `categorizza` command classifies error/fatal entries into 7 categories:

| Category | Italian | Keywords |
|----------|---------|----------|
| Communication | Comunicazione | timeout, connection refused, link down, handshake failed |
| Memory | Memoria | out of memory, allocation failed, heap, stack overflow |
| Security | Sicurezza | auth failed, unauthorized, invalid certificate, TLS/SSL |
| Configuration | Configurazione | invalid config, missing parameter, wrong version |
| Hardware | Hardware | hardware fault, sensor failure, I2C, SPI, GPIO |
| Timeout | Timeout | timeout error, timeout occurred |
| Protocol | Protocollo | protocol error, malformed, unexpected response |

---

## 10. Working with Results

### Viewing Results

Search results appear in two places:
1. **Chat History** — shows the query and response (HTML formatted)
2. **Results List** — shows indexed results with color-coded snippets

### Navigating to Log Entries

Double-click any result in the Results List to navigate to that entry
in the DLT Viewer's main table view.

### Clearing Results

Click **Clear** to remove all highlights and clear the results list.

### Performance Limits

| Limit | Value | Notes |
|-------|-------|-------|
| Max entries loaded | 500 000 | Hard limit, prevents memory exhaustion |
| Results displayed | 1 000 | UI responsiveness cap |
| AI pre-filter | 100 | Max entries sent to LLM per query |
| Plugin response cache | 10 000 | LRU-evicted |
| LLM analyzer cache | 1 000 | Half-flush when full |

---

## 11. Export

### Export Filtered Results (CSV)

Click **CSV** to export the current results list to a CSV file.
Columns: `#, Index, Time, Timestamp, Level, ECU, APID, CTID, Domain,
Payload, Source Query`.

### Export All Logs (CSV)

Click **CSV All** to export all loaded log entries to a CSV file.
Columns: `Index, Time, Timestamp, Level, ECU, APID, CTID, Domain,
Payload`.

CSV output is UTF-8 encoded with proper escaping for quotes, commas,
and newlines.

---

## 12. User Filters

User-defined filters allow regex-based highlighting of log entries.

### Loading Filters

1. Create a JSON filter file (see `automotive_filters_example.json`)
2. Click **Filters** and select the file
3. Matching entries will be highlighted in the viewer

### Filter File Format

```json
{
  "version": "1.0",
  "filters": [
    {
      "label": "CAN Errors",
      "pattern": "CAN.*error|arbitration|bus-off",
      "fields": ["payload"],
      "color": "#FF0000",
      "level": ["error", "fatal"],
      "domain": "carplay",
      "enabled": true
    }
  ]
}
```

**Fields**:
- `label` (required): Display name
- `pattern` (required): Regex pattern
- `fields` (optional): Fields to search (`payload`, `apid`, `ctid`, `ecu`)
- `color` (required): Hex color for highlights
- `level` (optional): Filter by log level
- `domain` (optional): Filter by domain
- `enabled` (optional): Whether filter is active (default: true)

Filters are applied after every query. If multiple filters match an
entry, the last matching filter's color wins.

---

## 13. Configuration File

The plugin saves configuration to `dlt_chat_plugin.ini`:

```ini
[Analyzer]
type = rule-based
llmEndpoint = http://localhost:11434/api/generate
llmApiKey =
llmModel = llama3.2:1b
bulkAnalysisEnabled = false

[Behavior]
maxResults = 1000
highlightColor = #FFE680
userFiltersPath =

[Filters]
# Path to DLT Viewer project file for native Quick Actions
dlpPath =

[Live]
# AI digest refresh interval in seconds during live capture
aiRefreshSec = 30
```

See the annotated [`dlt_chat_plugin.ini.example`](dlt_chat_plugin.ini.example) for all available keys
and full documentation of each section. Settings are loaded on startup and saved on shutdown.

---

## 14. Troubleshooting

| Problem | Solution |
|---------|----------|
| Plugin not visible in menu | Verify the plugin binary is in the correct plugins directory (check DLT Viewer → Preferences) |
| Plugin not loading (Windows) | Ensure Qt version matches the DLT Viewer host. Run `windeployqt --release --compiler-runtime` on the DLL to gather runtime dependencies |
| `Qt5Core.dll not found` (Windows) | Qt DLLs missing from PATH. Copy Qt DLLs to the viewer directory or run `windeployqt` |
| `libssl-1_1-x64.dll not found` (Windows, Qt5) | OpenSSL missing from Qt Network path. Copy `libssl-1_1-x64.dll` and `libcrypto-1_1-x64.dll` from Qt `bin/` directory |
| `libssl-3-x64.dll not found` (Windows, Qt6) | Same as above, but with OpenSSL 3.x DLLs |
| `msvcp140.dll not found` (Windows) | Install Visual C++ Redistributable or pass `--compiler-runtime` to `windeployqt` |
| No logs loaded | Open a DLT file first |
| Results list empty | Try different keywords or check log levels |
| AI not connecting | Verify endpoint URL and model name in configuration |
| AI connection timeout | Increase timeout value or check network/server |
| CSV export fails | Ensure destination path is writable |
| Dark mode issues | Plugin supports system theme automatically |
| All results show 1000 max | This is the display cap for UI performance |

---

## 15. Tips & Best Practices

- Start with **Summary** to get an overview of log contents
- Use level filters (`error`, `warn`) for quick issue detection
- For AI, be specific: *"Why did ECU_XYZ disconnect at 10:05:30?"*
  instead of *"What's wrong?"*
- Combine domain + level: `error carplay` for CarPlay-specific errors
- Use **Pattern** to identify recurring issues
- Export filtered results for external analysis
- Double-click any result to jump to the original log entry
- Install Ollama locally for privacy: models run on your machine
