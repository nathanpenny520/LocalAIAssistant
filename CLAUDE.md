# CLAUDE.md

<!-- PROJECT_ROOT: /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant -->
<!-- Always use: git -C PROJECT_ROOT <command> -->

## Build Commands

```bash
# CMakeLists.txt auto-prepends ~/Qt/6.10.3/macos to CMAKE_PREFIX_PATH.
# Override with: cmake -B build -DQT_PATH=/custom/qt/path
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4                    # Fast build (preferred)
./scripts/build.sh                                  # Full release build
./scripts/build.sh -c -d --no-run                   # Clean debug, no prompt
./scripts/build.sh test                             # Build and run unit tests
./scripts/build.sh run --gui                        # Run GUI
./scripts/build.sh run --cli                        # Run CLI
./scripts/build.sh build -p                         # Build + package (DMG/zip)
./scripts/build.sh package                          # Package existing build
./scripts/build.sh package --nsis                   # Package with NSIS installer (Windows)
./scripts/build.sh package --appimage               # Package with AppImage (Linux)

# Standalone packaging (CI-friendly, no build.sh dependency)
./scripts/package.sh --help                         # Full packaging options
./scripts/package.sh --build-dir build              # Auto-detect platform, create archive
./scripts/package.sh --build-dir build --nsis       # + NSIS installer
./scripts/package.sh --build-dir build --appimage   # + AppImage
```

**Note**: `build.sh` has an interactive prompt. Use `--no-run`, `--gui`, or `--cli` to skip it.
Prefer `cmake --build` for non-interactive use. Packaging delegates to `scripts/package.sh`.

## Git Workflow

- Commit after every change. Never accumulate unrelated edits.
- Format: `type(scope): brief description` (types: `feat`, `fix`, `refactor`, `docs`, `test`,
  `chore`)
- Commit locally first, push only after verifying `cmake --build` succeeds.
- Rollback: `git reset --soft HEAD~1` (keep changes) or `--hard` (discard).
- Always use: `git -C /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant <cmd>`

## Architecture

Qt 6 C++17 desktop app with GUI and CLI dual-mode. Four CMake libraries, two executables:

```
LocalAIAssistantCore  (src/core/, src/prompts/)   — network, sessions, file I/O, prompts
    ↑
    ├── TaskModule     (src/tasks/)               — file ops, safety checker, undo, command exec
    ├── KnowledgeModule (src/knowledge/)           — chunking, embedding, vector DB, doc import
    └── GirlfriendModule (src/girlfriend/)         — AI girlfriend: avatar, voice, memory, personality
            ↑
            └── LocalAIAssistant (GUI, src/ui/main.cpp → MainWindow)
```

- **Core**: `NetworkManager` (OpenAI/Ollama/LlamaCpp/Anthropic APIs via Provider pattern),
  `SessionManager` (JSON persistence), `FileManager`, `PromptManager`
- **TaskModule**: `TaskEngine` (AI response parsing, TASK_PLAN extraction), `AgentLoop` (state
  machine: plan→execute→feedback→continue cycle, detects `[TASK_COMPLETE]`), `CommandExecutor`
  (native file ops via Qt + shell commands with auto-detection), `SafetyChecker` (three-tier:
  Tier 1 Blocked / Tier 2 NeedsConfirmation / Tier 3 Approved, with `PathViolation` tracking),
  `OperationUndo`. Task prompt merged into system prompt — AI self-judges when to generate
  TASK_PLAN. All plans require user confirmation before execution (CLI: `/confirm` or inline
  `[Y/n]`, GUI: dialog with per-path Allow Once/Always/Deny). `--yes` flag auto-confirms Tier 2
  warnings for scripting (Tier 1 never bypassed).
- **GirlfriendModule** (single-file `girlfriendwindow.cpp`, 1,937 lines — split deferred per
  ROADMAP)
- **CLI**: links Core + TaskModule only (no Girlfriend, no Knowledge)
- **GUI**: links GirlfriendModule → transitively pulls in Core
- Settings: `QSettings("LocalAIAssistant", "Settings")`
- **UI Theme**: `AppTheme` color token system (`src/ui/apptheme.h`) — light/dark themes with ~40
  semantic color tokens. `StyleSheetManager` (singleton) generates QSS from tokens.
  `MarkdownRenderer::toHtml()` renders markdown theme-aware without requiring a theme parameter.

## Code Standards

### File & Function Size

- **Files ≤ 500 lines** (see ROADMAP.md for current violations)
- **Functions ≤ 50 lines** — extract private helpers aggressively
- One class / one concern per file. Qt signals/slots for cross-object communication.

### Naming & Style

- Files: `snake_case.cpp/h` | Types: `PascalCase` | Variables: `snake_case`, members `m_`
- Functions: `PascalCase` | Constants: `kPascalCase` or `ALL_CAPS`
- Include order: related header → C std → C++ std → Qt → other libs → project headers
- Run `clang-format -i <file>` before commit. CMake: `cmake-format -i CMakeLists.txt`.
- Full code standards: see `docs/design/CODE_STANDARDS.md`

### Comments（IMPROTANT）

- English only. Headers: Doxygen (`@brief`, `@param`, `@return`). Implementation: explain WHY, not
  WHAT.
- Tags: `// TODO(name):`, `// FIXME:`, `// HACK:`

## Testing

- Framework: Qt Test (`find_package(Qt6 REQUIRED COMPONENTS Test)`)
- Directory: `tests/` with own `CMakeLists.txt`. Files: `tests/test_<module>.cpp`
- Run: `ctest` from `build/`
- All new code requires tests. Bug fixes require regression tests.

## Roadmap

See `ROADMAP.md` for full refactoring plan, priority order, known issues, and verification
checklist.

## Documentation Maintenance

After every major fix or feature, update these docs if the changes affect them:

- **ROADMAP.md** — mark completed items, update line counts, add new findings
- **USAGE.md / USAGE_zh_CN.md** — if UI or user-facing behavior changed
- **README.md / README_EN.md** — if features, build steps, or config changed
- **CLAUDE.md** — if architecture, build commands, or standards changed

Do not wait for the user to ask. Review docs as the final step of any non-trivial change.

## Packaging

- **`scripts/package.sh`** — Standalone cross-platform packaging (797 lines). Self-sufficient:
  runs `macdeployqt`/`windeployqt` itself, does not depend on prior build steps.
- **`scripts/version.sh`** — Shared version source. Extracts `VERSION X.Y.Z` from CMakeLists.txt.
  Sourced by both `build.sh` and `package.sh`.
- **macOS**: DMG via `hdiutil`, with `--sign`/`--notarize` stubs (future).
- **Windows**: ZIP with DLL fix (copies all adjacent `.dll` files). `--nsis` creates
  NSIS installer (requires `makensis`). Falls back to ZIP if not installed.
- **Linux**: tar.gz with embedded `install.sh`. `--appimage` creates AppImage (requires
  `linuxdeployqt` + `appimagetool`). Falls back to tar.gz if not installed.
- **SHA256SUM**: Generated for all artifacts in output directory.
- **CI**: `.github/workflows/build.yml` uploads artifacts for all 3 platforms.

## Platform Notes

- **macOS**: `libedit` (readline-compatible). `macdeployqt` bundles frameworks.
- **Windows**: `windeployqt` deploys DLLs. MinGW from `Qt/Tools/mingwXXX_64/`.
