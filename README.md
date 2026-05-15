# Chat Log Assistant Plugin / Plugin Assistente Log Chat

[![License: MPL 2.0](https://img.shields.io/badge/License-MPL_2.0-blue.svg)](LICENSE)
[![Qt Version](https://img.shields.io/badge/Qt-5.15+%2F6.x-green.svg)](https://www.qt.io/)
[![DLT Viewer](https://img.shields.io/badge/DLT_Viewer-2.30.0+-orange.svg)](https://github.com/COVESA/dlt-viewer)
[![Version](https://img.shields.io/badge/Version-0.7.0-blue.svg)](.)
[![CI](https://img.shields.io/github/actions/workflow/status/WillyTv9/dlt-chat-plugin/build.yml?branch=main&label=CI)](https://github.com/WillyTv9/dlt-chat-plugin/actions)
[![Presentation](https://img.shields.io/badge/📊-Presentation-blue?style=flat&labelColor=555)](https://WillyTv9.github.io/dlt-chat-plugin/)

A chat-based log analysis plugin for COVESA DLT Viewer with rule-based analysis and optional AI (LLM) integration.

Plugin per l'analisi dei log DLT con interfaccia chat, analisi rule-based e supporto AI opzionale.

---

## Build Guide / Guida alla compilazione

### English

#### Prerequisites

- **Qt** 5.15+ or 6.x (Core, Gui, Widgets, Network, Xml)
- **DLT Viewer SDK** (qdlt) 2.30.0+
- **CMake** 3.16+
- **C++17** compiler (MSVC 2019+, GCC 9+, Clang 10+)

#### Building within DLT Viewer (recommended)

1. **Clone DLT Viewer:**
   ```bash
   git clone https://github.com/COVESA/dlt-viewer.git
   cd dlt-viewer
   ```

2. **Copy the plugin** under the plugins directory:
   ```bash
   cp -r /path/to/dlt-chat-plugin plugin/dlt-chat-plugin
   ```

3. **Configure with CMake:**
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```
   On Windows with MinGW (MSYS2):
   ```bash
   cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
   ```
   On Windows with MSVC:
   ```cmd
   cmake -G "Visual Studio 17 2022" -A x64 ..
   ```

4. **Build:**
   ```bash
   cmake --build . --parallel
   ```

5. **Output:**
   - Linux:   `build/bin/plugins/libdltchatplugin.so`
   - Windows: `build/bin/plugins/Release/dltchatplugin.dll` (MSVC)
   - Windows: `build/bin/plugins/libdltchatplugin.dll` (MinGW)

6. **Install to DLT Viewer plugins folder:**
   - Linux:   `cp build/bin/plugins/libdltchatplugin.so ~/.local/share/dlt-viewer/plugins/`
   - Windows: copy the DLL to `%LOCALAPPDATA%\Programs\dlt-viewer\plugins\`

#### Building standalone

```bash
cd dlt-chat-plugin
mkdir build && cd build

# Set DLT Viewer SDK path
set QDLT_ROOT=C:\path\to\dlt-viewer\sdk  # Windows
export QDLT_ROOT=/path/to/dlt-viewer      # Linux

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

Output: `build/src/host_interface/dltchatplugin.dll` (MSVC) or `build/libdltchatplugin.so` (Linux).

On Windows, you can also run `build_plugin.bat` from a Visual Studio Developer Prompt.

#### Build options

| Option | Description | Default |
|--------|-------------|---------|
| `-DDLTCHAT_BUILD_TESTS=ON` | Enable unit tests | OFF |
| `-DDLTCHAT_BUILD_DIST=ON` | Enable distribution package | OFF |
| `-DDLT_ENABLE_ASAN=ON` | AddressSanitizer (Debug, non-MSVC) | OFF |
| `-DQT_PREFIX=...` | Override Qt version (`Qt5`/`Qt6`) | Auto |

#### Running tests

```bash
# Build tests standalone
cmake -B build_tests -S tests -DQT_PREFIX=Qt6
cmake --build build_tests --config Release

# Run all tests
cd build_tests && ctest --output-on-failure

# Run a single test
cd build_tests && ctest -R rulebased --output-on-failure
```

#### Distribution package

```bash
cmake --build build --target dist
# Output in build/dist/dltchatplugin/
```

---

### Italiano

#### Prerequisiti

- **Qt** 5.15+ o 6.x (Core, Gui, Widgets, Network, Xml)
- **DLT Viewer SDK** (qdlt) 2.30.0+
- **CMake** 3.16+
- **Compilatore C++17** (MSVC 2019+, GCC 9+, Clang 10+)

#### Compilazione all'interno di DLT Viewer (consigliata)

1. **Clona DLT Viewer:**
   ```bash
   git clone https://github.com/COVESA/dlt-viewer.git
   cd dlt-viewer
   ```

2. **Copia il plugin** nella directory dei plugin:
   ```bash
   cp -r /path/to/dlt-chat-plugin plugin/dlt-chat-plugin
   ```

3. **Configura con CMake:**
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```
   Su Windows con MSVC:
   ```cmd
   cmake -G "Visual Studio 17 2022" -A x64 ..
   ```

4. **Compila:**
   ```bash
   cmake --build . --parallel
   ```

5. **Output:**
   - Linux:   `build/bin/plugins/libdltchatplugin.so`
   - Windows: `build/bin/plugins/Release/dltchatplugin.dll` (MSVC)

6. **Installa** nella cartella plugins di DLT Viewer:
   - Linux:   `cp build/bin/plugins/libdltchatplugin.so ~/.local/share/dlt-viewer/plugins/`
   - Windows: copia la DLL in `%LOCALAPPDATA%\Programs\dlt-viewer\plugins\`

#### Compilazione autonoma

```bash
cd dlt-chat-plugin
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

Oppure su Windows, esegui `build_plugin.bat` da un Developer Command Prompt for VS 2022.

---

## Installation / Installazione

- **Windows:** Copy `dltchatplugin.dll` to `%LOCALAPPDATA%\Programs\dlt-viewer\plugins\`
- **Linux:** Copy `libdltchatplugin.so` to `~/.local/share/dlt-viewer/plugins/`

Launch DLT Viewer, enable the plugin via **Settings > Plugin Settings > Chat Log Assistant**, then enable the panel via **View > Panels > Chat Log Assistant**.

---

## Usage / Utilizzo

### Quick Action Buttons / Bottoni Rapidi (26)

#### Row 0: Level filters / Livelli

| Button | Description |
|--------|-------------|
| Errors | Find error and fatal messages |
| Warnings | Find warning messages |
| Info | Find informational messages |
| Debug | Find debug messages |
| Verbose | Find verbose messages |

#### Row 1: Category filters / Categorie

| Button | Description |
|--------|-------------|
| CAN | Find CAN bus messages |
| Security | Find auth/security issues |
| Memory | Find memory-related problems |
| Performance | Find timeout and delay issues |
| Diagnostic | Find diagnostic trouble codes |
| GPS | Find GPS/navigation messages |
| Categories | Show available filter categories |

#### Row 2: Automotive presets / Preset automotive

| Button | Description |
|--------|-------------|
| CarPlay | Filter CarPlay domain messages |
| AndroidAuto | Filter Android Auto domain messages |
| Focus | Find video focus lost events |
| Ducking | Find audio ducking events |
| mDNS | Find mDNS handshake events |
| Sensor | Find vehicle sensor data |
| Auth Errors | Find authentication errors |

#### Row 3: Analysis & commands / Analisi e comandi

| Button | Description |
|--------|-------------|
| Session | Find session start/stop events |
| Pattern | Find repeated message patterns |
| Summary | Show log statistics |
| Timeline | Show chronological entry list |
| Categorizza | Classify errors by category |
| Help | Show available commands |
| Keywords | Show available keywords |

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

### Automotive Domain Classification

The plugin automatically classifies log entries into automotive domains:

**CarPlay:** APID `com.apple.carplay`, keywords `iap2`, `AirPlay`, `CARSIM`
- Recognized events: video_focus_lost, audio_ducking, mdns_handshake, hid_event, auth_tls

**Android Auto:** APID `CarAppService`, `AOAP`, `AndroidAuto`, keyword `USB_ACCESSORY`, `AOA`
- Recognized events: sensor_data, audio_focus, mdns_handshake, session_start, session_stop

### Error Categorization (7 categories)

The `categorizza` command classifies errors into:

| Category | Examples |
|----------|----------|
| Comunicazione | timeout, connection refused, link down |
| Memoria | out of memory, allocation failed |
| Sicurezza | auth failed, invalid certificate |
| Configurazione | invalid config, missing parameter |
| Hardware | hardware fault, sensor failure |
| Timeout | timeout error |
| Protocollo | protocol error, malformed response |

---

## Configuration / Configurazione

Example `dlt_chat_plugin.ini`:

```ini
[Analyzer]
type=rule-based
llmEndpoint=http://localhost:11434/api/generate
llmApiKey=
llmModel=llama3.2:1b
bulkAnalysisEnabled=false

[Behavior]
maxResults=1000
llmTimeout=120
highlightColor=#FFE680
userFiltersPath=
```

### User Filters

Load a JSON file with custom regex-based highlight rules via the **Filters** button. Schema:

```json
{
  "version": "1.0",
  "filters": [
    {
      "label": "iAP2 Connection Error",
      "pattern": "iap2.*(error|fail|timeout)",
      "fields": ["payload"],
      "color": "#FF4444",
      "level": ["error", "fatal"],
      "domain": "carplay",
      "enabled": true
    }
  ]
}
```

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

## License / Licenza

Mozilla Public License 2.0 (MPL-2.0). See [LICENSE](LICENSE).
