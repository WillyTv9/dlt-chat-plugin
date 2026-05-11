# DLT Chat Plugin - Configuration Guide

## Overview

The DLT Chat Plugin can be configured to use different analyzers:
1. **Rule-Based Analyzer** (default) - Fast, keyword-based, no external dependencies
2. **LLM Analyzer** - AI-powered analysis using OpenAI, Ollama, or LocalAI

---

## Configuration File

### Location
The configuration file should be named:
```
dlt_chat_plugin.ini
```

And placed in one of these locations:
- Same directory as the plugin binary
- DLT Viewer plugins configuration directory
- User's home config directory

### Creating Configuration

1. Copy `dlt_chat_plugin.ini.example` to `dlt_chat_plugin.ini`
2. Edit the file with your preferred analyzer settings
3. Restart DLT Viewer to apply changes

---

## Analyzer Configuration

### 1. Rule-Based Analyzer (Default)

**Pros:**
- No external dependencies
- Fast, real-time processing
- Works offline
- No API keys required

**Configuration:**
```ini
[Analyzer]
type=rule-based
```

**Supported Languages:** English, Italian, German, Spanish, French

**Keywords:** 20+ categories including errors, CAN bus, timing, memory, security, etc.

---

### 2. LLM Analyzer

#### Option A: Ollama (Recommended for Local)

**Ollama** - Free, open-source LLM framework

**Installation:**
1. Download Ollama from https://ollama.ai
2. Install and run: `ollama serve`
3. Pull a model: `ollama pull qwen3.5:4b` (or other models)
4. Verify running at http://localhost:11434

**Configuration:**
```ini
[Analyzer]
type=llm

[LLM]
llmEndpoint=http://localhost:11434
llmApiKey=
llmModel=qwen3.5:4b
```

**Recommended Models:**
- `qwen3.5:4b` - Balanced, multilingual (recommended)
- `llama3` - Larger, better quality
- `neural-chat` - Optimized for chat
- `mistral` - Fast, good quality

#### Option B: OpenAI (Cloud-based)

**Configuration:**
```ini
[Analyzer]
type=llm

[LLM]
llmEndpoint=https://api.openai.com/v1/chat/completions
llmApiKey=sk-xxxxxxxxxxxxxxxx
llmModel=gpt-3.5-turbo
```

**Get API Key:**
1. Visit https://platform.openai.com/account/api-keys
2. Create new secret key
3. Add to configuration

**Cost:** Variable based on token usage

#### Option C: LocalAI (Self-hosted)

**Configuration:**
```ini
[Analyzer]
type=llm

[LLM]
llmEndpoint=http://localhost:8000/v1/chat/completions
llmApiKey=
llmModel=gpt-4
```

**Installation:**
- See https://localai.io for setup

---

## Advanced Configuration

### Behavior Settings

```ini
[Behavior]
# Maximum results per query
maxResults=200

# LLM request timeout (milliseconds)
llmTimeout=30000

# Highlight color (RGB hex)
highlightColor=#FFE680
```

### Performance Tuning

For large log files (>1GB):
- Use rule-based analyzer for faster queries
- Increase `llmTimeout` if using LLM
- Reduce `maxResults` for faster processing

---

## Troubleshooting

### "LLM not available"
- Check if endpoint is running
- Verify model name is correct
- Check API key (if required)
- Review plugin log for network errors

### "Timeout in LLM response"
- Increase `llmTimeout` value
- Reduce number of log entries in analysis
- Check network connection
- Try simpler model

### "CSV export missing data"
- Data fields come from log file parsing
- Some fields may not be available
- Check source DLT file format

### Plugin not loading configuration
- Ensure file is named exactly: `dlt_chat_plugin.ini`
- Check file permissions (must be readable)
- Verify syntax (no typos in [Analyzer] section)
- Check DLT Viewer plugin directory permissions

---

## Environment Variables (Optional)

The plugin respects these environment variables:

```bash
# Set custom config file location
export DLT_CHAT_CONFIG=/path/to/config.ini

# Enable debug logging
export DLT_CHAT_DEBUG=1

# Set default analyzer
export DLT_CHAT_ANALYZER=llm
```

---

## Common Issues

| Issue | Solution |
|---|---|
| Plugin crashes on startup | Check configuration file syntax |
| LLM queries timeout | Increase `llmTimeout`, use simpler model |
| High memory usage | Reduce `maxResults`, close other applications |
| No results found | Try different keywords, check rule-based keywords list |
| Configuration not applied | Restart DLT Viewer after changing config |

---

## Support

For issues or questions:
1. Check CODE_REVIEW_REPORT.md for known issues
2. Review INSTALL.md for installation steps
3. Create GitHub issue with:
   - DLT Viewer version
   - Plugin version
   - Configuration used
   - Steps to reproduce

