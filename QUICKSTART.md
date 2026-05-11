# DLT Chat Plugin - Quick Start Guide

## What is DLT Chat Plugin?

A smart chat-based interface for analyzing DLT (Diagnostic Log and Trace) files in the COVESA DLT Viewer.

**Ask questions about your logs in natural language and get instant answers with direct navigation to relevant entries.**

---

## 5-Minute Setup

### 1. Installation
- Download the plugin binary for your platform
- Copy to DLT Viewer plugins directory
- Restart DLT Viewer

### 2. Open a DLT File
1. Launch DLT Viewer
2. File → Open → Select your DLT log file
3. The plugin window appears automatically

### 3. Ask Questions

**Try these queries:**
- `error` - Find all error messages
- `warn` - Show all warnings
- `can` - Filter CAN bus messages
- `timeout` - Find timing issues
- `summary` - Get log summary statistics
- `pattern` - Find repetitive messages

**Or use full questions:**
- "What errors happened after timestamp 1000?"
- "Show me all CAN bus communication"
- "Find network timeout issues"

### 4. Navigate Results
- Results appear in the list below
- Click any result to jump to that log entry
- Use "Highlight" to mark all matching entries

### 5. Export Results
- Click "Esporta CSV" to export results
- Use in Excel, Python, etc. for further analysis

---

## Key Features

### 🎯 Quick Action Buttons
- **Errori** - Find all errors instantly
- **Warnings** - Show warning messages
- **CAN** - Filter CAN bus data
- **Timeout** - Find timing/performance issues
- **Pattern** - Detect repetitive messages
- **Timeline** - View log sequence
- **Summary** - Get statistics
- **Help** - View available keywords

### 🌍 Multi-Language Support
English, Italian, German, Spanish, French

### 🧠 Dual Analysis Mode
- **Rule-Based** (default) - Fast, no setup needed
- **AI-Powered** (optional) - Use Ollama/OpenAI for intelligent analysis

### 📊 Export Results
- Export query results to CSV
- Export entire log to CSV
- Include metadata and timestamps

---

## Usage Examples

### Finding Errors
```
Query: "error"
Result: All error-level messages with indices and context
Action: Click any result to jump to that log entry
```

### CAN Bus Analysis
```
Query: "can communication"
Result: All CAN-related messages with identifiers
Action: Export to CSV for detailed analysis
```

### Performance Issues
```
Query: "timeout delay latency"
Result: All messages related to timing issues
Context: Shows surrounding messages for context
```

### Log Statistics
```
Query: "summary"
Result: 
- Total messages: X
- Errors: Y, Warnings: Z
- Most active contexts (ECU/APP combinations)
- Most frequent message patterns
```

---

## LLM Integration (Optional)

For AI-powered analysis, set up an LLM:

### Quick Setup with Ollama

1. **Install Ollama:** https://ollama.ai
2. **Run:** `ollama serve`
3. **Pull Model:** `ollama pull qwen3.5:4b`
4. **Configure:**
   - Type: `LLM` in plugin settings
   - Endpoint: `http://localhost:11434`
   - Model: `qwen3.5:4b`

### Or Use OpenAI

1. Get API key from https://openai.com/api
2. Configure plugin with your API key
3. Start asking questions

---

## Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Enter` | Send query |
| `Ctrl+R` | Clear results |
| `Ctrl+E` | Export to CSV |

---

## Tips & Tricks

1. **Refine Your Queries**
   - Be specific: "error in sensor" vs just "error"
   - Use timestamps: "error at 1234.5678"
   - Combine keywords: "can timeout"

2. **Use Quick Buttons**
   - Fastest way to analyze common issues
   - Click once for instant results

3. **Check Summary First**
   - Get overview with "summary" query
   - Then drill down into specific areas

4. **Export for Sharing**
   - Export results to CSV
   - Share with team or stakeholders
   - Include query used for reproducibility

5. **Multi-File Analysis**
   - Open different DLT files
   - Plugin context resets per file
   - Results export includes file reference

---

## Common Queries

| Goal | Query | Expected Results |
|---|---|---|
| Find errors | error / errore | Error messages only |
| Communication issues | can network ethernet | Network-related logs |
| Performance | timeout delay latency | Timing-related entries |
| Find patterns | pattern ripeti | Repeated messages |
| Statistics | summary riassumi | Log statistics |
| Specific index | index 42 / riga 42 | Entry #42 + context |
| Time range | timestamp 1000 1100 | Messages in time window |
| By level | warn fatal | Specific severity levels |

---

## Troubleshooting

### Plugin doesn't appear
- Check DLT Viewer plugins folder
- Verify plugin binary has correct extension (.so, .dll, .dylib)
- Restart DLT Viewer

### No results found
- Try simpler keywords
- Check if log file is loaded
- Use "summary" to verify log is accessible
- Type "help" or "?" for keyword categories

### LLM queries slow
- Use fewer log entries
- Switch to rule-based analyzer
- Check network connection (for cloud LLMs)

### Export fails
- Verify folder has write permissions
- Try different filename
- Check disk space

---

## Next Steps

1. **Learn More:** Read CONFIGURATION.md
2. **Integrate LLM:** See CONFIGURATION.md → LLM Setup
3. **Advanced Usage:** Check CODE_REVIEW_REPORT.md
4. **Get Help:** See CONTRIBUTING.md

---

**Happy analyzing! 🚀**

For issues: Create GitHub issue with log file and query used
