# CHANGELOG

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.1] - 2026-05-11

### Added
- CONFIGURATION.md - Comprehensive configuration guide
- QUICKSTART.md - Quick start guide for new users
- dlt_chat_plugin.ini.example - Configuration file template
- CODE_REVIEW_REPORT.md - Detailed code quality report
- QEventLoop timeout mechanism for LLM requests
- Mutex protection for request state (race condition fix)
- Null pointer checks in findRowForIndex()
- Support for configurable Ollama endpoint and model
- Enhanced error logging in network request handling
- Debug logging for LLM initialization

### Fixed
- **CRITICAL:** UI freezing due to busy-wait loop in LLM analyzer (replaced with QEventLoop)
- **CRITICAL:** Potential memory leak in QNetworkReply handling
- **CRITICAL:** Race condition in m_requestInProgress flag
- CSV export header to include all relevant fields
- Removed unused `isOllama` variable
- Improved null pointer safety in row finding
- Enhanced cleanup of network connections on error
- Better error messages for LLM configuration issues

### Changed
- LLM request handling now uses QEventLoop instead of busy-wait
- Improved logging verbosity for troubleshooting
- Enhanced configuration documentation
- Better error messages in analyzer responses

### Improved
- Code quality and best practices
- Documentation completeness
- User experience with configuration guides
- Error handling and cleanup procedures

---

## [0.2.0] - 2026-04-15

### Added
- LLM Analyzer support (OpenAI, Ollama, LocalAI)
- Multi-language support (EN, IT, DE, ES, FR)
- CSV export functionality
- Quick action buttons for common queries
- Manual highlighting system
- Configuration file support (INI format)

### Fixed
- Log entry parsing for various DLT formats
- Memory usage optimization for large files
- UI responsiveness improvements

---

## [0.1.0] - 2026-03-01

### Added
- Initial MVP release
- Rule-based analyzer with keyword matching
- Basic chat interface
- DLT file integration
- Summary and statistics
- Pattern detection
- Context navigation

---

## Known Issues

### Version 0.2.1
- CSV export may not include all metadata fields if source entries are not available
- LLM response parsing may vary depending on model output format
- Large log files (>1GB) may require more memory with LLM analyzer

---

## Deprecated

- None currently

---

## Security

### Vulnerabilities Fixed
- Removed hardcoded credentials/endpoints
- Added proper input validation for file operations
- Improved network request security

---

## Testing

### Test Coverage
- Rule-based analyzer: ✅ Tested with various log formats
- LLM analyzer: ⚠️ Manual testing only (no unit tests)
- CSV export: ✅ Tested with various result sets
- UI components: ⚠️ Visual testing only

### Tested Platforms
- Windows 10/11 (Qt 5.15.x)
- Linux (Qt 5.15.x / Qt 6.x)
- macOS (Qt 5.15.x)

---

## Migration Guide

### From 0.1.0 to 0.2.1

**Breaking Changes:** None

**New Configuration Options:**
```ini
[Analyzer]
type=rule-based  # or "llm"

# For LLM:
llmEndpoint=http://localhost:11434
llmApiKey=
llmModel=qwen3.5:4b
```

**Migration Steps:**
1. Backup existing configuration
2. Update plugin binary
3. (Optional) Create dlt_chat_plugin.ini with new settings
4. Restart DLT Viewer

---

## Contributors

- Initial development: University project
- Review and improvements: 2026

---

## References

- [COVESA DLT Viewer](https://github.com/COVESA/dlt-viewer)
- [Qt Documentation](https://doc.qt.io)
- [Keep a Changelog](https://keepachangelog.com/)

---

## Future Roadmap

### Version 0.3.0 (Planned)
- [ ] Unit tests for all analyzers
- [ ] Advanced filtering UI
- [ ] Real-time log analysis
- [ ] Custom keyword categories
- [ ] Database backend for large logs
- [ ] Web interface (optional)

### Version 0.4.0+ (Ideas)
- [ ] Multi-file analysis
- [ ] Machine learning anomaly detection
- [ ] Custom rules engine
- [ ] Plugin chaining
- [ ] REST API for programmatic access

---

**Latest Version:** 0.2.1  
**Last Updated:** 2026-05-11
