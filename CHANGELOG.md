# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.1] — 2026-05-17

### Added
- Dynamic version system: single source of truth in CMakeLists.txt, auto-propagated to all binaries
- GitHub community files: bug report / feature request templates, PR template, CONTRIBUTING.md, CHANGELOG.md
- Windows Qt path auto-detection in CMake (msvc/mingw fallback)
- CTest labels for all test targets (`ctest -L unit`)
- `--help` flag support for setup.sh and format.sh
- Test coverage for VectorDB, Embedder, KnowledgeBase, and ApiProvider (SSE parsing)

### Fixed
- Embedder ONNX name memory leak on model reload
- KnowledgeBase thread safety with mutex for async document import
- SessionManager debounced file saves to reduce IO during agent loops
- C++ syntax highlighter incorrectly matching hex color codes as preprocessor directives
- CommandExecutor env var expansion bounded to prevent infinite loops on circular references
- Version numbers unified across CLI (was 1.0.0), GUI (was 1.1.0), and CMake (1.1.1)

### Changed
- zh_TW/HK locale fallback to Simplified Chinese documented with explanatory comments
