# Chat Log Assistant Plugin / Plugin Assistente Log Chat

[![License: MPL 2.0](https://img.shields.io/badge/License-MPL_2.0-blue.svg)](LICENSE)
[![Qt Version](https://img.shields.io/badge/Qt-5.15+%2F6.x-green.svg)](https://www.qt.io/)
[![DLT Viewer](https://img.shields.io/badge/DLT_Viewer-2.30.0+-orange.svg)](https://github.com/COVESA/dlt-viewer)
[![Version](https://img.shields.io/badge/Version-0.3.0-blue.svg)](.)
[![Presentation](https://img.shields.io/badge/📊-Presentation-blue?style=flat&labelColor=555)](https://WillyTv9.github.io/dlt-chat-plugin/)

A chat-based log analysis plugin for COVESA DLT Viewer with rule-based analysis and optional AI (LLM) integration.

Plugin per l'analisi dei log DLT con interfaccia chat, analisi rule-based e supporto AI opzionale.

---

## Build Guide / Guida alla compilazione

### English

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

4. **Build the plugin:**
   ```bash
   cmake --build . --target dltchatplugin --parallel
   ```
   Or build everything:
   ```bash
   cmake --build . --parallel
   ```

5. **Output:**
   - Linux:   `build/bin/plugins/libdltchatplugin.so`
   - Windows: `build/bin/plugins/libdltchatplugin.dll` (MinGW)
   - Windows: `build/bin/plugins/Release/dltchatplugin.dll` (MSVC)

6. **Install to DLT Viewer plugins folder:**
   - Linux:   `cp build/bin/plugins/libdltchatplugin.so ~/.local/share/dlt-viewer/plugins/`
   - Windows: copy the DLL to `%LOCALAPPDATA%\Programs\dlt-viewer\plugins\`

#### Building standalone

```bash
cd plugin/dlt-chat-plugin
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

Or on Windows, double-click `build_plugin.bat` from a Visual Studio Developer Prompt.

---

### Italiano

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
   Su Windows con MinGW (MSYS2):
   ```bash
   cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
   ```
   Su Windows con MSVC:
   ```cmd
   cmake -G "Visual Studio 17 2022" -A x64 ..
   ```

4. **Compila il plugin:**
   ```bash
   cmake --build . --target dltchatplugin --parallel
   ```
   Oppure compila tutto:
   ```bash
   cmake --build . --parallel
   ```

5. **Output:**
   - Linux:   `build/bin/plugins/libdltchatplugin.so`
   - Windows: `build/bin/plugins/libdltchatplugin.dll` (MinGW)
   - Windows: `build/bin/plugins/Release/dltchatplugin.dll` (MSVC)

6. **Installa** nella cartella plugins di DLT Viewer:
   - Linux:   `cp build/bin/plugins/libdltchatplugin.so ~/.local/share/dlt-viewer/plugins/`
   - Windows: copia la DLL in `%LOCALAPPDATA%\Programs\dlt-viewer\plugins\`

#### Compilazione autonoma

```bash
cd plugin/dlt-chat-plugin
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

Oppure su Windows, esegui `build_plugin.bat` da un Developer Command Prompt for VS 2022.

---

## Installation / Installazione

- **Windows:** Copy `libdltchatplugin.dll` / `dltchatplugin.dll` to `%LOCALAPPDATA%\Programs\dlt-viewer\plugins\`
- **Linux:** Copy `libdltchatplugin.so` to `~/.local/share/dlt-viewer/plugins/`

Launch DLT Viewer, enable the plugin via **Settings > Plugin Settings > Chat Log Assistant**, then enable the panel via **View > Panels > Chat Log Assistant**.

## Usage / Utilizzo

### Quick Action Buttons / Bottoni Rapidi (21)

| English | Italiano | Description |
|---------|----------|-------------|
| Errors | Errori | Find error and fatal messages |
| Warnings | Warnings | Find warning messages |
| Info | Info | Find informational messages |
| Debug | Debug | Find debug messages |
| CAN | CAN | Find CAN bus messages |
| Security | Security | Find auth/security issues |
| Memory | Memoria | Find memory-related problems |
| Performance | Performance | Find timeout and delay issues |
| Diagnostic | Diagnostic | Find diagnostic trouble codes |
| Pattern | Pattern | Find repeated message patterns |
| Summary | Summary | Show log statistics |
| Timeline | Timeline | Show chronological entry list |
| GPS | GPS | Find GPS/navigation messages |
| Categorizza | Categorizza | Classify errors by category |
| Help | Help | Show available commands |

### AI Query

1. Click the gear icon (⚙) to open configuration
2. Select provider (Ollama, OpenAI, LocalAI, Custom)
3. Enter endpoint, API key (if needed), model name
4. Click **Test Connection** to verify
5. Save - green indicator confirms AI is ready

Type questions in the **AI** section:
- *"What caused the timeout?"*
- *"Show me the sequence of errors before the crash"*
- *"Quali sono i pattern di errore piu frequenti?"*

If AI is unavailable, falls back to rule-based analysis automatically.

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
highlightColor=#FFE680
```

## License / Licenza

Mozilla Public License 2.0 (MPL-2.0). See [LICENSE](LICENSE).
