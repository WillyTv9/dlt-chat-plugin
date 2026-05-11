# Repository Cleanup Checklist

## ✅ Files to Keep

These files are essential for the MVP:

### Core Source
- `dltchatplugin.h/cpp` - Main plugin
- `chatform.h/cpp` - UI
- `dltanalyzerinterface.h/cpp` - Analyzer interface
- `dltchatanalyzer.cpp` - Rule-based analyzer
- `dltllmanalyzerinterface.h/cpp` - LLM analyzer
- `dltexport.h/cpp` - CSV export

### Build & Config
- `CMakeLists.txt` - Build configuration

### Documentation
- `README.md` - Main readme
- `INSTALL.md` - Installation guide
- `DOCUMENTATION.md` - Technical docs
- `CONFIGURATION.md` - Configuration guide
- `QUICKSTART.md` - Quick start
- `CHANGELOG.md` - Version history
- `CONTRIBUTING.md` - Contribution guidelines
- `CODE_REVIEW_REPORT.md` - Code review

### Configuration Examples
- `dlt_chat_plugin.ini.example` - Example config

### Licensing
- `LICENSE` - MPL 2.0 license

---

## 🗑️ Files to Remove (if present)

### Development/Testing Files
- `test_*.cpp`, `test_*.h` - Old test files
- `*_bak.cpp`, `*_backup.h` - Backup files
- `*.pro` - Old Qt .pro files
- `Makefile` (if auto-generated)
- `.qmake.stash` - Qt cache

### Build Artifacts
- `build/` directory
- `*.o`, `*.a`, `*.so`, `*.dll` - Compiled files
- `*.moc` - Qt meta-object files
- `*.ui` (if not in use)

### IDE & OS Files
- `.vs/` - Visual Studio
- `.vscode/` - VSCode settings (if not needed)
- `.idea/` - CLion/IDEA
- `.DS_Store` - macOS
- `Thumbs.db` - Windows
- `*.swp`, `*.swo` - Vim
- `*~` - Emacs

### Documentation (Old/Duplicate)
- `README_OLD.md`
- `NOTES.md` (unless important)
- `TODO.md` - Move to GitHub issues instead

### Deprecated Features
- Any `*_deprecated.cpp` files
- Legacy analyzer implementations

---

## 📝 Files to Review/Update

### Header Comments
- [ ] Add MPL 2.0 license header to all `.cpp` files
- [ ] Add license header to all `.h` files
- [ ] Format: See template below

### Documentation
- [ ] Verify all links in README work
- [ ] Check all code examples in QUICKSTART
- [ ] Update version numbers (0.2.1)
- [ ] Verify configuration examples

### Code Quality
- [ ] Remove commented-out code blocks
- [ ] Fix any compiler warnings
- [ ] Verify no hardcoded paths/credentials

---

## 📄 License Header Template

Add this to the top of every source file:

```cpp
/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include "filename.h"
// rest of file...
```

---

## 🔍 Files to Verify

### Compilation
```bash
# Clean build should have zero errors/warnings
cmake --build . --target dltchatplugin --config Release
```

### Static Analysis (optional)
```bash
# If cppcheck installed
cppcheck --enable=all src/
```

### Code Style (optional)
```bash
# If clang-format installed
clang-format -i *.cpp *.h
```

---

## 📦 Final Checklist

Before release:

- [ ] All critical bugs fixed (from CODE_REVIEW_REPORT.md)
- [ ] No uncommitted changes
- [ ] LICENSE file present
- [ ] README has "Quick Start" link
- [ ] QUICKSTART.md is accurate
- [ ] CONFIGURATION.md covers all options
- [ ] CHANGELOG.md updated
- [ ] No build artifacts in repo
- [ ] No sensitive data in files
- [ ] No broken links in documentation
- [ ] Example config file present
- [ ] All headers have license text

---

## 🚀 Release Process

1. **Version Bump**
   - Update version in `dltchatplugin.h` (#define DLT_CHAT_PLUGIN_VERSION)
   - Update CHANGELOG.md with new version
   - Update README.md version badge

2. **Final Testing**
   - Clean build from scratch
   - Test with sample DLT file
   - Test both analyzers (rule-based + LLM)
   - Test CSV export

3. **Documentation**
   - Verify all docs are up-to-date
   - Check all links are valid
   - Review examples

4. **Git Operations**
   - Stage all changes
   - Create annotated tag: `git tag -a v0.2.1 -m "Release 0.2.1"`
   - Push: `git push && git push --tags`

5. **Release Notes**
   - Create GitHub release with CHANGELOG excerpt
   - Attach pre-built binaries

---

## 📊 File Metrics

Current state:
- **Source Files:** 6 .cpp + 6 .h files (12 core files)
- **Documentation:** 7 markdown files
- **Configuration:** 1 example .ini file
- **Build:** 1 CMakeLists.txt
- **License:** 1 LICENSE file

**Total:** ~18 files for complete MVP

---

## Notes

- Keep repository clean and focused
- Remove any unused/experimental branches
- Don't store compiled binaries (use CI/CD releases)
- Document any workarounds or hacks
- Link issues to relevant code comments

