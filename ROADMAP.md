# ROADMAP — Task List & Known Issues

## Session Context (carry-over to new session)

Three parallel explore agents audited the entire 17,500-line codebase. This document captures all findings and the refactoring-first plan. **Phase 1.1 deferred** due to high risk. Start with Phase 0 (Testing) or Phase 1.5 (FileParser).

### What was completed this session
- [x] Full codebase audit (3 explore agents, all 12 user-reported issues validated)
- [x] CLAUDE.md updated with: SRP + 500-line rules, code standards (Google C++, cmake-format), testing instructions, ROADMAP pointer
- [x] ROADMAP.md created (this file)
- [x] **Phase 1.1 attempted then rolled back** — splitting girlfriendwindow.cpp caused feature loss, deferred until tests exist

### What to do next (优先级排序)
**按照风险（低→高）和重要性（高→低）重新规划**

---

## 优先级执行顺序（Risk: Low→High, Importance: High→Low）

### **Tier 0: 测试基础设施（最高优先级）**
**风险**: 无风险（纯新增）  
**重要性**: 极高 — 所有后续重构的保障

#### 0.1 添加基础测试框架
- [ ] Add `enable_testing()` and `find_package(Qt6 Test)` to CMakeLists.txt
- [ ] Create `tests/CMakeLists.txt`
- [ ] Create `.github/workflows/build.yml` — CI for macOS, Windows, Linux

#### 0.2 添加关键模块单元测试
- [ ] Write `tests/test_fileparser.cpp` (PDF, DOCX, text, image parsing)
- [ ] Write `tests/test_safetychecker.cpp` (command safety validation)
- [ ] Write `tests/test_commandexecutor.cpp` (command execution, cross-platform)
- [ ] Write `tests/test_sessionmanager.cpp` (session persistence, JSON serialization)
- [ ] Add `ctest` to `scripts/build.sh`

**收益**: 有了测试后，Phase 1.1-1.4 的 UI 重构风险大幅降低

---

### **Tier 1: 低风险 + 高价值重构**

#### 1.1 Extract FileParser shared module 🟢 **DONE**
**风险**: 极低（纯函数，无状态，无 UI 依赖）  
**重要性**: 高（立即消除重复代码）  
**位置**: Duplicate code in `src/core/filemanager.cpp:174-213` and `src/knowledge/docimporter.cpp:143-189`

- [x] Create `src/parsers/fileparser.h/cpp` — unified text/PDF/DOCX/image parsing (266 lines)
- [x] Update `filemanager.cpp` — delegate to FileParser (257 → 132 lines)
- [x] Update `docimporter.cpp` — delegate to FileParser (271 → 80 lines)
- [x] Remove duplicate PDF extraction code (~100 lines eliminated)
- [x] Add `FileParser` library to CMakeLists.txt

**收益**: 
- 消除 ~100 行重复代码
- 易于测试（Phase 0.2 的 test_fileparser 可验证）
- 无破坏风险

#### 1.2 NetworkManager → Provider Pattern 🟢 **DONE**
**风险**: 低（已有清晰边界，无 UI 依赖）  
**重要性**: 中高（扩展性提升，代码更清晰）

- [x] `src/core/apiprovider.h/cpp` — abstract base with shared network logic (97 + 270 lines)
- [x] `src/core/openai_provider.cpp/h` — OpenAI /v1/chat/completions (25 + 141 lines)
- [x] `src/core/ollama_provider.cpp/h` — Ollama /api/chat (24 + 121 lines)
- [x] `src/core/llamacpp_provider.cpp/h` — LlamaCpp extends OpenAIProvider (18 + 7 lines)
- [x] Update `networkmanager.cpp/h` — thin facade, delegates to active provider (72 + 240 lines)
- [x] Add `<QSettings>` include to `mainwindow.cpp` (was transitively included via old networkmanager.h)

**收益**:
- 易于添加新 provider（如 Anthropic）
- 清晰的 API 边界
- 易于单元测试每个 provider

---

### **Tier 2: 平台修复（解决功能缺陷）**

#### 2.1 Windows command execution 🔴 **阻塞问题**
**风险**: 中等（影响核心功能，需仔细测试）  
**重要性**: 高（Windows 用户无法使用）

- [ ] Replace Unix shell commands in `src/tasks/taskengine.cpp:129-152` with `QDir`/`QFile` Qt APIs
- [ ] Add PowerShell fallback in `src/tasks/commandexecutor.cpp`
- [ ] Add Windows dangerous command patterns to `src/tasks/safetychecker.cpp`

**收益**: Windows 用户可正常使用任务执行功能

#### 2.2 UI theme fixes 🟡 **可访问性问题**
**风险**: 低（只修改颜色值）  
**重要性**: 高（light theme 文字不可见）

- [ ] Fix disabled button text contrast in light theme (`src/ui/stylesheetmanager.cpp:168`, `#e0e0e0` → higher contrast)
- [ ] Replace hardcoded `#ff9500` with theme-aware colors in `src/ui/settingsdialog.cpp:133`
- [ ] Fix hardcoded `#ff9500` in `src/ui/operationconfirmdialog.cpp:72`

**收益**: Light theme 可正常使用

#### 2.3 Windows resource copying 🟢 **功能缺失**
**风险**: 低（添加缺失的资源复制）  
**重要性**: 中（功能完整性）

- [ ] Add `docs/` copy to CMakeLists.txt WIN32 block (mirror macOS lines 391-400)
- [ ] Add `AIGirlfriend` image copy for CLI on Windows

**收益**: Windows 帮助文档可正常显示

---

### **Tier 3: 中等风险重构（等测试完成后）**

#### 3.1 Split embedder.cpp & vectordb.cpp 🟡
**风险**: 中低（相对独立模块）  
**重要性**: 中（代码组织优化）

- [ ] `src/knowledge/embedder.cpp` (591) → `embedder.cpp/h` (orchestrator, ~300) + `embedder_onnx.cpp/h` (ONNX backend, ~280)
- [ ] `src/knowledge/vectordb.cpp` (555) → `vectordb.cpp/h` (API, ~280) + `vectordb_hnsw.cpp/h` (HNSW backend, ~270)

**前提**: Phase 0 测试完成

#### 3.2 Split stylesheetmanager.cpp 🟡
**风险**: 中（UI 相关）  
**重要性**: 低（可选优化）

- [ ] `src/ui/stylesheetmanager.cpp` (575) → `stylesheetmanager.cpp/h` (manager, ~290) + `theme_dark.cpp/h` (~280) + `theme_light.cpp/h` (~280)

**前提**: Phase 0 测试完成 + Phase 2.2 theme fixes 完成

#### 3.3 Fix build.sh interactive prompt 🟡
**风险**: 中（影响构建流程）  
**重要性**: 中（自动化构建）

- [ ] Fix interactive "G/C" prompt (line 870-881) in `scripts/build.sh`:
  - Add timeout or default to GUI when stdin unavailable
  - Or use `--gui/--cli` flag explicitly
  - Document workaround in CLAUDE.md (already done)

**前提**: 可独立进行，不阻塞其他工作

---

### **Tier 4: 高风险重构（推迟到有测试后）**

#### 4.1 Split girlfriendwindow.cpp ⚠️ **DEFERRED**
**风险**: 高（42 个紧密耦合函数，复杂 Qt 信号槽，零测试）  
**重要性**: 中（代码组织，不阻塞功能）  
**状态**: 已尝试并回滚，导致功能丢失（头像等级切换、会话管理）

- [ ] **推迟**直到：
  - Phase 0 测试框架完成
  - GirlfriendModule 有完整功能测试
  - 或需要添加新功能时再拆分

**文件**: `src/girlfriend/girlfriendwindow.cpp` (1,937 lines)

#### 4.2 Split mainwindow.cpp ⚠️ **DEFERRED**
**风险**: 高（God Object，直接依赖 girlfriendwindow/taskengine/knowledgebase）  
**重要性**: 中（代码组织）

- [ ] **推迟**直到 Phase 0 测试完成
- [ ] `mainwindow.cpp/h` — orchestrator: menu, layout, top-level signal wiring (~400 lines)
- [ ] `chatwidget.cpp/h` — chat display + streaming + input area (~450 lines)
- [ ] `sessionpanel.cpp/h` — session list sidebar + CRUD (~400 lines)
- [ ] `chatcontroller.cpp/h` — mediates ChatWidget ↔ SessionManager ↔ NetworkManager (~400 lines)

**文件**: `src/ui/mainwindow.cpp` (1,769 lines)

#### 4.3 Split voicemanager.cpp ⚠️ **DEFERRED**
**风险**: 中高（有状态管理，WebSocket 连接）  
**重要性**: 低（可选优化）

- [ ] **推迟**直到 Phase 0 测试完成
- [ ] `voicemanager.cpp/h` — facade, state machine, public API (~350 lines)
- [ ] `voice_asr.cpp/h` — speech-to-text, iFlytek WebSocket, audio capture (~400 lines)
- [ ] `voice_tts.cpp/h` — text-to-speech, iFlytek WebSocket, audio playback (~400 lines)
- [ ] `voice_config.cpp/h` — credential loading, platform paths (~300 lines)

**文件**: `src/girlfriend/voicemanager.cpp` (1,505 lines)

#### 4.4 Split cli_application.cpp ⚠️ **DEFERRED**
**风险**: 中低（CLI 相对独立）  
**重要性**: 低（可选优化）

- [ ] **推迟**直到需要添加 CLI 新功能时
- [ ] `cli_application.cpp/h` — main loop, command dispatch (~400 lines)
- [ ] `cli_renderer.cpp/h` — output formatting, markdown, colors (~380 lines)
- [ ] `cli_input.cpp/h` — readline/libedit, history, completion (~350 lines)

**文件**: `src/cli/cli_application.cpp` (1,165 lines)

---

### **Tier 5: 代码质量标准（渐进式）**

#### 5.1 Code formatting
**风险**: 无（只影响格式）  
**重要性**: 中（长期维护）

- [ ] Add `.clang-format` (Google style base, adapted for Qt SIGNAL/SLOT macros)
- [ ] Add `.cmake-format.json` for CMake style
- [ ] Run clang-format on all files progressively

#### 5.2 Comment cleanup
**风险**: 无（不影响功能）  
**重要性**: 低（代码清晰度）

- [ ] Convert all Chinese comments to professional English
- [ ] Remove redundant/obvious comments (keep WHY, delete WHAT)
- [ ] Add Doxygen headers (`@file`, `@brief`) to all public API headers

---

### **Tier 6: UI/UX 优化（可选）**

#### 6.1 UI improvements
**风险**: 低（不破坏功能）  
**重要性**: 低（用户体验优化）

- [ ] Fix input height: remove `setFixedHeight()`, enable scrollbar, raise cap to 300px (files: `mainwindow.cpp:182-183, 1722`)
- [ ] Fix thinking collapse: replace `<details>` HTML with custom QWidget toggle (file: `mainwindow.cpp:119-127`)
- [ ] Fix chat history: incremental rendering, remove garbled `৻` regex delimiter (file: `mainwindow.cpp:33-59`)
- [ ] Add session size limits and auto-truncation (file: `sessionmanager.cpp`)
- [ ] Fix help docs search path: remove duplicate path (file: `settingsdialog.cpp:348-349`)

---

### **Tier 7: 构建系统拆分（可选）**

#### 7.1 Split CMakeLists.txt
**风险**: 中等（影响构建流程）  
**重要性**: 低（代码组织）

- [ ] `CMakeLists.txt` (763) → main (~250) + `cmake/dependencies.cmake` (~250) + `cmake/platform.cmake` (~250)

**前提**: Phase 3.3 build.sh fix 完成，构建稳定后

#### 7.2 Split build.sh
**风险**: 中等（影响构建流程）  
**重要性**: 低（代码组织）

- [ ] `scripts/build.sh` (1278) → `scripts/build.sh` (main, ~430) + `scripts/build_impl.sh` (cmake+compile, ~430) + `scripts/package.sh` (packaging, ~400)

**前提**: Phase 3.3 build.sh fix 完成

#### 7.3 Trim setup.sh
**风险**: 低  
**重要性**: 低

- [ ] `scripts/setup.sh` (519) → trim to ≤500 lines

---

### **Tier 8: 分发打包（最后）**

#### 8.1 Packaging & Distribution
**风险**: 低（不影响代码）  
**重要性**: 低（分发便利性）

- [ ] Add NSIS/InnoSetup installer for Windows
- [ ] Add AppImage generation for Linux
- [ ] Add optional macOS code signing (certificate-based)
- [ ] Bundle VC++ redistributable for Windows

**前提**: 所有功能稳定，测试覆盖完整

---

## 文件行数违规记录

| File | Lines | Status |
|------|-------|--------|
| `src/girlfriend/girlfriendwindow.cpp` | 1,937 | ⚠️ **DEFERRED** - Phase 4.1 |
| `src/ui/mainwindow.cpp` | 1,769 | ⚠️ **DEFERRED** - Phase 4.2 |
| `src/girlfriend/voicemanager.cpp` | 1,505 | ⚠️ **DEFERRED** - Phase 4.3 |
| `scripts/build.sh` | 1,278 | 🟡 Phase 3.3 (fix prompt) → Phase 7.2 (split) |
| `src/cli/cli_application.cpp` | 1,165 | ⚠️ **DEFERRED** - Phase 4.4 |
| `src/ui/markdownrenderer.cpp` | 855 | 🟡 Phase 3.2 (if needed) |
| `CMakeLists.txt` | 788 | 🟢 Phase 7.1 (optional) |
| `src/knowledge/embedder.cpp` | 591 | 🟢 Phase 3.1 |
| `src/core/networkmanager.cpp` | ~~581~~ 240 | ✅ **Phase 1.2 done** — split into ApiProvider + 3 providers |
| `src/ui/stylesheetmanager.cpp` | 575 | 🟡 Phase 3.2 |
| `src/knowledge/vectordb.cpp` | 555 | 🟢 Phase 3.1 |
| `scripts/setup.sh` | 519 | 🟢 Phase 7.3 |

---

## 执行优先级总结

**立即开始（本周）**:
1. ✅ **Tier 0**: 测试框架（零风险，最高价值）
2. ✅ **Tier 1.1**: FileParser 提取（最低风险，立即收益）
3. ✅ **Tier 1.2**: NetworkManager Provider（低风险，架构改善）

**近期（有基础测试后）**:
4. 🟢 **Tier 2**: 平台修复（Windows 支持，theme fixes）
5. 🟢 **Tier 3**: 中等风险重构（embedder, vectordb, stylesheetmanager）

**远期（有完整测试覆盖后）**:
6. ⚠️ **Tier 4**: 高风险 UI 重构（girlfriendwindow, mainwindow, voicemanager）
7. 🟢 **Tier 5-8**: 代码质量、UI/UX、构建系统、分发打包

---

## SRP Rules (apply to all work)

- Each function does ONE thing (verb in name = what it does)
- No function over ~40 lines; extract helpers aggressively
- Public methods = API surface; private methods = implementation detail
- Qt signals/slots for cross-object communication, not direct calls
- Each file = one class or one coherent set of related functions

## Verification Checklist

Run these after completing critical phases:

- [ ] `find src -name "*.cpp" -o -name "*.h" | xargs wc -l | sort -rn | head -20` — no files >500 lines
- [ ] `cmake --build build --parallel 4` — clean build succeeds
- [ ] `ctest` — all tests pass
- [ ] `./scripts/build.sh run --gui` — UI loads, all features work
- [ ] `./scripts/build.sh run --cli` — CLI chat works
- [ ] Open settings dialog in light theme — all text readable
- [ ] Voice input/output works (requires iFlytek credentials)
- [ ] Windows build and run — command execution works