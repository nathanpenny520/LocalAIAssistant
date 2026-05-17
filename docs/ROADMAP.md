# ROADMAP

## Current State (2026-05-15)

| Metric | Count |
|---|---|
| Total source files (.cpp/.h) | 75 |
| Total source lines | ~19,324 |
| Prompt files (.md) | 7 files, ~1,200 lines |
| Shell scripts | 5 files, ~2,645 lines |
| CMakeLists.txt | 902 lines |
| Test files | 8 files, ~1,780 lines |
| **Files over 500 lines** | **9 files** |
| Build targets | 6 libraries + 2 executables |

### Files Over 500 Lines

| File | Lines | Concern Count |
|---|---|---|
| `src/girlfriend/girlfriendwindow.cpp` | 1864 | 4 (UI, network, voice, sessions) |
| `src/ui/mainwindow.cpp` | 1746 | 5 (UI, sessions, network, search, agents) |
| `src/cli/cli_application.cpp` | 1318 | 4 (init, commands, tasks, network) |
| `src/girlfriend/voicemanager.cpp` | 1285 | 1 (voice pipeline — ASR+TTS) |
| `src/ui/markdownrenderer.cpp` | 850 | 2 (markdown, highlighting) |
| `src/tasks/safetychecker.cpp` | 774 | 1 (safety validation) |
| `src/knowledge/vectordb.cpp` | 566 | 1 (vector store) |
| `src/tasks/commandexecutor.cpp` | 555 | 1 (command execution) |
| `src/knowledge/embedder.cpp` | 551 | 1 (text embedding) |

---

## Phase 1: Cleanup ✅ (completed 2026-05-15)

### 1.1 Delete `src/core/soul.md` ✅

- **Why:** Zero references in any C++ code. `PromptManager` loads from `prompts/<lang>/*.md` and has its own hardcoded fallbacks. CMakeLists.txt copied it with "legacy" comments.
- **Done:** Deleted file. Removed 5 CMakeLists.txt copy/install references (Windows post-build, Linux post-build, macOS install, Windows install, Linux install).

### 1.2 Delete `src/girlfriend/personality.md` ✅

- **Why:** Orphaned. `personalityengine.cpp:19` calls `PromptManager::instance()->girlfriendPrompt()` which loads from `prompts/<lang>/girlfriend.md`. `personality.md` was copied into the app bundle but never read by any code path.
- **Done:** Deleted file. Removed CMake copy commands from macOS post-build, Windows post-build, Linux post-build, Windows install, Linux install.

### 1.3 Rename `src/girlfriend/memory.md` → `src/girlfriend/girlfriend_memory.md` ✅

- **Why:** Clarify it's girlfriend-specific, not general app memory.
- **Done:** Renamed file. Updated 7 path strings in `memorymanager.cpp`. Updated CMake copy commands on all 3 platforms + install blocks.

### 1.4 Simplify `CLAUDE.md` ✅

- **Why:** Was 161 lines with redundant table, verbose descriptions, procedural notes.
- **Done:** Simplified to ~75 lines. Removed HTML comments, duplicate table, verbose descriptions, Documentation Maintenance section. Kept all core info: build commands, git workflow, architecture diagram, code standards, testing, packaging, platform notes.

---

## Phase 2: File Splitting by Functional Cohesion

**Principle:** Split only when a file has multiple distinct concerns. Single-concern files stay together regardless of line count.

### 2.1 `src/girlfriend/girlfriendwindow.cpp` (1864 lines → 4 files)

4 distinct concerns separated by existing section comments:

| File | ~Lines | Content |
|---|---|---|
| `girlfriendwindow.cpp` | 550 | Constructor, setupUI, addMessageBubble, event handlers, input management, applyTheme, retranslateUi |
| `girlfriendwindow_network.cpp` | 500 | onSendClicked, onStreamChunkReceived, onStreamFinished, onNetworkError |
| `girlfriendwindow_voice.cpp` | 350 | onVoiceClicked, voice config dialog, ASR callbacks, speaking state, overlay labels |
| `girlfriendwindow_sessions.cpp` | 460 | Session switching, manage conversations, clear history, settings toggles, loadSessionMessages, clearChatUI |

All files stay in `src/girlfriend/`. Header unchanged.

### 2.2 `src/ui/mainwindow.cpp` (1746 lines → 5 files)

| File | ~Lines | Content |
|---|---|---|
| `mainwindow.cpp` | 450 | Constructor, setupUI, setupMenuBar, event handlers, input management |
| `mainwindow_sessions.cpp` | 450 | Session rendering, session list, CRUD operations, context menu, history panel |
| `mainwindow_network.cpp` | 420 | Send, stream chunk, stream finish, network error, retry logic |
| `mainwindow_search.cpp` | 250 | Search bar, search navigation, highlight management |
| `mainwindow_agents.cpp` | 380 | Task response handling, agent loop callbacks, file display, command output |

All files stay in `src/ui/`. Header unchanged.

### 2.3 `src/cli/cli_application.cpp` (1318 lines → 4 files)

| File | ~Lines | Content |
|---|---|---|
| `cli_application.cpp` | 420 | Constructor, run, interactive loop, readInput, printUsage |
| `cli_application_commands.cpp` | 500 | Command dispatch, help, session commands, file commands, config display |
| `cli_application_tasks.cpp` | 370 | Task plan extraction, plan preview, plan execution, agent loop callbacks |
| `cli_application_network.cpp` | 200 | Response/stream/error handlers, single query runner |

All files stay in `src/cli/`. Header unchanged.

### 2.4 `src/ui/markdownrenderer.cpp` (850 lines → 2 files)

Clear seam at line 372: markdown-to-HTML vs syntax highlighting.

| File | ~Lines | Content |
|---|---|---|
| `markdownrenderer.cpp` | 375 | toHtml, escapeHtml, headers, bold/italic, tables |
| `markdownrenderer_highlight.cpp` | 478 | highlightCode, C++/Python/JS/JSON/Bash/generic highlighters |

Both files stay in `src/ui/`. Header unchanged.

### 2.5 NOT Split — single concerns

| File | Lines | Reason |
|---|---|---|
| `voicemanager.cpp` | 1285 | One voice pipeline. ASR+TTS share WebSocket state. |
| `safetychecker.cpp` | 774 | One security layer. |
| `vectordb.cpp` | 566 | One vector store. |
| `commandexecutor.cpp` | 555 | One command dispatcher. |
| `embedder.cpp` | 551 | One embedding engine. |

---

## Phase 3: CMakeLists.txt & Tooling ✅ (completed 2026-05-15)

### 3.1 Deduplicate post-build copy commands ✅

macOS (lines 408-521), Windows (616-723), Linux (728-835) copy identical resource sets. Extract a `copy_app_resources(TARGET DEST_DIR)` CMake function. Reduces ~250 lines of duplication to a single function + 3 one-line calls.

**Done:** `copy_app_resources` function at CMakeLists.txt:371. macOS, Windows, and Linux each call it with one line. Saved ~131 net lines.

### 3.2 Integrate `shfmt` into workflow ✅

- `.shfmtrc` replaced with `.editorconfig` (shfmt reads it natively)
- `scripts/format.sh` created — runs `clang-format`, `cmake-format`, `shfmt` with `--check` mode
- `set -euo pipefail` added to `build.sh`, `setup.sh`, `cli-wrapper.sh` (all tested)
- All shell scripts formatted with `shfmt -i 4 -ci -bn`
- `version.sh` skipped (sourced by other scripts)

### 3.3 (Optional) CI format check ✅

Added `.github/workflows/format-check.yml` to enforce `clang-format`, `cmake-format`, and `shfmt` on PRs.

---

## Phase 4: Extension/Plugin Architecture

### Goal

Add new AI backends, tools, web search, and MCP servers without modifying/recompiling C++.

### Layer 1: Native C++ Plugins (Qt `QPluginLoader`)

- Interface: `IAssistantPlugin` (`src/core/iplugin.h`) — pluginId, pluginName, initialize, shutdown
- Sub-interfaces: `IProviderPlugin` (new AI backends), `IToolPlugin` (custom tools)
- `PluginManager` (`src/core/pluginmanager.cpp`): scans `<app_dir>/plugins/`, loads `.dylib`/`.so`/`.dll`
- Convert `OllamaProvider` to plugin form as POC

### Layer 2: JavaScript Plugins (Qt `QJSEngine`)

- Zero external dependencies — QJSEngine ships with Qt6::Qml
- Users write `.js` files in `plugins/` directory
- Expose safe C++ APIs: `httpGet`, `httpPost`, `readFile`, `writeFile`, `log`
- Hot-reload: watch files, reload on change
- Sandboxed execution

### Layer 3: MCP Integration (QProcess + stdio JSON-RPC)

- MCP servers run as separate processes (Node.js, Python, any language)
- Communication via stdin/stdout JSON-RPC 2.0
- Config: `mcp_servers.json` lists servers with command/args/env
- `McpClient` (`src/core/mcpclient.cpp`): spawn, communicate, auto-restart
- `McpClientManager`: manage multiple MCP servers
- Example: Brave Search MCP server for web search capability

### Implementation Order

| Step | Deliverable |
|---|---|
| 1 | `IAssistantPlugin` + `IProviderPlugin` interfaces |
| 2 | `PluginManager` with `QPluginLoader` |
| 3 | Convert one provider to plugin as POC |
| 4 | `JsPluginRuntime` with `QJSEngine` |
| 5 | `McpClient` + `McpClientManager` |
| 6 | `mcp_servers.json` config + TaskEngine integration |

---

## Phase 5: Later

- `src/girlfriend/girlfriend_translations.h` (360 lines, header-only) → split into `.h` (declarations) + `.cpp` (definitions). Low priority — compile cost of 60 trivial inline getters is negligible.

---

## Known Issues

| Issue | Status |
|---|---|
| `girlfriendwindow.cpp` 1864 lines | Phase 2.1 |
| `mainwindow.cpp` 1746 lines | Phase 2.2 |
| `cli_application.cpp` 1318 lines | Phase 2.3 |
| `markdownrenderer.cpp` 850 lines | Phase 2.4 |
| `soul.md` dead code copied by CMake | ✅ Phase 1.1 |
| `personality.md` orphaned — never read by code | ✅ Phase 1.2 |
| `memory.md` ambiguous name | ✅ Phase 1.3 |
| `CLAUDE.md` missing directories / verbose | ✅ Phase 1.4 |
| CMake copy duplication (~250 lines) | ✅ Phase 3.1 |
| `.shfmtrc` never invoked by tooling | ✅ Phase 3.2 |
| No plugin/extension architecture | Phase 4 |
| `girlfriend_translations.h` header-only (360 lines) | Phase 5 |
| `ROADMAP.md` referenced but didn't exist | Created 2026-05-15 |

---

## Verification Checklist

- [ ] `cmake -B build && cmake --build build --parallel 4` succeeds
- [ ] `ctest` — all 8 test suites pass
- [ ] GUI launches and basic chat works
- [ ] CLI launches and basic commands work
- [ ] Girlfriend module loads personality/memory correctly
- [ ] Voice service connects (if credentials configured)
- [ ] File parsing (PDF/DOCX/txt) works
- [ ] Knowledge base embedding/search works
