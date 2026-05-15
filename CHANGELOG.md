# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.7.0] - 2026-05-15

### Added
- Enterprise Release Hardening: production-grade build system and CI/CD.
- Reusable GitHub Actions workflow for build and test.
- Support for `QDLT_ROOT` as a CMake variable and environment variable.
- Improved Linux support in `Findqdlt.cmake` for `.so` libraries.
- New `MessageRole` API in `chatform` for structured chat integration.
- Unified versioning via `PROJECT_VERSION` in `CMakeLists.txt`.
- Build information header `dltchat_version.h` with Git hash and timestamp.
- Distribution target `dist` to create ready-to-use plugin bundles.

### Changed
- Refactored GitHub Actions `build.yml` and `release.yml` for better reliability and caching.
- CI tests now run from the root CMake project instead of a standalone test folder.
- `build_plugin.bat` now correctly handles distribution packaging.
- Clang-tidy order in `CMakeLists.txt` fixed to avoid configuration warnings.
- Compiler warnings level increased and applied to all core targets.

### Fixed
- Fixed broken artifact paths in GitHub Actions.
- Fixed `Findqdlt` failing on Linux due to missing `.so` candidate names.
- Fixed potential linker errors in `chatform` by implementing missing API methods.
- Fixed duplicate distribution logic in `dist/CMakeLists.txt`.
- Fixed `.gitignore` missing compressed archive patterns.

## [0.6.0] - 2026-05-12

### Added
- Initial support for AI-based log analysis.
- Integration with Ollama and other LLM providers.
- Rule-based analyzer for quick pattern matching.
- Contextual extractor for providing log context to AI.
- Automotive log parser with presets for CarPlay, Android Auto, etc.
- Export results to CSV functionality.
