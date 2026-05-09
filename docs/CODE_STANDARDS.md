# LocalAIAssistant Code Standards

> Established during Tier 5 code quality standardization. All contributors must follow these standards.
> Formatting is enforced by tooling (clang-format, cmake-format, prettier, shfmt); remaining rules are
> enforced in code review.

---

## 1. File Organization

### Naming

| Category | Convention | Example |
|----------|-----------|---------|
| Source files | `snake_case.cpp/.h` | `file_parser.cpp`, `safety_checker.h` |
| Test files | `test_<module>.cpp` | `test_fileparser.cpp` |
| CMake modules | `snake_case.cmake` | `dependencies.cmake` |
| Shell scripts | `snake_case.sh` | `build.sh`, `setup.sh` |
| Markdown docs | `UPPER_SNAKE.md` | `ROADMAP.md`, `CODE_STANDARDS.md` |

### Size Limits

- **Files**: ≤ 500 lines (current violations tracked in ROADMAP.md)
- **Functions**: ≤ 50 lines — extract private helpers aggressively
- One class / one concern per file. Qt signals/slots for cross-object communication.

### Directory Structure

```
src/
├── core/          # NetworkManager, SessionManager, FileManager, datamodels
├── parsers/       # FileParser (text/PDF/DOCX/image extraction)
├── prompts/       # PromptManager + en/ and zh_CN/ prompt templates
├── tasks/         # TaskEngine, CommandExecutor, SafetyChecker, OperationUndo
├── knowledge/     # KnowledgeBase, Embedder, VectorDB, TextChunker, DocImporter
├── girlfriend/    # GirlfriendWindow, AvatarWidget, VoiceManager, PersonalityEngine
├── ui/            # MainWindow, SettingsDialog, StyleSheetManager, AppTheme
└── cli/           # CliApplication, cli_main
```

Each directory maps to a CMake library target. Cross-module includes use relative paths from `src/`.

---

## 2. Code Style

### Indentation & Spacing

- **4 spaces** per indent level. No tabs.
- Column limit: **100 characters**
- Trailing whitespace: stripped
- One blank line between logical sections; max 1 consecutive blank line

### Braces (K&R Variant)

Opening brace on the same line; closing brace on its own line. Always use braces, even for
single-statement bodies.

```cpp
// Correct
if (condition) {
    doSomething();
} else {
    doOther();
}

// Correct — class/function braces on same line
class FileParser {
public:
    static QString extractText(const QString& path);
};

void Foo::bar() {
    // ...
}

// Wrong — no brace omission
if (condition)
    doSomething();
```

### Naming

| Category | Convention | Example |
|----------|-----------|---------|
| Types (class/struct/enum) | `PascalCase` | `CommandExecutor`, `ApiType` |
| Functions / methods | `PascalCase` | `extractText()`, `sendChatRequest()` |
| Variables | `snake_case` | `file_count`, `total_size` |
| Member variables | `m_` prefix | `m_network`, `m_streamBuffer` |
| Constants | `kPascalCase` or `ALL_CAPS` | `kTextExtensions`, `DEFAULT_TIMEOUT` |
| Static globals | `kPascalCase` | `kImageExtensions` |
| Macros / defines | `ALL_CAPS` | `NO_PDF_SUPPORT`, `HAS_LIBZIP` |

### References and Pointers

- `&` and `*` bind to the **type**, not the variable name.

```cpp
// Correct
void foo(const QString& path);     // reference to QString
int* ptr;                          // pointer to int

// Wrong
void foo(const QString &path);     // space before &
int *ptr;                          // space before *
```

### Include Order

Related header → C standard → C++ standard → Qt → third-party libs → project headers.

```cpp
// fileparser.cpp
#include "fileparser.h"        // 1. Related header

#include <cstring>             // 2. C standard

#include <string>              // 3. C++ standard
#include <vector>

#include <QFile>               // 4. Qt
#include <QString>

#include <poppler-document.h>  // 5. Third-party (poppler, zip, pugixml, onnx, hnswlib)

#include "datamodels.h"        // 6. Project headers
#include "networkmanager.h"
```

_Note: clang-format automatically enforces this order via `IncludeBlocks: Regroup`._

### Header Guard

Every header uses both `#pragma once` and a traditional `#ifndef` guard, followed by a Doxygen
`@file` block.

```cpp
/**
 * @file fileparser.h
 * @brief Unified text/PDF/DOCX/image parsing with auto-detection and plain text extraction.
 */
#pragma once

#ifndef FILEPARSER_H
#define FILEPARSER_H

// ... declarations ...

#endif  // FILEPARSER_H
```

### Qt-Specific

- Qt macros (`Q_OBJECT`, `Q_SIGNALS`, `Q_SLOTS`, `Q_EMIT`, `Q_UNUSED`, etc.) must stand on their own
  line with no trailing semicolon
- Signals use `signals:` keyword (Qt 5/6 compatible); `Q_SIGNALS` is reserved for macOS keyword clash
- `tr()` for all user-facing strings; `QStringLiteral()` for immutable internal strings
- `QObject`-derived classes pass `QObject* parent = nullptr` in constructors

```cpp
class MyWidget : public QWidget {
    Q_OBJECT

public:
    explicit MyWidget(QWidget* parent = nullptr);

signals:
    void dataReady(const QString& data);
};
```

### Constructor Initializer Lists

One member per line, comma before the member, indented 8 spaces from the colon.

```cpp
NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
    , m_provider(nullptr)
    , m_apiType(ApiType::OpenAI)
    , m_isStreaming(false) {
    // body
}
```

---

## 3. Comments

### Golden Rule

**Explain WHY, not WHAT.** The code already tells us what it does through well-named identifiers.
Comments should explain the non-obvious: edge cases, design trade-offs, algorithm rationale.

```cpp
// BAD — restates what the code says
// Set the width to 50 pixels
label->setFixedWidth(50);

// GOOD — explains why
// Fixed width avoids percentage rendering bugs in QTextBrowser
label->setFixedWidth(50);
```

### Language

- **English only** for all comments, identifiers, and documentation
- Chinese text is reserved for user-facing `tr()` strings in the girlfriend module and prompt
  templates only

### Doxygen Headers

Every **public API header** (header included by another module) must have a `@file` / `@brief` block
immediately after the header guard:

```cpp
/**
 * @file networkmanager.h
 * @brief Thin facade over ApiProvider subclasses for LLM API calls.
 */
```

Method-level Doxygen uses `///` triple-slash:

```cpp
/// Parse TASK_PLAN JSON from an AI response and validate against the safety checker.
OperationPlan parsePlanFromAIResponse(const QString& aiResponse) const;
```

### Comment Tags

Use standardized tags for technical debt tracking:

| Tag | Purpose |
|-----|---------|
| `// TODO(name):` | Planned improvement, not urgent |
| `// FIXME:` | Known bug or broken behavior |
| `// HACK:` | Temporary workaround, needs replacement |
| `// NOTE:` | Important observation that might surprise a reader |

### Redundant Comments

Delete comments that merely restate what the identifier already says. Examples of comments that
should be **removed**:

```cpp
// BAD — delete these
// Set the file path
void setFilePath(const QString& path);
// Returns the name
QString name() const;
// Create the button
auto* button = new QPushButton(this);
```

---

## 4. Formatting Tools

All project formatting is enforced by tooling. Run the appropriate formatter before every commit.

### C++ — clang-format

```bash
clang-format -i <file>
```

Config: `.clang-format` (project root) — Google style base, 4-space indent, Qt macro support, K&R
braces.

### CMake — cmake-format

```bash
cmake-format -i CMakeLists.txt
```

Config: `.cmake-format.json` — uppercase keywords, canonical command casing, 4-space indent.

### Shell scripts — shfmt

```bash
shfmt -i 4 -ci -bn -w scripts/*.sh
```

Config: `.shfmtrc` — 4-space indent, binary next-line, keep column alignment.

### Markdown / YAML — prettier

```bash
prettier --write "*.md" ".github/workflows/*.yml"
```

Config: `.prettierrc` + `.prettierignore` — 100-char width, 4-space tab, prose wrapping, excludes
`build/` and `resources/models/`.

### Installation (macOS)

```bash
brew install clang-format shfmt
brew install pipx && pipx install --include-deps cmake-format
npm install -g prettier
```

---

## 5. Git Conventions

### Commit Format

```
type(scope): brief description
```

| Type | Usage |
|------|-------|
| `feat` | New feature |
| `fix` | Bug fix |
| `refactor` | Code restructuring (no behavior change) |
| `docs` | Documentation or comments |
| `test` | Adding or updating tests |
| `chore` | Tooling, config, build scripts |
| `style` | Whitespace, formatting (no logic change) |

### Workflow

1. **Commit after every change.** Never accumulate unrelated edits in a single commit.
2. Make changes → run formatter → `cmake --build build --parallel 4` → `ctest --test-dir build` →
   commit
3. Push after verifying clean build and all tests pass
4. Never amend pushed commits; use new commits for fixes

### Rollback

```bash
git reset --soft HEAD~1   # Keep changes in working tree
git reset --hard HEAD~1   # Discard changes (CAUTION)
```

---

## 6. Testing Standards

### Framework

Qt Test (`find_package(Qt6 REQUIRED COMPONENTS Test)`). Tests live in `tests/` with their own
`CMakeLists.txt`.

### File Naming

```
tests/test_<module>.cpp
```

### Running Tests

```bash
ctest --test-dir build
```

### Requirements

- **All new code** requires tests
- **Bug fixes** require regression tests that fail before the fix and pass after
- Test functions should be self-contained and not depend on execution order
- Use `QVERIFY`, `QCOMPARE`, `QVERIFY_EXCEPTION_THROWN` macros

### Current Test Files

```
tests/test_fileparser.cpp
tests/test_safetychecker.cpp
tests/test_commandexecutor.cpp
tests/test_sessionmanager.cpp
tests/test_apptheme.cpp
tests/test_stylesheetmanager.cpp
tests/test_markdownrenderer.cpp
```

---

## 7. Project Architecture Reference

```
LocalAIAssistantCore (src/core/, src/prompts/)   — network, sessions, file I/O, prompts
    ↑
    ├── TaskModule      (src/tasks/)             — file ops, safety checker, undo, command exec
    ├── KnowledgeModule (src/knowledge/)         — chunking, embedding, vector DB, doc import
    └── GirlfriendModule (src/girlfriend/)        — AI girlfriend: avatar, voice, memory, personality
            ↑
            └── LocalAIAssistant (GUI, src/ui/main.cpp → MainWindow)
```

- **CLI**: links Core + TaskModule only (no Girlfriend, no Knowledge)
- **GUI**: links GirlfriendModule → transitively pulls in all modules
- **AppTheme**: `src/ui/apptheme.h` — ~40 semantic color tokens, single source of truth for UI colors
- **Settings**: `QSettings("LocalAIAssistant", "Settings")`

---

## 8. Pre-Commit Checklist

Before every commit, verify:

- [ ] `clang-format -i` on all changed `.cpp`/`.h` files
- [ ] `cmake-format -i CMakeLists.txt` if CMakeLists.txt changed
- [ ] `shfmt -w` on changed shell scripts
- [ ] `prettier --write` on changed `.md`/`.yml` files
- [ ] `cmake --build build --parallel 4` — clean build
- [ ] `ctest --test-dir build` — all tests pass
- [ ] Commit message follows `type(scope): description` format
