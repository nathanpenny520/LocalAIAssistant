# CLAUDE.md

## Build Commands

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4                    # Fast build (preferred)
./scripts/build.sh                                  # Full release build
./scripts/build.sh -c -d --no-run                   # Clean debug, no prompt
./scripts/build.sh test                             # Build and run unit tests
./scripts/build.sh run --gui                        # Run GUI
./scripts/build.sh run --cli                        # Run CLI
./scripts/build.sh build -p                         # Build + package
```

**Note**: Prefer `cmake --build` for non-interactive use. `build.sh` has an interactive prompt —
use `--no-run`, `--gui`, or `--cli` to skip it. Packaging delegates to `scripts/package.sh`.

## Git Workflow

- Commit after every change. Format: `type(scope): brief description` (`feat`, `fix`, `refactor`,
  `docs`, `test`, `chore`)
- Push only after `cmake --build` succeeds.
- Rollback: `git reset --soft HEAD~1` (keep changes) or `--hard` (discard).

## Architecture

Qt 6 C++17 desktop app. Five CMake libraries, two executables (GUI + CLI).

```
FileParser              (src/parsers/)              — PDF/DOCX/text parsing
    ↑
LocalAIAssistantCore   (src/core/, src/prompts/)   — network, sessions, providers, prompts, theme
    ↑
    ├── TaskModule      (src/tasks/)               — task engine, agent loop, safety checker, undo
    ├── KnowledgeModule (src/knowledge/)            — embedding, vector DB, chunking, doc import
    └── GirlfriendModule (src/girlfriend/)          — girlfriend window, voice, avatar, personality
            ↑
            └── LocalAIAssistant (GUI, src/ui/)

LocalAIAssistant-CLI (src/cli/)                    — links Core + Task + Knowledge (no Girlfriend)
```

- **Core**: `NetworkManager` (OpenAI/Ollama/LlamaCpp/Anthropic via Provider pattern),
  `SessionManager`, `FileManager`, `PromptManager`, `AppTheme`
- **FileParser**: Standalone lib for PDF (poppler), DOCX (libzip+pugixml), text. Separate to allow
  new formats without touching Core.
- **TaskModule**: `TaskEngine` (TASK_PLAN extraction), `AgentLoop` (plan→execute→feedback cycle),
  `CommandExecutor` (native ops + shell), `SafetyChecker` (3-tier: Blocked/NeedsConfirmation/
  Approved), `OperationUndo`. Plans require user confirmation (CLI: `/confirm`, GUI: dialog).
  `--yes` bypasses Tier 2 only.
- **KnowledgeModule**: `Embedder` (ONNX), `VectorDB` (hnswlib), `TextChunker`, `KnowledgeBase`,
  `DocImporter`
- **GirlfriendModule**: `GirlfriendWindow`, `VoiceManager` (XFYUN TTS/ASR), `PersonalityEngine`,
  `AvatarWidget`, `MemoryManager`
- Settings: `QSettings("LocalAIAssistant", "Settings")`

## Code Standards

- **Files ≤ 500 lines** (see ROADMAP.md for violations), **functions ≤ 50 lines**
- One class per file. Qt signals/slots for cross-object communication.
- Naming: `snake_case.cpp/h` files, `PascalCase` types, `snake_case` vars, `m_` members,
  `kPascalCase` constants
- Include order: related header → C std → C++ std → Qt → other libs → project headers
- Formatters: `clang-format -i`, `cmake-format -i CMakeLists.txt`,
  `shfmt -i 4 -ci -bn -w scripts/*.sh`, `prettier --write`
  (see `.clang-format`, `.cmake-format.json`, `.shfmtrc`, `.prettierrc`)
- English only comments. Doxygen in headers. Explain WHY, not WHAT.

## Testing

- Qt Test framework. `tests/` with own `CMakeLists.txt`. Run: `ctest` from `build/`.
- All new code requires tests. Bug fixes require regression tests.

## Roadmap

See `ROADMAP.md` for refactoring plan, known issues, and verification checklist.

## Packaging

- `scripts/package.sh` — Standalone cross-platform packaging. Runs `macdeployqt`/`windeployqt`
  itself.
- `scripts/version.sh` — Shared version source from `CMakeLists.txt`.
- macOS: DMG via `hdiutil`. Windows: ZIP + optional NSIS. Linux: tar.gz + optional AppImage.
- SHA256SUM generated for all artifacts. CI: `.github/workflows/build.yml`.

## Platform Notes

- macOS: `libedit` (readline-compatible). `macdeployqt` bundles frameworks.
- Windows: `windeployqt` deploys DLLs. MinGW from `Qt/Tools/mingwXXX_64/`.
