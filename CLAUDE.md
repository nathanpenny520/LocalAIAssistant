# CLAUDE.md

<!-- PROJECT_ROOT: /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant -->
<!-- NOTE: Bash tool may start in parent directory. Always use git -C PROJECT_ROOT for git commands. -->

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Full build (all targets, release)
./scripts/build.sh

# Build specific target only
./scripts/build.sh LocalAIAssistant        # GUI only
./scripts/build.sh LocalAIAssistant-CLI    # CLI only

# Clean debug build (non-interactive)
./scripts/build.sh -c -d --no-run

# Build and create platform package (DMG/zip/tar.gz)
./scripts/build.sh build -p

# Run without rebuilding (use --gui or --cli to avoid interactive prompt)
./scripts/build.sh run --gui
./scripts/build.sh run --cli

# Direct cmake build (parallel, no interactive prompts)
cmake --build build --parallel 4

# First-time setup after clone
./scripts/setup.sh
```

**Note**: `build.sh` has an interactive prompt at line 870 asking "[G]ui / [C]li". Use `--gui` or `--cli` flags, or `--no-run` to skip the prompt. For background/CI builds, use direct cmake commands.

## Git Commit Principles (Mandatory)

**Core Principle**: Commit immediately after each change to prevent irreversible mistakes. Small commits are easier to review, test, and rollback.

### Commit Workflow

**1. Before Committing**
```bash
# Verify changes compile and work
cmake --build build --parallel 4
./scripts/build.sh run --gui  # or --cli

# Check what changed
git status
git diff
```

**2. Commit Immediately After Each Change**
- One feature/fix/refactor = ONE commit
- Never accumulate multiple unrelated changes
- Commit even if change is small (10-50 lines)
- Better to have many small commits than one large commit

**3. Commit Message Format**
```
<type>(<scope>): <brief description>

<body - explain what changed and why>

<footer - reference issues/breaking changes>
```

**Types**:
- `feat`: New feature
- `fix`: Bug fix
- `refactor`: Code restructuring (no behavior change)
- `docs`: Documentation updates
- `test`: Adding tests
- `chore`: Build/config changes

**Example**:
```
refactor(core): Extract FileParser to remove duplicate code

- Create src/parsers/fileparser.h/cpp (400 lines)
- Update filemanager.cpp to delegate parsing
- Update docimporter.cpp to delegate parsing
- Remove ~100 lines of duplicate PDF parsing logic

Verified: cmake --build succeeds, all tests pass.
```

**4. Local Commit First, Push Later**
```bash
# Commit locally (can rollback)
git add <files>
git commit -m "..."

# Test the commit
cmake --build build --parallel 4
./scripts/build.sh run --gui

# Only push after confirming no issues
git push origin main
```

**5. Rollback Strategy**
```bash
# View recent commits
git log --oneline -10

# Rollback last commit (keep changes in working directory)
git reset --soft HEAD~1

# Rollback last commit (discard changes permanently)
git reset --hard HEAD~1

# Rollback to specific commit
git reset --hard <commit-hash>

# View what changed in a commit
git show <commit-hash>
git diff <commit-hash>~1 <commit-hash>
```

### Commit Rules

✅ **DO**:
- Commit after every successful change
- Write clear commit messages explaining WHY
- Test before committing
- Keep commits small and focused
- Commit locally first for safety

❌ **DON'T**:
- Accumulate multiple changes in one commit
- Skip testing before commit
- Push without local verification
- Commit half-finished work
- Make large refactoring commits without tests

### Example Scenarios

**Scenario 1: Safe Refactoring**
```bash
# Step 1: Make change
# (extract FileParser)

# Step 2: Test
cmake --build build --parallel 4
ctest

# Step 3: Commit
git add src/parsers/fileparser.h src/parsers/fileparser.cpp
git add src/core/filemanager.cpp src/knowledge/docimporter.cpp
git commit -m "refactor(parsers): Extract FileParser module"

# Step 4: Verify commit
git show HEAD
cmake --build build --parallel 4

# Step 5: Push (optional, later)
git push origin main
```

**Scenario 2: Bug Found After Commit**
```bash
# Bug discovered after commit but before push
git log --oneline -5
git show HEAD  # Review what changed

# Option A: Fix with new commit
# (make fix, test, commit)

# Option B: Rollback if severe
git reset --soft HEAD~1  # Keep changes, fix them
# (fix issues)
git commit -m "fix(core): Correct FileParser implementation"

# If already pushed, create fix commit
git commit -m "fix(core): Fix FileParser PDF handling"
git push origin main
```

**Why This Matters**: 
- Phase 1.1 (girlfriendwindow split) failed after 1 hour of work without commits
- Had to rollback using `git reset --hard HEAD~1` (lost all work)
- If we committed after each file, we could identify which file broke functionality
- Small commits = easy debugging + easy rollback

## Architecture

The project is a **Qt 6 C++17 desktop app** with GUI and CLI dual-mode. It is split into four CMake libraries and two executables, linked in a dependency chain:

```
LocalAIAssistantCore  (src/core/, src/prompts/)   — network, sessions, file handling, prompts
    ↑
    ├── TaskModule     (src/tasks/)               — file ops, safety checker, undo, command execution
    ├── KnowledgeModule (src/knowledge/)           — text chunking, embedding, vector DB, doc import
    └── GirlfriendModule (src/girlfriend/)         — AI girlfriend: avatar, voice, memory, personality
            ↑
            └── LocalAIAssistant (GUI executable, src/ui/main.cpp → MainWindow)
```

- **LocalAIAssistantCore** — Owns `NetworkManager` (OpenAI/Ollama/LlamaCpp API), `SessionManager` (multi-session persistence via JSON), `FileManager`, and `PromptManager` (loads markdown prompts from `src/prompts/` by locale).
- **GirlfriendModule** — AI girlfriend feature (avatar, voice, memory, personality). **Architecture (after Phase 1.1 split):**
  - `GirlfriendWindow` — main window shell, coordinates split components
  - `GirlfriendChatView` — chat message display, streaming
  - `GirlfriendInputPanel` — text/voice input handling
  - `GirlfriendSidebarManager` — settings menu, overlay labels
  - `GirlfriendMediator` — signal wiring, network callbacks
  - Depends on LocalAIAssistantCore, Qt::Multimedia, Qt::WebSockets
- **CLI executable** (`src/cli/`) links to LocalAIAssistantCore, TaskModule — it does NOT link GirlfriendModule or KnowledgeModule.
- **GUI executable** (`src/ui/main.cpp`) links GirlfriendModule (which transitively pulls in Core).

### API Abstraction

`NetworkManager` (`src/core/networkmanager.h`) supports three backends via the `ApiType` enum: `OpenAI` (v1/chat/completions, used by llama.cpp/vLLM too), `Ollama` (/api/chat), and `LlamaCpp` (OpenAI-compatible). Settings are stored in `QSettings("LocalAIAssistant", "Settings")`.

### Optional Features (compile-time)

| Feature | Required lib | CMake define |
|---------|-------------|--------------|
| PDF import | Poppler (pkg-config `poppler-cpp`) | None (auto-detected) |
| DOCX import | libzip + pugixml | `LIBZIP_AVAILABLE` |
| Real embeddings | ONNX Runtime | None (falls back to placeholder vectors) |

When Poppler is absent, `NO_PDF_SUPPORT` is defined. The embedder (`src/knowledge/embedder.cpp`) returns zero vectors when ONNX Runtime is unavailable.

### Data Models

Core types in `src/core/datamodels.h`: `FileAttachment`, `ChatMessage` (role + content + attachments), `ChatSession` (UUID id, title, message vector, pinned flag). Sessions are persisted to JSON files by `SessionManager`.

### Translations

Qt Linguist `.ts` files in `translations/`. The build runs `lrelease` to produce `.qm` files copied to `build/translations/`. `TranslationManager` loads them at startup based on `QSettings("language")`.

### .env Configuration

`.env` (gitignored) holds iFLYTEK voice credentials for ASR/TTS. Template at `.env.example`. The file is loaded at runtime by the voice manager, not at compile time. macOS packaging strips `.env` from the app bundle.

## C++ Commenting Conventions

### Comment Types

- **Explanatory comments** — `//` (single-line) or `/* */` (multi-line). For logic details inside `.cpp` files.
- **Documentation comments** — Doxygen style: `/** ... */` or `///`. Used in headers for public APIs to enable auto-generated docs.

### Commenting by Level

**File header** — Every source file starts with `@file`, `@brief`, `@author`, `@date`, `@copyright`.

**Class / interface** — In `.h` files, document the class purpose, thread-safety notes, and all public methods with `@brief`, `@param`, `@return`, `@throw`, `@note`.

**Implementation details** — In `.cpp` files, comment above complex logic blocks only. **Explain why**, not what. Don't comment the obvious (e.g., `i++; // increment i`).

### Best Practices

- **Language** — Use English consistently. Never mix languages.
- **Placement** — Place comments on the line above the code, not trailing inline (except for very short declarations).
- **Tags** — Use standard markers:
  - `// TODO: [name] <description>`
  - `// FIXME: <bug description>`
  - `// HACK: <temporary workaround>`
- **Keep comments in sync** with code changes. An outdated comment is worse than no comment.

### Self-Documenting Code

Prefer clear naming over comments. Replace magic-number conditions with well-named variables:

```cpp
// Bad
if (a > 18 && s > 1000) { ... }

// Good
bool isEligibleForReward = (age > 18 && score > 1000);
if (isEligibleForReward) { ... }
```

### When to Write Comments

1. **Complex algorithms** — Describe the approach, time complexity, or reference a paper/link.
2. **Non-obvious parameters** — Explain the meaning of magic numbers.
3. **Gotchas** — Explain why a seemingly simpler alternative won't work.
4. **Public API** — All public headers must have thorough Doxygen comments.

**Guiding principle: concise, accurate, consistent.** Comments help future readers (including yourself) build a mental model quickly — they don't translate code into prose.

## Code Standards (mandatory)

### File Size & Responsibility

- **Every source file must be ≤ 500 lines.** Files exceeding 500 lines must be split by concern (see ROADMAP.md for violations).
- **Single Responsibility Principle (SRP)** — each file and each function does exactly one thing. A function name answers "what does this do?" with a single verb.
- **No function over ~50 lines.** Extract private helpers for any subtask longer than a few lines.
- Public methods form the API surface; private methods are implementation details. Use Qt signals/slots for cross-object communication, not direct coupling.

### C++ Style (Google C++ base + Qt adaptations)

- `.clang-format` at repo root enforces the style automatically.
- File names: `snake_case.cpp` / `snake_case.h`
- Type names: `PascalCase` (classes, structs, enums, typedefs)
- Variable names: `snake_case`; member variables prefixed with `m_`
- Function names: `PascalCase` (`camelCase` for private helpers if preferred, but be consistent per-file)
- Constants: `kPascalCase` or `ALL_CAPS`
- Include order: related header → C system → C++ standard → Qt → other libraries → project headers
- Run `clang-format -i <file>` before committing.

### CMake Style

- `.cmake-format.json` at repo root. Run `cmake-format -i CMakeLists.txt` before committing.
- Lowercase command names: `add_library`, `target_link_libraries`
- Two-space indent. Comment blocks use `#` with a space.

### Comments

- **Language**: English only. All Chinese comments must be converted.
- **Header files**: Doxygen style (`@brief`, `@param`, `@return`) on all public API.
- **Implementation files**: Comment WHY, not WHAT. No obvious comments.
- **Tags**: `// TODO(name):`, `// FIXME:`, `// HACK:`

## Testing

- Test framework: Qt Test (`find_package(Qt6 REQUIRED COMPONENTS Test)`)
- Test directory: `tests/` with its own `CMakeLists.txt`
- Run tests: `ctest` from `build/`, or `./scripts/build.sh test`
- All new code requires tests. Bug fixes require a regression test.
- Test file naming: `tests/test_<module>.cpp`

## Task Roadmap

See `ROADMAP.md` for the full refactoring plan, known issues, and verification checklist.

### Platform Notes

- macOS: `libedit` is built-in (readline-compatible for CLI). `macdeployqt` bundles Qt frameworks into `.app` for distribution.
- Windows: `windeployqt` deploys DLLs automatically after build. MinGW compiler is detected from `Qt/Tools/mingwXXX_64/`.


