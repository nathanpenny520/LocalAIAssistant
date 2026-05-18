# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed
- Knowledge base import file dialog now shows all 70+ supported formats instead of only 4

## [1.1.2] — 2026-05-18

### Added
- Dynamic version system: single source of truth in CMakeLists.txt, auto-propagated to all binaries
- GitHub community files: bug report / feature request templates, PR template, CONTRIBUTING.md, CHANGELOG.md
- Windows Qt path auto-detection in CMake (msvc/mingw fallback)
- CTest labels for all test targets (`ctest -L unit`)
- `--help` flag support for setup.sh and format.sh
- Test coverage for VectorDB, Embedder, KnowledgeBase, and ApiProvider (SSE parsing)
- `scripts/format.sh` for unified code formatting (clang-format, cmake-format, shfmt)
- GitHub Actions format-check workflow for PRs
- DOCX/PDF parsing support on Windows CI via vcpkg
- Current date/time injection into system prompts
- Right-click context menu for chat display and input line
- MCP (Model Context Protocol) integration plan document
- Enterprise-level code review report and improvement plan documents

### Fixed
- Phase 1 stability fixes: 6 critical bugs in core (Embedder ONNX name leak, KnowledgeBase thread safety, SessionManager debounced file saves)
- Reverted Fix 1/4/6 to original behavior based on test feedback
- VectorDB SQLite connection name made unique per instance (fixes Windows test failure)
- C++ syntax highlighter incorrectly matching hex color codes as preprocessor directives
- CommandExecutor env var expansion bounded to prevent infinite loops on circular references
- Corrected vcpkg poppler package name and CMake target for Windows CI
- Added `set -euo pipefail` to build.sh, setup.sh, and cli-wrapper.sh

### Changed
- Project restructured: docs organized into user/development subdirectories
- Runtime assets (prompts, girlfriend images) moved from src/ to resources/
- AIGirlfriend/ directory renamed to resources/girlfriend/
- Extracted `copy_app_resources` CMake function to deduplicate post-build copies
- Deleted dead files, renamed memory.md, simplified CLAUDE.md
- Replaced non-functional `.shfmtrc` with `.editorconfig`
- Applied cmake-format and shfmt across the project
- Marked ROADMAP Phase 3 as completed
- Updated docs for Phase 1+3 changes
- Fixed stale personality.md/memory.md references in USAGE docs
- Cleaned up README files
- Version numbers unified across CLI, GUI, and CMake

## [1.1.1] — 2026-05-14

### Added
- AI-based affection evaluation for girlfriend module
- UI color improvements and internationalization (i18n) support

### Fixed
- Corrected night time range from <2 to <6 to cover 2-5 AM
- Split CONFIG key=value pairs into separate lines for correct prompt parsing
- Added English release notes to CI build workflow

### Changed
- Deleted obsolete Chinese documentation files (本地AI助手文档.md, WSL使用教程.md)
- Improved README formatting
- Wrapped CONFIG key=value pairs in code fences for markdown readability

## [1.1.0] — 2026-05-10

### Added
- Initial cross-platform release (macOS, Windows, Linux)
- Dual-mode architecture: GUI (Qt Widgets) + CLI
- Multi-provider AI support: OpenAI, Ollama, LlamaCpp, Anthropic
- Knowledge base with ONNX embeddings and HNSW vector search
- Document parsing: PDF (poppler), DOCX (libzip + pugixml), text/code
- Agent loop for autonomous task execution with safety checker
- Girlfriend module with TTS/ASR (iFlytek), avatar, and personality engine
- Theme system with light/dark modes and customizable colors
- Session management with streaming and markdown rendering
