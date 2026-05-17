# Code Improvement Plan — 基于 docs/review.md 全面审核（第二轮完整版）

**review.md 总声称**：21 C++ + 5 CMake + 5 Shell + 8 GitHub规范 = 39 项

**实际验证结果**：**20 项确认属实**，**12 项不成立**，**7 项部分成立/过度夸大**

---

## 审核验证：review.md 中不成立或过度夸大的声明

| # | 声称的问题 | review.md 评级 | 实际验证结果 |
|---|---|---|---|
| 1 | OllamaProvider "data:" 前缀过滤导致 NDJSON 被丢弃 | 🟠 High | **不成立** — OllamaProvider 重写了 `extractDeltaFromSSE()`，不按 `data:` 过滤 |
| 2 | MarkdownRenderer 末尾 list/table 标签未闭合 | 🟠 High | **不成立** — 循环结束后第 175-183 行有闭合逻辑 |
| 3 | SafetyChecker `startsWith` 路径穿越 | 🟠 High | **不成立** — 比较前已通过 `resolveCanonicalPath()` 规范化路径 |
| 4 | build.sh `$(nproc)` 在 macOS 不存在 | 🟠 High | **不成立** — macOS 分支使用了 `sysctl -n hw.ncpu` |
| 5 | package.sh windeployqt 无回退路径 | 🟠 High | **不成立** — 第 292-295 行有 `QT_PATH/bin/windeployqt.exe` 回退 |
| 6 | CMakeLists.txt `set(CMAKE_BUILD_TYPE Release)` 强制 Release | 🟡 Medium | **不成立** — CMakeLists.txt 中没有任何 `CMAKE_BUILD_TYPE` 设置 |
| 7 | 缺少 `install(TARGETS ...)` 规则 | 🟢 Low | **不成立** — 第 671/674/689/703 行为所有平台定义了 install 规则 |
| 8 | package.sh `cp -r build/Release/*.exe` 无前置检查 | 🟡 Medium | **不成立**（路径不匹配）— 实际路径是 `$BUILD_DIR/*.exe`，且有 `2>/dev/null \|\| true` |
| 9 | AnthropicProvider SSE "跨包静默丢弃" | 🟠 High | **部分成立** — `currentEvent` 是局部变量，但 JSON 内 `type` 字段提供了回退（影响有限） |
| 10 | Windows 语音输入 `WA_InputMethodEnabled` | 🟠 High | **部分成立** — 属性默认启用；`AllocConsole` 仅在 debug 模式 |
| 11 | 全部脚本缺少 `--help` | 🟢 Low | **部分成立** — build.sh 和 package.sh 有 --help；setup.sh 和 format.sh 没有 |
| 12 | PromptManager 缓存被 QThreadPool 调用 | 🟡 Medium | **部分成立** — 缓存确实无线程保护，但 `knowledgePrompt()` 实际从主线程调用 |

---

## Phase 1: Critical Fixes — C++ 稳定性（6 项）

### Fix 1 — SessionManager::removeSession 悬空 ID

**文件**: [src/core/sessionmanager.cpp:108-110](src/core/sessionmanager.cpp#L108-L110)

**当前（3行）**:
```cpp
void SessionManager::removeSession(const QString& sessionId) {
    m_sessions.remove(sessionId);
}
```

**问题**: 删除当前 session 后，`m_currentSessionId` 仍指向已删除的 key。`currentSession()` 调用 `m_sessions[m_currentSessionId]`，QMap 的 `operator[]` 会静默插入一个带新 UUID 的空 ChatSession。

**修复**: 删除后检查是否为当前 session，若是则选择剩余 session 中的第一个，若无剩余则调用 `createNewSession()`。同时更新 QSettings 中的 lastSessionId。

---

### Fix 2 — Embedder ONNX 名称内存泄漏

**文件**: [src/knowledge/embedder.cpp:97,106](src/knowledge/embedder.cpp#L97), [src/knowledge/embedder.h:75-76](src/knowledge/embedder.h#L75-L76)

**问题**: `qstrdup()` 分配堆内存，存入 `QVector<const char*>` 成员。析构函数为 `= default`，永不释放。每次 `loadModel()` 调用都会泄漏之前的分配。

**修复**:
- 在 `loadModel()` 开头添加清理循环：遍历 `m_onnxInputNames`/`m_onnxOutputNames`，对每个名称 `delete[]`，然后 `.clear()`
- 将 `~Embedder() = default;` 改为显式析构函数，释放名称存储

---

### Fix 3 — KnowledgeBase 线程安全

**文件**: [src/knowledge/knowledgebase.cpp:80-93](src/knowledge/knowledgebase.cpp#L80-L93), [src/knowledge/knowledgebase.h:65-70](src/knowledge/knowledgebase.h#L65-L70)

**问题**: `importDocumentAsync` lambda 为 QThreadPool 捕获了裸 `[this]`。无互斥锁保护 `m_ready`/`m_importer` 访问。

**修复**:
- 在头文件中添加 `QMutex m_mutex` 成员
- 在 `importDocumentAsync` 和 `importDocumentsAsync` 中：锁定互斥锁，拷贝 `m_importer` 指针，解锁，然后在副本上调用 `importDocument`
- 保持锁范围最小（不在昂贵的导入调用期间持有锁）

---

### Fix 4 — AgentLoop 阻塞 UI 线程

**文件**: [src/tasks/agentloop.cpp:132-156](src/tasks/agentloop.cpp#L132-L156), [src/tasks/agentloop.h](src/tasks/agentloop.h)

**问题**: `executeAndContinue` 同步调用 `engine->executePlan(plan)`，经过 `TaskEngine` → `CommandExecutor::runCommand` 创建嵌套 `QEventLoop`。UI 在最多 10 次迭代期间冻结。

**修复**:
- 同步执行计划（嵌套的 QEventLoop 已经处理事件），但使用 `QTimer::singleShot(0, ...)` 推迟反馈/继续，让调用处理程序先返回
- 添加 `QVector<CommandResult> m_pendingResults` 成员以在异步边界之间存储结果

---

### Fix 5 — SessionManager 每次 addMessage 同步保存

**文件**: [src/core/sessionmanager.cpp:60,70,79,88](src/core/sessionmanager.cpp#L60), [src/core/sessionmanager.h](src/core/sessionmanager.h)

**问题**: 所有四个 `addMessage*` 方法都同步调用 `saveSessionsToFile()`。Agent 10 次迭代 = 10+ 次全量 session map 文件写入。

**修复**:
- 添加 `QTimer* m_saveDebounceTimer` 成员（单次触发，2000ms）
- 将所有四处的 `saveSessionsToFile()` 调用替换为 `m_saveDebounceTimer->start()`
- 连接定时器超时信号到 `saveSessionsToFile`
- 在 `closeEvent` 和流完成路径保留显式保存

---

### Fix 6 — Agent/流式路径中的双重渲染

**文件**: [src/ui/mainwindow.cpp:213-221](src/ui/mainwindow.cpp#L213-L221)

**问题**: `onStreamFinished` 调用 `addMessageToSession` → 发出 `sessionChanged` → `renderCurrentSession()` 触发。同时流式路径也在更新显示。`m_suppressRender` 仅在 `onSendClicked` 中设置，从不在 agent/流式路径中使用。

**修复**: 在 `sessionChanged` lambda 连接（第 213 行）中添加 `m_isStreaming` 守卫：
```cpp
if (m_suppressRender || m_isStreaming) return;
```
流式更新显示时跳过完整的 `setHtml()` 重新渲染。

---

## Phase 2: High/Medium Fixes — C++ 正确性与打磨（9 项）

### Fix 7 — AnthropicProvider SSE currentEvent → 成员变量

**文件**: [src/core/anthropic_provider.cpp:99](src/core/anthropic_provider.cpp#L99), [src/core/apiprovider.h](src/core/apiprovider.h)

将 `currentEvent` 从局部变量改为 `ApiProvider::m_sseCurrentEvent` 成员。在 `abortCurrentRequest()` 和 `onStreamFinished()` 中清除。JSON `type` 字段提供了回退，所以紧急度低。

### Fix 8 — CommandExecutor 环境变量展开迭代限制

**文件**: [src/tasks/commandexecutor.cpp:83-106](src/tasks/commandexecutor.cpp#L83-L106)

在 `expandPath()` 的三个 `while` 循环中添加 `int maxIterations = 100` 计数器。超过计数器时退出。防止循环环境变量引用时的无限循环。

### Fix 9 — 版本号统一

| 文件 | 当前值 | 改为 |
|---|---|---|
| [src/cli/cli_application.cpp:93,214](src/cli/cli_application.cpp#L93) | `"1.0.0"` | `"1.1.1"` |
| [src/ui/main.cpp:65](src/ui/main.cpp#L65) | `"1.1.0"` | `"1.1.1"` |
| [CMakeLists.txt:48](CMakeLists.txt#L48) | `1.1.1` | （规范值，不变） |

存在三个不同版本：1.0.0（CLI）、1.1.0（GUI）、1.1.1（CMake）。

### Fix 10 — MemoryEnhancer O(n²) 搜索 → O(n)

**文件**: [src/knowledge/memoryenhancer.cpp:124-138](src/knowledge/memoryenhancer.cpp#L124-L138), [src/knowledge/memoryenhancer.h](src/knowledge/memoryenhancer.h)

添加 `QHash<QString, int> m_entryIndex`（id → m_entries 中的索引）。用哈希查找替换内部线性扫描。在添加/删除/加载时维护索引。

### Fix 11 — PromptManager 缓存互斥锁

**文件**: [src/prompts/promptmanager.h:56](src/prompts/promptmanager.h#L56), [src/prompts/promptmanager.cpp:231-255](src/prompts/promptmanager.cpp#L231-L255)

添加 `mutable QMutex m_cacheMutex`。用 `QMutexLocker` 守卫所有 `m_cache` 读写。实际上调用仅来自主线程，但设计应当健全。

### Fix 12 — VectorDB 唯一化 SQLite 连接名

**文件**: [src/knowledge/vectordb.cpp:50](src/knowledge/vectordb.cpp#L50), [src/knowledge/vectordb.h](src/knowledge/vectordb.h)

将硬编码的 `"vectordb_conn"` 替换为存储在 `m_connName` 成员中的 `"vectordb_conn_" + hex(this)`。防止测试多实例冲突。更新全部 6 个调用点。

### Fix 13 — 5 个类的 Meyers 单例

**文件**: SessionManager、TaskEngine、AgentLoop、KnowledgeBase、PromptManager（`.h` 和 `.cpp`）

从裸 `static T* s_instance = nullptr; s_instance = new T();` 转换为函数局部静态。遵循已有的 `StyleSheetManager::instance()` 模式。检查析构函数中 QObject 子对象的重复删除。

### Fix 14 — kTransferTimeoutMs 可配置

**文件**: [src/core/apiprovider.cpp:9](src/core/apiprovider.cpp#L9)

从 `static constexpr int` 改为从 `QSettings` 加载的 `int m_transferTimeoutMs` 成员。添加 setter。最小 5000ms。

### Fix 15 — zh_TW/HK 地区注释

**文件**: [src/ui/main.cpp:79](src/ui/main.cpp#L79), [src/prompts/promptmanager.cpp:126](src/prompts/promptmanager.cpp#L126)

无行为变更——不存在繁体中文翻译。添加代码注释说明有意回退到简体中文。

---

## Phase 3: CMake 构建改进（2 项确认属实 + 1 项低优先级）

review.md 声称 5 个 CMake 问题，**2 个不成立**（CM2 强制 Release 不存在、CM5 install 规则已存在）。

### Fix 16 — CMakeLists.txt Windows Qt 路径自动检测

**文件**: [CMakeLists.txt:5-40](CMakeLists.txt#L5-L40)

**问题**: Qt 自动检测块仅有 `if(APPLE)` 和 `elseif(UNIX AND NOT APPLE)` 分支，缺少 `elseif(WIN32)` 分支。Windows 开发者需手动设置 `QT_PATH`。

**修复**: 在 `endif()` 前添加 `elseif(WIN32)` 分支，检测 `C:/Qt/<version>/msvc2019_64` 或类似路径。

### Fix 17 — 测试文件添加 CTest 标签

**文件**: [tests/CMakeLists.txt](tests/CMakeLists.txt)

**问题**: 8 个测试均未设置 `LABELS` 属性，无法通过 `ctest -L unit` 筛选。

**修复**: 为每个 `add_test()` 调用添加 `set_tests_properties(<name> PROPERTIES LABELS unit)`。

### Fix 18（可选）— 宏命名风格统一

**文件**: [CMakeLists.txt:323,341](CMakeLists.txt#L323)

**问题**: `HNSWLIB_AVAILABLE` 缩写为 "LIB"，`ONNXRUNTIME_AVAILABLE` 使用全名。风格不一致但功能正确。

**修复**: 低优先级。可以将 `HNSWLIB_AVAILABLE` 重命名为 `HNSWLIBRARY_AVAILABLE`，但需要同步更新 `src/knowledge/vectordb.cpp` 和 `.h` 中的 12 处引用。

---

## Phase 4: 工程规范补充（GitHub 规范 + 测试覆盖 + Shell 脚本）

review.md 声称的 8 个 GitHub规范问题 **全部确认属实**。

### Fix 19 — 补全 GitHub 社区规范文件

创建以下文件：
- `.github/ISSUE_TEMPLATE/bug_report.md` — Bug 报告模板（含平台/API 类型/复现步骤）
- `.github/ISSUE_TEMPLATE/feature_request.md` — 功能请求模板
- `.github/PULL_REQUEST_TEMPLATE.md` — PR 模板（含变更描述/测试确认/平台验证复选框）
- `CONTRIBUTING.md` — 贡献指南（开发环境搭建/提交规范/PR 流程）
- `CHANGELOG.md` — 语义化版本变更记录

### Fix 20 — 补充测试覆盖（高优先级）

**当前测试缺口**（review.md 第 2.4 节全部确认）：

| 缺失测试 | 优先级 | 说明 |
|---|---|---|
| AgentLoop 业务流测试 | 🟠 High | 当前仅测信号存在性和状态，无 start→execute→feedback 循环 |
| SessionManager 删除当前 session 回归 | 🟠 High | `testRemoveSession` 未验证 ID 切换行为（恰好是 Fix 1 的回归用例） |
| VectorDB 测试 | 🟠 High | 全新模块，无测试 |
| Embedder 测试 | 🟠 High | 全新模块，无测试 |
| KnowledgeBase 测试 | 🟠 High | 全新模块，无测试 |
| ApiProvider SSE 解析测试 | 🟠 High | 网络流式解析无覆盖 |

### Fix 21 — Shell 脚本改进

**问题**: `scripts/setup.sh` 和 `scripts/format.sh` 缺少 `--help` 用法输出。所有脚本均无日志分级。

**修复**:
- 为 `setup.sh` 添加 `--help` / `-h` 参数处理
- 为 `format.sh` 添加 `--help` / `-h` 参数处理
- （可选后续）为所有脚本引入颜色标记的 INFO/WARN/ERROR 输出

---

## 不执行的修复（review.md 声明经核实不成立）

| 声称的问题 | review.md 评级 | 核实结论 |
|---|---|---|
| OllamaProvider "data:" 过滤 | 🟠 High | 不成立 — 代码正确 |
| MarkdownRenderer 未闭合标签 | 🟠 High | 不成立 — 闭合逻辑存在 |
| SafetyChecker 路径穿越 | 🟠 High | 不成立 — 路径已规范化 |
| build.sh nproc macOS | 🟠 High | 不成立 — macOS 分支正确 |
| package.sh windeployqt 无回退 | 🟠 High | 不成立 — 有 QT_PATH 回退 |
| CMake 强制 Release 类型 | 🟡 Medium | 不成立 — 无此设置 |
| CMake 缺少 install 规则 | 🟢 Low | 不成立 — 所有平台已定义 |
| package.sh cp *.exe 无检查 | 🟡 Medium | 不成立 — 路径不匹配，且有容错 |
| Windows WA_InputMethodEnabled | 🟠 High | 过度夸大 — 默认启用，AllocConsole 仅 debug |
| AnthropicProvider SSE 丢弃 | 🟠 High | 过度夸大 — JSON type 字段有回退 |

---

## 依赖关系

- **Fix 4（AgentLoop 异步）与 Fix 6（双重渲染）**: Fix 4 使 agent 迭代异步化后，Fix 6 的 `m_isStreaming` 守卫更加重要
- **Fix 5（去抖动保存）与 Fix 4**: 异步迭代受益于保存合并
- **Fix 1（removeSession）与 Fix 20（测试）**: Fix 1 的修复需要对应的回归测试
- **所有其他修复均独立**，可按任意顺序进行

---

## 建议执行顺序

1. **Phase 1**（C++ Critical）：Fix 1 → Fix 2 → Fix 3 → Fix 5 → Fix 6 → Fix 4（Fix 4 放最后因为改动最大）
2. **Phase 2**（C++ High/Medium）：Fix 9 → Fix 8 → Fix 10 → Fix 12 → Fix 7 → Fix 14 → Fix 11 → Fix 15 → Fix 13
3. **Phase 3**（CMake）：Fix 16 → Fix 17
4. **Phase 4**（规范 + 测试）：Fix 20 → Fix 19 → Fix 21

---

## 验证方式

1. **Fix 1**: 通过 GUI/CLI 删除当前 session → `currentSession()` 返回有效 session
2. **Fix 2**: ASAN 下运行，`loadModel()` 两次 → ONNX 名称无泄漏
3. **Fix 3**: 快速切换 KB 时调用 `importDocumentAsync` → 无崩溃
4. **Fix 4**: 多迭代 agent 任务 → UI 保持响应
5. **Fix 5**: agent 循环期间文件写入 → 每 2 秒最多 1 次
6. **Fix 6**: agent 循环期间聊天显示 → 无闪烁
7. **Fix 8**: `export A='$B' && export B='$A'`，调用 `expandPath("$A")` → 有界时间内返回
8. **Fix 9**: `--version` 两个二进制均打印 `1.1.1`
9. **Fix 10**: 10k 条目 `MemoryEnhancer::search()` → 常数时间
10. **Fix 12**: 两个 VectorDB 实例 → 均无 SQLite 错误
11. **完整构建**: `cmake --build build --parallel 4` 成功
12. **完整测试**: `ctest` 通过
