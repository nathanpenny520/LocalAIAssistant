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
./scripts/build.sh build -p                         # Package (DMG/zip)
```

**Note**: `build.sh` has an interactive prompt (line 870). Use `--no-run`, `--gui`, or `--cli` to skip it. Prefer `cmake --build` for non-interactive use.

## Git Workflow

- Commit after every change. Never accumulate unrelated edits.
- Format: `type(scope): brief description`  (types: `feat`, `fix`, `refactor`, `docs`, `test`, `chore`)
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

- **Core**: `NetworkManager` (OpenAI/Ollama/LlamaCpp APIs), `SessionManager` (JSON persistence), `FileManager`, `PromptManager`
- **GirlfriendModule** (single-file `girlfriendwindow.cpp`, 1,937 lines — split deferred per ROADMAP)
- **CLI**: links Core + TaskModule only (no Girlfriend, no Knowledge)
- **GUI**: links GirlfriendModule → transitively pulls in Core
- Settings: `QSettings("LocalAIAssistant", "Settings")`

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

### Comments
- English only. Headers: Doxygen (`@brief`, `@param`, `@return`). Implementation: explain WHY, not WHAT.
- Tags: `// TODO(name):`, `// FIXME:`, `// HACK:`

## Testing

- Framework: Qt Test (`find_package(Qt6 REQUIRED COMPONENTS Test)`)
- Directory: `tests/` with own `CMakeLists.txt`. Files: `tests/test_<module>.cpp`
- Run: `ctest` from `build/`
- All new code requires tests. Bug fixes require regression tests.

## Roadmap

See `ROADMAP.md` for full refactoring plan, priority order, known issues, and verification checklist.

## Platform Notes

- **macOS**: `libedit` (readline-compatible). `macdeployqt` bundles frameworks.
- **Windows**: `windeployqt` deploys DLLs. MinGW from `Qt/Tools/mingwXXX_64/`.
