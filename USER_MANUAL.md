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
┌──────────────────────────────────────────────────────────┐
│ Chat Log Assistant          AI: llama3.2:1b [online] [⚙]│  ← Title bar
│ Loaded 15000 msgs | CarPlay: 230 | AA: 150               │  ← Status bar
├──────────────────────────────────────────────────────────┤
│ Quick Actions ┌─────┬─────┬─────┬─────┬─────┐           │
│     Row 0:    │Errors│Warns│Info │Debug│Verb│           │  ← Level filters
│     Row 1:    │CAN│Security│Memory│Perf│Diag│GPS│Cat.│  │  ← Category filters
│     Row 2:    │CP│AA│Focus│Duck│mDNS│Sensor│Auth│       │  ← Automotive presets
│     Row 3:    │Sess│Patt│Summ│Time│Catgz│Help│Keyw│    │  ← Analysis commands
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

## 4. Using Quick Action Buttons

The plugin features 76 quick action buttons arranged in a 13-row scrollable grid (rows 0–12):

### Row 0: Severity Levels
| Button | Query | Description |
|--------|-------|-------------|
| **Fatal** | `fatal` | Find fatal/critical errors |
| **Error** | `error` | Find error messages |
| **Warn** | `warn` | Find warnings |
| **Info** | `info` | Find informational messages |
| **Debug** | `debug` | Find debug messages |
| **Verb** | `verbose` | Find verbose messages |

### Row 1: Core & Boot
| Button | Query | Description |
|--------|-------|-------------|
| **System** | `system` | System core and frameworks |
| **Diag** | `diag` | Diagnostics and health monitoring |
| **GPS** | `gps` | Navigation and positioning |
| **Startup** | `boot` | Boot process and init |
| **Power** | `power` | Power management and sleep/suspend |
| **Session** | `login` | Session and user management |
| **Security** | `security` | Auth and access control |

### Row 2: Connectivity
| Button | Query | Description |
|--------|-------|-------------|
| **Network** | `network` | Generic network connectivity |
| **WiFi** | `wifi` | Wireless networking and hotspots |
| **Ethernet**| `ethernet`| Wired LAN and PHY management |
| **BT** | `bt` | Bluetooth and BLE |
| **USB Stack**| `usbstack`| USB host/device stack |
| **USB Media**| `usb` | External mass storage |

### Row 3: Hardware & Vehicle
| Button | Query | Description |
|--------|-------|-------------|
| **Hardware**| `hw` | Low-level drivers and firmware |
| **Vehicle** | `vehicle` | IVI and generic vehicle data |
| **CAN** | `can` | CAN bus and vehicle bus data |
| **Sensors** | `sensors` | Accelerometer, gyro, and temp sensors |
| **Storage** | `storage` | Filesystems and databases |

### Row 4: Media & HMI
| Button | Query | Description |
|--------|-------|-------------|
| **FM** | `radio` | FM Tuner and radio management |
| **DAB** | `dab` | Digital radio (DAB) |
| **Audio** | `audio` | Media playback and streams |
| **Video** | `video` | Video rendering and streaming |
| **Routing** | `mixer` | Audio routing and ALSA mixer |
| **Meta** | `metadata` | Gracenote and media metadata |
| **UI** | `ui` | HMI, QML, and display |
| **Voice** | `voice` | Voice assistant and speech (Siri/Google) |

### Row 5: Performance & Maps
| Button | Query | Description |
|--------|-------|-------------|
| **Perf** | `perf` | CPU, memory, and profiling |
| **Stats** | `metrics` | Telemetry and monitoring |
| **Time** | `time` | Clock and time sync |
| **OTA** | `update` | Software updates (OTA) |
| **Maps** | `maps` | Map rendering and tiles |
| **Location**| `location`| Geolocation services |

### Row 6: Smartphone Projection
| Button | Query | Description |
|--------|-------|-------------|
| **CarPlay** | `carplay` | Apple CarPlay domain |
| **AndroidAuto**| `androidauto`| Android Auto domain |
| **Projection**| `projection`| Generic projection issues |
| **Focus** | `video_focus` | Video focus lost events |
| **Ducking** | `audio_ducking` | Audio ducking events |
| **mDNS** | `mdns` | mDNS/DNS-SD discovery |
| **Sensor** | `sensor_data` | Vehicle sensor data (AA) |
| **Session Proj**| `session_proj`| Projection session events |

### Row 7: Smart Filters
| Button | Query | Description |
|--------|-------|-------------|
| **Critical** | `critical_only`| Fatal level logs only |
| **Err Only** | `errors_only` | Error and Fatal levels |
| **Warn Only** | `warnings_only`| Warning level only |
| **GPS Err** | `gps_errors` | Errors in Navigation |
| **Radio Warn**| `radio_warnings`| Warnings in FM/DAB radio |
| **Net TO** | `network_timeouts`| Network timeouts |
| **Veh Fault** | `vehicle_faults` | Vehicle interface errors |
| **DAB Issue** | `dab_audio_issues`| DAB audio drops |

### Row 8: Advanced Projection
| Button | Query | Description |
|--------|-------|-------------|
| **CP Err** | `carplay_errors`| CarPlay specific errors |
| **AA Err** | `androidauto_errors`| Android Auto specific errors |
| **Wireless** | `wireless_projection`| Wireless link issues |
| **Audio Proj**| `audio_routing` | Audio routing issues |
| **USB CP** | `usb_cp` | USB CP negotiation |
| **USB AA** | `usb_aa` | USB AA negotiation |

### Row 9: Analysis & Help
| Button | Query | Description |
|--------|-------|-------------|
| **Summary** | `summary` | Show log statistics |
| **Timeline** | `timeline` | Chronological list |
| **Pattern** | `pattern` | Duplicate detection |
| **Categorizza**| `categorizza` | AI classification |
| **Help** | `help` | Commands help |
| **Categories** | `categories` | List all filter categories |

### Row 10: Managers
| Button | Query | Description |
|--------|-------|-------------|
| **SysMgr** | `sysmgr` | System manager and watchdog |
| **Launcher** | `launcher` | App launcher and Rapp manager |
| **MsgBus** | `msgbus` | Message bus / event bus |
| **ConMgr** | `conmgr` | Connectivity manager |

### Row 11: Subsystems
| Button | Query | Description |
|--------|-------|-------------|
| **TTS** | `tts` | Text-to-speech manager |
| **Haptic** | `haptic` | Haptic controller |

### Row 12: Special Filters
| Button | Query | Description |
|--------|-------|-------------|
| **GPS Epoch** | `gps_epoch` | GPS entries mentioning 1970/epoch |
| **Sys Fatal** | `sys_fatal` | Fatal events in System core |
| **All Fatal** | `all_fatal` | All fatal-level entries |
| **Auth Err** | `auth_errors` | Authentication and security errors |

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
llmTimeout = 120000
highlightColor = #FFE680
userFiltersPath =
```

Settings are loaded on startup and saved on shutdown.

---

## 14. Troubleshooting

| Problem | Solution |
|---------|----------|
| Plugin not visible in menu | Ensure DLL is in the correct plugins directory |
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
