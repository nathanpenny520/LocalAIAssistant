先把所有未查看的文件夹全部读完。现在读取 cli 和 prompts 文件夹。全部文件夹已读取完毕。现在整合所有信息输出最终完整报告。

---

# LocalAIAssistant — 企业级代码全量审查报告

**审查范围**：src/（全部8个模块）、tests/、scripts/（6个脚本）、.github/workflows/、CMakeLists.txt、docs/、cmake/、third_party/

**代码规模**：75个源文件，约19,324行，CMakeLists.txt 902行，Shell脚本 2,645行，测试文件8个约1,780行

---

## 1. 【项目整体评级】：良好偏优（B+）

项目整体工程化成熟度属个人开源项目中的上游水平：三平台 CI/CD 完整、代码规范工具链齐全（clang-format / cmake-format / shfmt / prettier）、中英双语文档详尽、CMake 现代化程度高、测试框架已搭建。核心短板集中在异步架构设计缺陷（Agent 阻塞主线程）、流式渲染竞争、以及 KnowledgeBase / Embedder 的线程安全问题，这些是直接影响用户体验的已知 Bug 根源。

---

## 2. 【现存核心问题汇总】

### 2.1 C++ 代码核心问题

| # | 类别 | 问题 | 严重度 | 位置 |
|---|---|---|---|---|
| 1 | **线程安全** | `importDocumentAsync` 用 lambda 捕获裸 `this`，无互斥保护，主线程销毁对象时 use-after-free | 🔴 Critical | `knowledge/knowledgebase.cpp:81` |
| 2 | **主线程阻塞** | `AgentLoop::executeAndContinue` 同步调用 `executePlan`，QEventLoop 嵌套阻塞 UI，最多 10 轮迭代全程冻结 | 🔴 Critical | `tasks/agentloop.cpp` |
| 3 | **渲染竞争** | `onStreamFinished→handleTaskResponse→AgentLoop::addMessageToSession→sessionChanged→renderCurrentSession` 二次渲染，`m_suppressRender` 对 Agent 路径无保护 | 🔴 Critical | `ui/mainwindow.cpp` |
| 4 | **悬空 ID** | `removeSession` 只删 map 条目，不更新 `m_currentSessionId`；后续 `currentSession()` 用 `QMap::operator[]` 静默插入空 ChatSession | 🔴 Critical | `core/sessionmanager.cpp` |
| 5 | **内存泄漏** | ONNX 输入输出名称用 `qstrdup()` 分配（malloc），存入 `QVector<const char*>` 但从不 `free`；每次 `loadModel` 都泄漏 | 🔴 Critical | `knowledge/embedder.cpp` |
| 6 | **SSE 跨包丢失** | Anthropic SSE 解析中 `currentEvent` 是局部变量；TCP 分包时 event/data 行跨 readyRead 到达，事件类型丢失，导致流式 chunk 被静默丢弃 | 🟠 High | `core/anthropic_provider.cpp` |
| 7 | **NDJSON 被 SSE 误处理** | Ollama 返回 NDJSON（每行完整 JSON），父类 `onStreamReadyRead` 按 `data:` 前缀过滤，Ollama 内容全部被丢弃 | 🟠 High | `core/ollama_provider.cpp` |
| 8 | **高频磁盘写入** | 每次 `addMessage` 都同步 `saveSessionsToFile`，Agent 10 轮迭代最多写 10 次；无 debounce，卡顿明显 | 🟠 High | `core/sessionmanager.cpp` |
| 9 | **路径穿越** | `isPathSafe` 用 `startsWith` 比较未规范化路径；AI 生成 `../../etc/passwd` 可绕过白名单 | 🟠 High | `tasks/safetychecker.cpp` |
| 10 | **HTML 末尾未关闭** | `toHtml()` 循环结束后若 `inList/inTable` 仍为 true，`</ul>/<table>` 不追加，HTML 不合法，QTextBrowser 丢失末尾内容 | 🟠 High | `ui/markdownrenderer.cpp` |
| 11 | **Windows 语音输入** | `QPlainTextEdit` 未设置 `WA_InputMethodEnabled`；`AllocConsole()` 可能抢焦点干扰语音识别目标窗口 | 🟠 High | `ui/mainwindow.cpp`, `ui/main.cpp` |
| 12 | **环境变量展开无上限** | 三段 `while` 循环展开 `$VAR/%VAR%`，仅以 `varValue.isEmpty()` 为 break 条件；循环引用变量理论上可无限循环 | 🟠 High | `tasks/commandexecutor.cpp` |
| 13 | **版本号硬编码不一致** | `cli_application.cpp` 写死 `"1.0.0"`，CMakeLists.txt 为 `1.1.1` | 🟡 Medium | `cli/cli_application.cpp:L62` |
| 14 | **O(n²) 记忆检索** | `MemoryEnhancer::search` 对每个向量检索结果执行全量线性扫描 `m_entries`，条目多时性能退化 | 🟡 Medium | `knowledge/memoryenhancer.cpp` |
| 15 | **PromptManager 缓存线程不安全** | `mutable QMap<> m_cache` 无互斥保护；知识库在 QThreadPool 中调用 `generateContext→PromptManager::knowledgePrompt()` 时存在数据竞争 | 🟡 Medium | `prompts/promptmanager.cpp` |
| 16 | **VectorDB SQLite 全局连接名** | 使用固定连接名 `"vectordb_conn"`；测试环境多实例并发时连接互相覆盖 | 🟡 Medium | `knowledge/vectordb.cpp` |
| 17 | **单例析构顺序 UB** | `SessionManager`、`TaskEngine`、`AgentLoop`、`KnowledgeBase`、`PromptManager` 均裸 `new`；QApplication 析构后访问残存 QTranslator 触发 UB | 🟡 Medium | 全局单例 |
| 18 | **kTransferTimeoutMs 硬编码** | Agent 多步场景下 120s 可能不够；慢速本地模型（llama.cpp）触发超时中断任务 | 🟡 Medium | `core/apiprovider.cpp:L7` |
| 19 | **繁简体 locale 误映射** | `zh_TW`、`zh_HK` 被映射为 `zh_CN` 简体 | 🟢 Low | `ui/main.cpp` |
| 20 | **VectorDB 析构不置空** | `delete m_index; delete m_space` 后未置 nullptr（析构一般无影响，但不符合 RAII 最佳实践） | 🟢 Low | `knowledge/vectordb.cpp` |
| 21 | **mainwindow.cpp 职责过重** | 70KB/1746行，包含 UI、流式渲染、Agent 调度、文件管理、搜索、IME 等多关注点，ROADMAP 已计划拆分 | 🟢 Low | `ui/mainwindow.cpp` |

### 2.2 CMake 工程构建问题

| # | 问题 | 严重度 |
|---|---|---|
| 1 | Windows 本地开发无 Qt 路径自动检测（macOS/Linux 已实现，Windows 缺 `elseif(WIN32)` 分支） | 🟡 Medium |
| 2 | `set(CMAKE_BUILD_TYPE Release)` 若存在此行则强制 Release，开发者无法直接 Debug | 🟡 Medium |
| 3 | hnswlib 宏用 `HNSWLIB_AVAILABLE` 而 Embedder 用 `ONNXRUNTIME_AVAILABLE`；两个可选功能的宏命名风格不统一 | 🟢 Low |
| 4 | 测试 `CMakeLists.txt` 缺少 `set_tests_properties(... LABELS unit)` 标签，无法通过 `ctest -L unit` 筛选 | 🟢 Low |
| 5 | 缺少 `install(TARGETS ...)` 规则；非 macOS/Windows 包用户无法 `cmake --install` | 🟢 Low |

### 2.3 Shell 脚本问题

| # | 脚本 | 问题 | 严重度 |
|---|---|---|---|
| 1 | `build.sh` | `$(nproc)` 在 macOS 不存在，应改为 `$(nproc 2>/dev/null \|\| sysctl -n hw.logicalcpu)` | 🟠 High |
| 2 | `package.sh` | `windeployqt` 路径依赖 PATH，未用 `which windeployqt \|\| qtpaths --binaries-dir` 动态查找 | 🟠 High |
| 3 | `package.sh` | `cp -r build/Release/*.exe` 无前置检查，Release 目录不存在时静默失败 | 🟡 Medium |
| 4 | `setup.sh` | 21KB 复杂逻辑，多平台分支下错误处理一致性待审；建议显式 `set -euo pipefail` | 🟡 Medium |
| 5 | 全部脚本 | 缺少 `--help` 用法输出、无日志分级（INFO/WARN/ERROR 用颜色区分） | 🟢 Low |

### 2.4 GitHub 开源规范问题

| # | 问题 | 严重度 |
|---|---|---|
| 1 | 缺少 `.github/ISSUE_TEMPLATE/`（Bug 报告 + 功能请求模板） | 🟡 Medium |
| 2 | 缺少 `.github/PULL_REQUEST_TEMPLATE.md` | 🟡 Medium |
| 3 | 缺少 `CONTRIBUTING.md` 贡献指南 | 🟡 Medium |
| 4 | 缺少 `CHANGELOG.md` 版本变更记录 | 🟡 Medium |
| 5 | 应用未签名（README 已说明），影响 macOS Gatekeeper / Windows SmartScreen 首次启动体验 | 🟡 Medium |
| 6 | `test_agentloop.cpp` 仅测信号存在性，未覆盖真实业务流（start/execute/feedback 循环） | 🟠 High |
| 7 | `test_sessionmanager.cpp::testRemoveSession` 未验证删除当前 session 后 ID 切换行为（BUG-04 的回归用例缺失） | 🟠 High |
| 8 | 知识库模块（VectorDB、Embedder、KnowledgeBase）、网络模块（ApiProvider SSE 解析）无测试覆盖 | 🟠 High |

---

## 3. 【高危 BUG & 紧急整改项】

### 🔴 FIX-01：AgentLoop 异步化，消除主线程阻塞

**根因**：`executeAndContinue` 同步调用 `executePlan`，内部 `QProcess::waitForFinished` 嵌套 `QEventLoop` 阻塞 UI 线程。

```cpp
// tasks/agentloop.h — 新增
#include <QFutureWatcher>
private:
    QFutureWatcher<QVector<CommandResult>>* m_watcher = nullptr;

// tasks/agentloop.cpp
void AgentLoop::executeAndContinue(const OperationPlan& plan) {
    m_iterationCount++;
    if (m_iterationCount > m_maxIterations) {
        m_state = MaxIterations;
        emit stateChanged(m_state);
        emit loopFinished(tr("Maximum iterations reached (%1)").arg(m_maxIterations), m_sessionId);
        return;
    }

    // ✅ 异步执行，不阻塞主线程
    m_watcher = new QFutureWatcher<QVector<CommandResult>>(this);
    connect(m_watcher, &QFutureWatcher<QVector<CommandResult>>::finished,
            this, [this]() {
        auto results = m_watcher->result();
        m_watcher->deleteLater();
        m_watcher = nullptr;

        QString feedback = buildResultFeedback(results);
        m_lastFeedback = feedback;

        ChatMessage feedbackMsg("user", feedback);
        feedbackMsg.isAgentLoopInjected = true;
        // 使用静默版本，避免触发 renderCurrentSession
        SessionManager::instance()->addMessageToSessionSilent(m_sessionId, feedbackMsg);
        emit executionResultReady(feedback, m_sessionId);
    });
    m_watcher->setFuture(QtConcurrent::run(
        [plan]() { return TaskEngine::instance()->executePlan(plan); }
    ));
}
```

同时确认 `CommandExecutor::runCommand` 超时有效：

```cpp
// tasks/commandexecutor.cpp
if (!m_currentProcess->waitForFinished(timeoutSecs * 1000)) {
    m_currentProcess->kill();
    result.success = false;
    result.errorMessage = tr("Command timed out after %1s").arg(timeoutSecs);
}
```

---

### 🔴 FIX-02：消除流式结束后双重渲染

**根因**：`AgentLoop` 调用 `addMessageToSession` → `sessionChanged` → `renderCurrentSession`，与流式结束时的渲染产生竞争。

```cpp
// core/sessionmanager.h — 新增静默版本
void addMessageToSessionSilent(const QString& sessionId, const ChatMessage& message);

// core/sessionmanager.cpp
void SessionManager::addMessageToSessionSilent(const QString& sessionId,
                                                const ChatMessage& message) {
    if (!m_sessions.contains(sessionId)) return;
    m_sessions[sessionId].messages.append(message);
    truncateSession(sessionId);
    scheduleSave(); // 走 debounce 路径，不 emit sessionChanged
}

// ui/mainwindow.cpp — AgentLoop 完成时才做一次完整渲染
void MainWindow::onAgentLoopFinished(const QString& /*summary*/,
                                      const QString& sessionId) {
    if (sessionId == SessionManager::instance()->currentSessionId()) {
        renderCurrentSession(); // 唯一的渲染触发点
    }
    setInputEnabled(true);
}
```

---

### 🔴 FIX-03：removeSession 悬空 ID

```cpp
// core/sessionmanager.cpp
void SessionManager::removeSession(const QString& sessionId) {
    m_sessions.remove(sessionId);

    if (m_currentSessionId == sessionId) {
        if (!m_sessions.isEmpty()) {
            m_currentSessionId = m_sessions.firstKey();
        } else {
            createNewSession(); // 保证至少有一个 session
            return;
        }
        emit sessionChanged(m_currentSessionId);
    }
}

// 防御性修改 currentSession()，彻底杜绝静默插入
ChatSession& SessionManager::currentSession() {
    auto it = m_sessions.find(m_currentSessionId);
    Q_ASSERT_X(it != m_sessions.end(), "currentSession",
               "currentSessionId not in sessions map — call removeSession() to fix");
    return it.value();
}
```

---

### 🔴 FIX-04：Anthropic SSE currentEvent 跨包保持

```cpp
// core/anthropic_provider.h
private:
    QString m_pendingEvent; // 跨 readyRead 调用保持事件类型

// core/anthropic_provider.cpp
QString AnthropicProvider::extractDeltaFromSSE(const QByteArray& data) {
    QString result;
    const QStringList lines = QString::fromUtf8(data).split('\n');

    for (const QString& line : lines) {
        if (line.startsWith("event: ")) {
            m_pendingEvent = line.mid(7).trimmed();
        } else if (line.startsWith("data: ")) {
            QString jsonStr = line.mid(6).trimmed();
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
            if (doc.isNull() || !doc.isObject()) continue;

            QJsonObject root = doc.object();
            if (root.contains("error")) {
                emit errorOccurred(root["error"].toObject()["message"].toString("Unknown error"));
                m_pendingEvent.clear();
                return result;
            }

            const QString eventType = m_pendingEvent.isEmpty()
                                          ? root["type"].toString()
                                          : m_pendingEvent;
            if (eventType == "content_block_delta") {
                QJsonObject delta = root["delta"].toObject();
                if (delta["type"].toString() == "text_delta")
                    result += delta["text"].toString();
            }
            m_pendingEvent.clear(); // 消费后清空
        } else if (line.trimmed().isEmpty()) {
            m_pendingEvent.clear(); // SSE 空行 = 事件块边界
        }
    }
    return result;
}
```

---

### 🔴 FIX-05：Embedder ONNX 名称内存泄漏

```cpp
// knowledge/embedder.h
private:
    // ✅ 改为管理生命周期的容器
    QVector<QByteArray> m_onnxInputNameStorage;
    QVector<QByteArray> m_onnxOutputNameStorage;
    QVector<const char*> m_onnxInputNames;
    QVector<const char*> m_onnxOutputNames;

// knowledge/embedder.cpp — loadModel() 内
m_onnxInputNameStorage.clear();
m_onnxOutputNameStorage.clear();
m_onnxInputNames.clear();
m_onnxOutputNames.clear();

for (size_t i = 0; i < numInputs; ++i) {
    auto name = m_session->GetInputNameAllocated(i, allocator);
    m_onnxInputNameStorage.append(QByteArray(name.get())); // QByteArray 管理内存
    m_onnxInputNames.append(m_onnxInputNameStorage.last().constData());
    if (m_onnxInputNameStorage.last() == "token_type_ids") m_hasTokenTypeIds = true;
}
// 输出同理
```

---

### 🔴 FIX-06：KnowledgeBase 异步导入线程安全

```cpp
// knowledge/knowledgebase.cpp
void KnowledgeBase::importDocumentAsync(const QString& filePath) {
    // ✅ 用 QPointer 保护 this，避免 use-after-free
    QPointer<KnowledgeBase> guard(this);

    QThreadPool::globalInstance()->start([guard, filePath]() {
        if (!guard) return; // KnowledgeBase 已销毁

        // m_importer 访问需要互斥（或用 QtConcurrent + signal 结果回调）
        ImportResult result;
        {
            QMutexLocker locker(&guard->m_mutex); // 新增 QMutex m_mutex
            if (!guard->m_ready || !guard->m_importer) return;
            result = guard->m_importer->importDocument(filePath);
        }

        QMetaObject::invokeMethod(guard, [guard, result]() {
            if (!guard) return;
            if (result.success)
                emit guard->documentImported(result.documentPath, result.chunkCount);
            else
                emit guard->importFailed(result.documentPath, result.errorMessage);
        }, Qt::QueuedConnection);
    });
}
```

---

### 🟠 FIX-07：MarkdownRenderer 末尾状态清理

```cpp
// ui/markdownrenderer.cpp — toHtml() for 循环结束后立即追加：
if (inList) {
    result += QString("<ul style='margin:8px 0;padding-left:20px;color:%1;'>")
                  .arg(t.textPrimary.name()) +
              listHtml + "</ul>\n";
}
if (inTable && !tableLines.isEmpty()) {
    result += processTable(tableLines, t);
}
if (inCodeBlock && !codeBlockContent.isEmpty()) {
    // 防御性：AI 响应中断时代码块未关闭
    result += "<pre><code>" +
              highlightCode(codeBlockContent, codeBlockLanguage, t) +
              "</code></pre>\n";
}
```

---

### 🟠 FIX-08：SessionManager 高频写磁盘 debounce

```cpp
// core/sessionmanager.h
private:
    QTimer* m_saveTimer = nullptr;
    void scheduleSave();
    void flushNow(); // 退出时立刻落盘

// core/sessionmanager.cpp
SessionManager::SessionManager(QObject* parent) : QObject(parent) {
    // ... 现有初始化 ...
    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(500); // 500ms 防抖
    connect(m_saveTimer, &QTimer::timeout, this, &SessionManager::saveSessionsToFile);
}
void SessionManager::scheduleSave() {
    if (!m_saveTimer->isActive()) m_saveTimer->start();
}
void SessionManager::flushNow() {
    m_saveTimer->stop();
    saveSessionsToFile();
}
// 所有原 saveSessionsToFile() 调用 → 改为 scheduleSave()

// ui/mainwindow.cpp — closeEvent
void MainWindow::closeEvent(QCloseEvent* event) {
    SessionManager::instance()->flushNow(); // 确保退出数据落盘
    event->accept();
}
```

---

### 🟠 FIX-09：SafetyChecker 路径穿越修复

```cpp
// tasks/safetychecker.cpp
bool SafetyChecker::isPathSafe(const QString& path) const {
    // ✅ 规范化后再比较，防止 ../../ 穿越
    const QString normalized =
        QDir::cleanPath(QDir(path).absolutePath());

    auto check = [&](const QString& allowed) -> bool {
        const QString na = QDir::cleanPath(QDir(allowed).absolutePath());
        return normalized == na || normalized.startsWith(na + '/');
    };

    for (const QString& a : m_allowedPaths)
        if (check(a)) return true;
    for (const QString& t : m_temporaryAllowedPaths)
        if (check(t)) return true;
    return false;
}
```

---

### 🟠 FIX-10：Ollama 独立流式读取（绕过 SSE buffer）

```cpp
// core/ollama_provider.h
protected:
    void onNdjsonReadyRead(QNetworkReply* reply); // 新增

// core/ollama_provider.cpp — 在 sendChatRequest 的 connect 中替换
connect(m_currentReply, &QNetworkReply::readyRead,
        this, [this]() { onNdjsonReadyRead(m_currentReply); });

void OllamaProvider::onNdjsonReadyRead(QNetworkReply* reply) {
    m_streamBuffer += QString::fromUtf8(reply->readAll());
    int pos;
    while ((pos = m_streamBuffer.indexOf('\n')) != -1) {
        QString line = m_streamBuffer.left(pos).trimmed();
        m_streamBuffer = m_streamBuffer.mid(pos + 1);
        if (line.isEmpty()) continue;
        const QString chunk = extractDeltaFromSSE(line.toUtf8());
        if (!chunk.isEmpty()) emit streamChunkReceived(chunk);
    }
}
```

---

## 4. 【代码细节优化建议】

### 4.1 单例统一改为 Meyer's Singleton

```cpp
// 所有单例统一写法（SessionManager / TaskEngine / AgentLoop /
//                   KnowledgeBase / PromptManager）
static SessionManager* SessionManager::instance() {
    static SessionManager s_instance; // C++11 线程安全，析构顺序确定
    return &s_instance;
}
// StyleSheetManager 已是此写法，作为模板
```

### 4.2 apiprovider.cpp — RAII 替代临时 swap

```cpp
void ApiProvider::sendChatRequest(const QVector<ChatMessage>& messages) {
    // 构建 effectivePrompt（纯局部，不修改成员）
    QString effectivePrompt = m_systemPrompt;
    // ... 时间占位符替换 ...

    const QString knowledgeSnapshot = m_knowledgeContext;
    m_knowledgeContext.clear(); // 明确消费语义

    if (!knowledgeSnapshot.isEmpty())
        effectivePrompt += buildKnowledgeHeader() + knowledgeSnapshot;

    // RAII guard：自动还原，异常安全
    struct PromptGuard {
        QString& ref; const QString saved;
        PromptGuard(QString& r, const QString& v) : ref(r), saved(r) { ref = v; }
        ~PromptGuard() { ref = saved; }
    } guard(m_systemPrompt, effectivePrompt);

    QJsonObject payload = buildBasePayload();
    payload["messages"] = buildMessagesArray(messages);
    // guard 析构自动还原 m_systemPrompt
    // ... 发送 ...
}
```

### 4.3 MemoryEnhancer 检索优化（O(n²) → O(n)）

```cpp
// knowledge/memoryenhancer.cpp — search()
QVector<MemoryEntry> MemoryEnhancer::search(const QString& query, int topK) const {
    if (!m_embedder || !m_vectorDB || m_entries.isEmpty()) return {};

    // ✅ 预建 id → entry 索引，避免内层循环
    QHash<QString, const MemoryEntry*> idIndex;
    idIndex.reserve(m_entries.size());
    for (const auto& e : m_entries)
        idIndex.insert(e.id, &e);

    const QVector<SearchResult> results =
        m_vectorDB->search(m_embedder->embed(query), topK);

    QVector<MemoryEntry> matched;
    matched.reserve(results.size());
    for (const auto& sr : results) {
        const QString& docPath = sr.chunk.documentPath;
        if (!docPath.startsWith("memory:")) continue;
        const QString entryId = docPath.mid(7);
        if (const MemoryEntry* e = idIndex.value(entryId, nullptr)) {
            MemoryEntry scored = *e;
            scored.confidence = sr.similarity;
            matched.append(scored);
        }
    }
    return matched;
}
```

### 4.4 版本号统一从 CMake 注入

```cpp
// CMakeLists.txt — 已有 project(LocalAIAssistant VERSION 1.1.1)
// 添加：
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
)

// cmake/version.h.in（新建）
#pragma once
#define APP_VERSION "@PROJECT_VERSION@"
#define APP_VERSION_MAJOR @PROJECT_VERSION_MAJOR@
#define APP_VERSION_MINOR @PROJECT_VERSION_MINOR@
#define APP_VERSION_PATCH @PROJECT_VERSION_PATCH@

// cli/cli_application.cpp — 替换硬编码
#include "version.h"
QCoreApplication::setApplicationVersion(APP_VERSION); // ✅ 统一来源
```

### 4.5 expandPath 环境变量展开加迭代上限

```cpp
// tasks/commandexecutor.cpp
static QRegularExpression winEnvVar(QStringLiteral("%([A-Za-z_][A-Za-z0-9_]*)%"));
int limit = 20; // 防循环引用
QRegularExpressionMatch m;
while (limit-- > 0 && (m = winEnvVar.match(expanded)).hasMatch()) {
    const QString val = QProcessEnvironment::systemEnvironment().value(m.captured(1));
    if (val.isEmpty()) break;
    expanded.replace(m.capturedStart(), m.capturedLength(), val);
}
// Unix $VAR 同理加 limit
```

### 4.6 build.sh nproc 跨平台修复

```bash
# scripts/build.sh — 替换 $(nproc)
JOBS=$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)
cmake --build . --parallel "$JOBS"
```

### 4.7 Windows 语音输入支持

```cpp
// ui/mainwindow.cpp — 构造函数中
m_inputLine->setAttribute(Qt::WA_InputMethodEnabled, true);
m_inputLine->setInputMethodHints(Qt::ImhNone);

// ui/main.cpp — Windows 下不主动 AllocConsole，仅 AttachConsole
// attachDebugConsole() 改为只在 AttachConsole 成功时重定向
// 避免新建控制台窗口抢走输入焦点
void attachDebugConsole() {
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
    // ❌ 移除 else { AllocConsole(); ... } 分支，改为写日志文件
}
```

### 4.8 VectorDB SQLite 连接名唯一化

```cpp
// knowledge/vectordb.cpp
bool VectorDB::initStorage(const QString& dir) {
    // ✅ 用目录路径 hash 确保连接名唯一
    const QString connName = QString("vectordb_%1")
        .arg(QString::fromLatin1(
            QCryptographicHash::hash(dir.toUtf8(), QCryptographicHash::Md5).toHex()));
    m_connName = connName; // 存为成员变量，后续使用

    if (QSqlDatabase::contains(connName))
        QSqlDatabase::removeDatabase(connName);
    // ...
}
```

---

## 5. 【架构 & 模块化重构方向】

### 5.1 异步调度层（当前缺失）

Agent 任务执行、知识库导入、向量检索需要统一的异步调度管理：

```
src/async/                     ← 新增
├── taskrunner.h/.cpp          ← 封装 QThreadPool + QFutureWatcher
└── asyncresult.h              ← 统一 Result<T, E> 类型
```

### 5.2 mainwindow.cpp 文件拆分（ROADMAP 已规划）

按 ROADMAP Phase 2.2 方案执行：

| 新文件 | 约行数 | 内容 |
|---|---|---|
| `mainwindow.cpp` | 450 | 构造、setupUI、事件路由 |
| `mainwindow_sessions.cpp` | 450 | session 渲染、列表、CRUD |
| `mainwindow_network.cpp` | 420 | 发送、流式、错误、重试 |
| `mainwindow_search.cpp` | 250 | 搜索栏、导航、高亮 |
| `mainwindow_agents.cpp` | 380 | Agent 回调、文件显示、命令输出 |

### 5.3 Provider 策略模式完善

```cpp
// ✅ 将流式解析从基类剥离，每个 Provider 独立管理
class StreamParser {
public:
    virtual QString parse(const QByteArray& chunk) = 0;
    virtual void reset() {}
};
class SseStreamParser  : public StreamParser {}; // OpenAI / Anthropic
class NdjsonStreamParser : public StreamParser {}; // Ollama
// ApiProvider 持有 StreamParser*，sendChatRequest 根据类型选择
```

### 5.4 知识库检索异步化

```cpp
// knowledge/knowledgebase.h
QFuture<QString> generateContextAsync(const QString& query, int topK = 5) const;

// 使用：
auto future = KnowledgeBase::instance()->generateContextAsync(userMessage);
QFutureWatcher<QString>* w = new QFutureWatcher<QString>(this);
connect(w, &QFutureWatcher<QString>::finished, this, [this, w]() {
    m_networkManager->setKnowledgeContext(w->result());
    // 然后发送 HTTP 请求
});
w->setFuture(future);
```

---

## 6. 【性能提速优化方案】

| 问题 | 当前实现 | 优化方案 | 预期收益 |
|---|---|---|---|
| 每条消息写磁盘 | 同步 `saveSessionsToFile` | 500ms debounce（FIX-08） | IO 减少 90%+ |
| Agent 阻塞 UI | `QEventLoop` 嵌套 | `QtConcurrent::run`（FIX-01） | UI 完全不冻结 |
| 流式重渲染 | 每 chunk 触发 `setHtml` | 50ms 批量更新 + 增量追加 | 重绘减少 80%+ |
| 全量 Session 序列化 | 每次写所有 session JSON | 退出时全量写 + 运行时只追加增量 | 大历史时节省 80% |
| 记忆检索 O(n²) | 嵌套线性扫描 | `QHash` 索引（FIX / 4.3） | 线性降为常数 |
| 知识库同步阻塞 | `search` 同步调用 | `QFuture` 异步（§5.4） | 消除卡顿感 |
| MarkdownRenderer 无缓存 | 每次 `renderCurrentSession` 重渲全部消息 | 按 message hash 缓存 HTML | 历史长对话渲染 O(1) |

**流式批量更新示例：**

```cpp
// ui/mainwindow.h
QTimer* m_streamFlushTimer = nullptr;
QString m_pendingChunks;

// ui/mainwindow.cpp
m_streamFlushTimer = new QTimer(this);
m_streamFlushTimer->setInterval(50);
connect(m_streamFlushTimer, &QTimer::timeout, this, [this]() {
    if (!m_pendingChunks.isEmpty()) {
        appendChunkToDisplay(m_pendingChunks); // 增量追加，不 setHtml
        m_pendingChunks.clear();
    }
});

void MainWindow::onStreamChunkReceived(const QString& chunk) {
    m_pendingChunks += chunk;
    if (!m_streamFlushTimer->isActive())
        m_streamFlushTimer->start();
}
```

---

## 7. 【测试覆盖提升方案】

### 7.1 现有测试评估

| 测试文件 | 覆盖质量 | 关键缺失 |
|---|---|---|
| `test_safetychecker.cpp` | ✅ 优秀，含命令注入/Windows 特有攻击 | 路径穿越 `../../` 回归用例 |
| `test_commandexecutor.cpp` | ✅ 良好，集成测试含临时目录 | 超时场景、cancel() 测试 |
| `test_sessionmanager.cpp` | 🟡 一般 | **删除当前 session 后 ID 切换验证缺失** |
| `test_markdownrenderer.cpp` | 🟡 一般 | **末尾列表/表格未闭合回归用例缺失** |
| `test_agentloop.cpp` | 🔴 弱，仅测信号存在性 | 完整 start→execute→feedback 循环 |
| `test_apptheme.cpp` | ✅ 良好 | — |
| `test_stylesheetmanager.cpp` | ✅ 良好 | — |
| `test_fileparser.cpp` | ✅ 良好 | — |

### 7.2 高优先级补充测试

```cpp
// ① test_sessionmanager.cpp — BUG-04 回归
void testRemoveCurrentSession_SwitchesId() {
    auto* sm = SessionManager::instance();
    sm->createNewSession("A");
    sm->createNewSession("B");
    const QString currentId = sm->currentSessionId();
    sm->removeSession(currentId);

    QVERIFY(!sm->currentSessionId().isEmpty());
    QVERIFY(sm->currentSessionId() != currentId);
    QVERIFY(sm->allSessions().contains(sm->currentSessionId()));
    // 验证 currentSession() 不崩溃
    QVERIFY(!sm->currentSession().id.isEmpty());
}

// ② test_markdownrenderer.cpp — BUG-10 回归
void testUnclosedList_AtEnd() {
    const QString html = MarkdownRenderer::toHtml("- item1\n- item2"); // 无末尾空行
    QVERIFY2(html.contains("</ul>"), "末尾列表必须闭合");
    QCOMPARE(html.count("<ul"), html.count("</ul>"));
}
void testUnclosedTable_AtEnd() {
    const QString md = "| A | B |\n|---|---|\n| 1 | 2 |"; // 无末尾空行
    const QString html = MarkdownRenderer::toHtml(md);
    QVERIFY2(html.contains("</table>"), "末尾表格必须闭合");
}

// ③ test_taskengine.cpp — 核心 JSON 解析（新建）
void testParsePlan_ValidJson() {
    const QString response =
        R"([TASK_PLAN]{"description":"test","operations":[
            {"type":"shell_command","command":"echo hi","description":"say hi","timeout":5}
        ]}[/TASK_PLAN])";
    const OperationPlan plan =
        TaskEngine::instance()->parsePlanFromAIResponse(response);
    QVERIFY(!plan.isEmpty());
    QCOMPARE(plan.operations.size(), 1);
    QCOMPARE(plan.operations[0].command, QString("echo hi"));
}
void testParsePlan_WithCodeFences() {
    const QString response =
        "[TASK_PLAN]\n```json\n{\"description\":\"x\","
        "\"operations\":[]}\n```\n[/TASK_PLAN]";
    const OperationPlan plan =
        TaskEngine::instance()->parsePlanFromAIResponse(response);
    QVERIFY(plan.isEmpty()); // operations 为空视为空 plan
}
void testParsePlan_MalformedJson() {
    const OperationPlan plan =
        TaskEngine::instance()->parsePlanFromAIResponse("[TASK_PLAN]{bad}[/TASK_PLAN]");
    QVERIFY(plan.isEmpty());
}

// ④ test_safetychecker.cpp — 路径穿越补充
void testPathTraversal_DotDot() {
    SafetyChecker sc;
    sc.setAllowedPaths({QDir::homePath() + "/Documents"});
    ShellOperation op;
    op.type = ShellOperation::WriteFile;
    op.target = QDir::homePath() + "/Documents/../../etc/passwd";
    QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
}
```

---

## 8. 【GitHub 开源规范升级方案】

### 8.1 仍缺失的规范文件（建议添加）

```
.github/
├── ISSUE_TEMPLATE/
│   ├── bug_report.md          ← 含平台/API类型/复现步骤模板
│   └── feature_request.md
├── PULL_REQUEST_TEMPLATE.md   ← 含变更描述/测试确认/平台验证复选框
CONTRIBUTING.md                ← 开发环境搭建/提交规范/PR流程
CHANGELOG.md                   ← 语义化版本变更记录
```

### 8.2 CI 优化建议（当前已很完整，微调）

```yaml
# .github/workflows/build.yml — 当前不足之处

# 1. Linux/macOS 并行数统一：
run: cmake --build build --parallel  # CMake 3.12+ 自动检测，无需 $(nproc)

# 2. Qt 版本三平台统一（当前 Linux/macOS 用系统包，版本不可控）
# 建议 Linux/macOS 也改用 jurplel/install-qt-action

# 3. 补充 format-check 作为 PR 必要门控
# format-check.yml 中加 if: github.event_name == 'pull_request'
# 并在 branch protection 中设为 required status
```

### 8.3 代码签名建议（影响用户首次启动体验）

目前 Release Notes 已说明未签名，用户需手动授权。长期建议：
- **macOS**：申请 Apple Developer 账号（$99/年），使用 `codesign + notarytool` 实现 Notarization
- **Windows**：申请 EV Code Signing 证书，消除 SmartScreen 警告
- **过渡方案**：在 README 首屏和 Release Notes 中加醒目提示，降低用户疑虑

---

## 9. 【长期技术迭代路线】

### Phase 1（0–1 个月）：稳定性修复

- ✅ FIX-01 AgentLoop 异步化（消除 UI 冻结）
- ✅ FIX-02 消除双重渲染（流式丝滑）
- ✅ FIX-03 removeSession 悬空 ID
- ✅ FIX-04 Anthropic SSE currentEvent 跨包
- ✅ FIX-05 Embedder ONNX 内存泄漏
- ✅ FIX-06 KnowledgeBase 线程安全
- ✅ FIX-07 Markdown 末尾未关闭
- ✅ FIX-08 SessionManager debounce
- ✅ FIX-09 SafetyChecker 路径穿越
- ✅ FIX-10 Ollama NDJSON 独立解析
- ✅ Windows 语音输入兼容（4.7）
- ✅ 版本号统一注入（4.4）
- ✅ 补充 BUG-03/BUG-10/TaskEngine/SafetyChecker 回归测试

### Phase 2（1–3 个月）：架构重构

- mainwindow.cpp 拆分（按 ROADMAP Phase 2.2）
- 引入统一异步调度层 `src/async/`
- Provider 策略模式重构（OllamaProvider 独立 StreamParser）
- 知识库检索异步化
- MemoryEnhancer O(n²) 优化
- 所有单例迁移到 Meyer's Singleton
- 补全 GitHub 规范文件（Issue 模板 / PR 模板 / CONTRIBUTING）

### Phase 3（3–6 个月）：工程化完善

- 三平台统一用 `install-qt-action`，Qt 版本锁定
- 版本统一注入 CMake → 所有代码
- 流式渲染批量更新 + MarkdownRenderer 缓存
- CI format-check 设为 PR 必要门控
- 代码签名（macOS Notarization 为优先）
- 完善 CHANGELOG.md 记录每次版本变化

### Phase 4（6 个月+）：生态扩展

- 插件化 Provider（用户可扩展新 API 后端）
- WebSocket 协议支持（某些 API 后端）
- 配置热重载（无需重启更改 API）
- MathBERT 或支持 LaTeX 的嵌入模型（改善数学 PDF 检索）
- clang-tidy 集成到 CI

---

> **附：本报告与前几轮对话中已发现问题的对应关系**
>
> 所有前两轮发现的 Bug（BUG-01 至 BUG-09）均已保留并整合进本报告第 2-3 节；本轮新增 5 个（Embedder 泄漏、KnowledgeBase 线程安全、版本号不一致、MemoryEnhancer O(n²)、VectorDB 连接名冲突）；前轮对 LICENSE/README/CI/测试的错误判断已在报告中予以修正，评级从"一般"上调为"良好偏优"。