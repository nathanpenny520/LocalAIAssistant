# 本地AI助手文档

#### 作者：聂磐-2025012164- np25@maills.tsinghua.edu.cn - 17207051990

## 一、摘要

本项目实现了一个基于 Qt 6 框架和 C++17 标准的跨平台本地AI助手桌面应用程序，版本号为 1.1.0。系统采用模块化架构设计，包含核心通信模块（LocalAIAssistantCore）、任务执行模块（TaskModule）、知识库模块（KnowledgeModule）和AI女友交互模块（GirlfriendModule）四大功能单元，支持 GUI 图形界面与 CLI 命令行双模式运行。

核心技术栈涵盖：Qt 6 框架（Widgets、Network、Multimedia、WebSockets、Sql、Concurrent）、CMake 构建系统、ONNX Runtime 嵌入推理引擎、HNSW 向量检索算法、以及讯飞开放平台 WebSocket API。系统实现了多 AI 服务提供商适配（OpenAI、Ollama、llama.cpp、Anthropic Claude）、SSE 流式输出、三级安全防护架构、Agent 迭代循环执行、语义向量检索、以及语音交互等功能特性。

项目通过 GitHub Actions CI/CD 管道实现跨平台自动化构建与发布，支持 macOS（DMG）、Windows（ZIP）和 Linux（tar.gz）三种分发格式。代码遵循严格的工程规范：文件尽量不超过 500 行、函数尽量不超过 50 行、Qt 信号槽机制进行跨对象通信、Qt Test 框架进行单元测试覆盖。项目代码量约 25,000 行（源码 19,335 行、测试 1,901 行、脚本 2,645 行、构建配置 1,138 行），测试覆盖核心模块包括 SafetyChecker、AgentLoop、CommandExecutor、SessionManager 等关键组件。

**关键词**：本地AI助手、Qt 6、C++17、跨平台应用、模块化架构、知识库、向量检索、Agent系统、语音交互

---

## 二、项目介绍

### 2.1 项目背景与目标

随着大语言模型（LLM）技术的快速发展，AI 助手应用已成为软件开发和日常办公的重要工具。然而，现有主流AI助手产品（如 ChatGPT 网页版、Claude 等）存在以下局限性：

1. **隐私安全风险**：云端服务要求用户上传敏感数据，存在数据泄露风险
2. **网络依赖性强**：离线环境下无法使用，影响工作效率
3. **文件操作受限**：无法直接执行本地文件操作，需要用户手动处理
4. **个性化不足**：缺乏情感交互和长期记忆能力

本项目旨在解决上述问题，构建一个具备以下特性的本地AI助手系统：

- **本地部署优先**：支持 Ollama、llama.cpp 等本地推理框架，保护用户隐私
- **真实文件操作**：通过 Agent 循环实现自主文件操作、Shell 命令执行
- **知识库管理**：支持文档导入、语义检索，实现个人知识管理
- **情感化交互**：AI 女友模块提供语音交互、情绪系统和长期记忆
- **跨平台兼容**：macOS、Windows、Linux 三平台统一代码库

### 2.2 系统架构设计

系统采用分层模块化架构，遵循高内聚低耦合原则。整体架构图如下：

```
┌─────────────────────────────────────────────────────────────────┐
│                    Executables Layer                             │
│  ┌─────────────────────┐    ┌─────────────────────────────────┐ │
│  │   LocalAIAssistant  │    │    LocalAIAssistant-CLI        │ │
│  │     (GUI .app)      │    │      (CLI executable)          │ │
│  └─────────────────────┘    └─────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────────┐
│                    Libraries Layer                               │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │              GirlfriendModule (src/girlfriend/)              ││
│  │  GirlfriendWindow | AvatarWidget | VoiceManager | Memory    ││
│  └─────────────────────────────────────────────────────────────┘│
│                              │                                   │
│  ┌──────────────────────┐    │    ┌────────────────────────────┐│
│  │    KnowledgeModule   │    │    │       TaskModule           ││
│  │  (src/knowledge/)    │    │    │     (src/tasks/)           ││
│  │ KnowledgeBase        │    │    │ TaskEngine | AgentLoop     ││
│  │ Embedder | VectorDB  │    │    │ SafetyChecker | Executor   ││
│  │ TextChunker | DocImp │    │    │ OperationPlan | Undo       ││
│  └──────────────────────┘    │    └────────────────────────────┘│
│                              │                                   │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │              LocalAIAssistantCore (src/core/)                ││
│  │  NetworkManager | SessionManager | FileManager | PromptMgr  ││
│  │  ApiProvider (OpenAI/Ollama/LlamaCpp/Anthropic)             ││
│  └─────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────────┐
│                    Foundation Layer                              │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────┐│
│  │ Qt6::    │ │ Qt6::    │ │ Qt6::    │ │ Qt6::    │ │ ONNX   ││
│  │ Widgets  │ │ Network  │ │ Multi    │ │ WebSock  │ │ Runtime││
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘ └────────┘│
└─────────────────────────────────────────────────────────────────┘
```

### 2.3 模块职责划分

系统由 **5 个 CMake 库** 和 **2 个可执行文件** 组成：

#### 库模块

| 库名 | 源码路径 | 行数 | 核心职责 | 关键类 |
|------|----------|------|----------|--------|
| **FileParser** | `src/parsers/` | 301 | TXT/PDF/DOCX/图片解析 | `FileParser` |
| **LocalAIAssistantCore** | `src/core/`, `src/prompts/` | 1,920+545 | 网络通信、会话管理、文件处理、Prompt 管理 | `NetworkManager`, `SessionManager`, `ApiProvider` |
| **TaskModule** | `src/tasks/` | 2,448 | 任务解析、安全检查、命令执行、撤销机制 | `TaskEngine`, `AgentLoop`, `SafetyChecker`, `CommandExecutor` |
| **KnowledgeModule** | `src/knowledge/` | 2,187 | 文档切分、向量嵌入、语义检索、记忆增强 | `KnowledgeBase`, `Embedder`, `VectorDB`, `TextChunker` |
| **GirlfriendModule** | `src/girlfriend/` | 6,007 | AI 女友交互、语音管理、情绪系统、记忆持久 | `GirlfriendWindow`, `VoiceManager`, `PersonalityEngine` |

#### 可执行文件

| 可执行文件 | 源码路径 | 行数 | 链接库 |
|------------|----------|------|--------|
| **LocalAIAssistant** (GUI) | `src/ui/` | 4,514 | Core + TaskModule + KnowledgeModule + GirlfriendModule |
| **LocalAIAssistant-CLI** | `src/cli/` | 1,413 | Core + TaskModule + KnowledgeModule |

### 2.4 技术栈选型分析

| 技术 | 选型理由 |
|------|----------|
| **Qt 6** | 跨平台 GUI 框架，原生性能优秀，信号槽机制优雅，多媒体支持完善 |
| **C++17** | 高性能、内存安全、现代特性（std::optional、结构化绑定、if constexpr） |
| **CMake** | 跨平台构建系统，支持复杂依赖管理，生成 compile_commands.json |
| **ONNX Runtime** | 跨平台推理引擎，支持 all-MiniLM-L6-v2 嵌入模型，无需 GPU |
| **hnswlib** | Header-only HNSW 算法，高性能近似最近邻搜索，适合中小规模向量库 |
| **Qt Test** | Qt 官方测试框架，与 Qt 组件无缝集成，支持信号槽测试 |

---

## 三、应用介绍

### 3.1 核心功能模块

#### 3.1.1 多 AI 服务提供商适配

系统采用 Provider 模式实现多 API 适配，抽象基类定义统一接口：

```cpp
class ApiProvider {
public:
    virtual QString endpointPath() const = 0;
    virtual QJsonArray buildMessagesArray(const QVector<ChatMessage>&) const = 0;
    virtual QString extractDeltaFromSSE(const QByteArray&) = 0;
    virtual QString extractContentFromResponse(const QByteArray&) = 0;
};
```

各提供商实现协议特定逻辑：

| Provider | API 类型 | 协议特点 |
|----------|----------|----------|
| `OpenAIProvider` | OpenAI 兼容 | `/v1/chat/completions`，SSE `data: {...}` 格式 |
| `OllamaProvider` | Ollama 本地 | `/api/chat`，JSON 流式响应 |
| `LlamaCppProvider` | llama.cpp | `/completion`，自定义 SSE 格式 |
| `AnthropicProvider` | Claude API | `/v1/messages`，非 SSE 流式，需 X-API-Key |

#### 3.1.2 流式输出实现

采用 SSE (Server-Sent Events) 协议实现实时 token 显示：

```cpp
void NetworkManager::onStreamReadyRead() {
    QByteArray data = m_streamReply->readAll();
    QStringList lines = QString::fromUtf8(data).split('\n');
    for (const QString& line : lines) {
        if (line.startsWith("data: ")) {
            QString jsonStr = line.mid(6);
            if (jsonStr == "[DONE]") continue;
            QString delta = m_provider->extractDeltaFromSSE(jsonStr.toUtf8());
            if (!delta.isEmpty()) {
                emit streamTokenReceived(delta);
            }
        }
    }
}
```

特性：
- Thinking block 过滤：`<thinking>...</thinking>` 自动移除
- Buffer 累积处理：处理分包到达的不完整 SSE 行
- 超时保护：30 秒无数据自动终止连接

#### 3.1.3 会话管理与会话持久化

会话数据采用 JSON 格式持久化，存储路径为 `<AppDataLocation>/sessions/`：

```
sessions/
├── sessions.json         # 会话元数据列表
└── session_<uuid>.json   # 单会话完整对话历史
```

核心数据模型定义 (`src/core/datamodels.h`)：

```cpp
struct ChatSession {
    QString id;                      // UUID 标识
    QString title;                   // 会话标题
    QVector<ChatMessage> messages;   // 对话历史
    bool pinned = false;             // 固定标记
    bool autoNamed = false;          // 自动命名标记
};

struct ChatMessage {
    QString role;                    // "user" | "assistant" | "system"
    QString content;                 // 消息文本
    QVector<FileAttachment> attachments;  // 文件附件
    bool isAgentLoopInjected = false;     // Agent 循环注入标记
};
```

上下文窗口管理通过 `computeContextStartIndex()` 实现 Token 预算控制，超出 `maxContext` 限制时自动截断早期消息。

### 3.2 任务执行模块

#### 3.2.1 Agent 迭代循环架构

AgentLoop 实现状态机驱动的迭代执行模式，定义于 `src/tasks/agentloop.h`：

```cpp
enum State { Idle, Running, AwaitingUserConfirm, Completed, Stopped, MaxIterations };
```

状态转换图：

```
┌──────────┐    ┌──────────┐    ┌────────────────────┐
│   Idle   │───▶│ Running  │───▶│ AwaitingUserConfirm│
└──────────┘    └──────────┘    └────────────────────┘
                    │                       │
        ┌───────────┼───────────┐           │ /confirm
        │           │           │           │
        ▼           ▼           ▼           ▼
   ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐
   │Completed │ │  Stopped │ │MaxIterat │ │ Running  │
   └──────────┘ └──────────┘ └──────────┘ └──────────┘
   [TASK_      /cancel       达10次       继续迭代
   COMPLETE]   或用户中断    迭代上限
```

**六种状态说明**：

| 状态 | 说明 | 触发条件 |
|------|------|----------|
| `Idle` | 初始状态 | 程序启动或任务完成 |
| `Running` | 等待AI响应 | 发送请求后 |
| `AwaitingUserConfirm` | 等待用户确认 | 收到需要确认的计划 |
| `Completed` | 任务成功完成 | 收到 `[TASK_COMPLETE]` |
| `Stopped` | 用户中断 | `/cancel` 命令 |
| `MaxIterations` | 达到迭代上限 | 10次迭代后自动终止 |

迭代循环流程：

1. 用户发送任务请求
2. AI 生成 `[TASK_PLAN]...[/TASK_PLAN]` 格式的 JSON 操作计划
3. TaskEngine 解析 JSON 提取操作序列
4. SafetyChecker 执行三级安全验证
5. 用户确认（CLI `/confirm` / GUI 对话框）
6. CommandExecutor 执行操作
7. 执行结果注入 `[ITERATION_FEEDBACK]`
8. AI 分析结果生成新计划或 `[TASK_COMPLETE]`
9. 最多 10 次迭代后自动终止

#### 3.2.2 三级安全防护架构

SafetyChecker 实现分层安全验证：

| 安全层级 | 验证规则 | 处理方式 |
|----------|----------|----------|
| **Tier 1 (Blocked)** | 命令注入、权限提升、磁盘破坏 | 永久拦截，不可绕过 |
| **Tier 2 (NeedsConfirmation)** | 系统路径、白名单外路径 | 用户逐项确认 |
| **Tier 3 (Approved)** | `~/`、`/tmp`、Desktop、Documents | 自动批准执行 |

Tier 1 检测的危险模式（30+ 种）：

```cpp
static const QVector<QString> kBlockedPatterns = {
    "sudo", "su", "eval ", "`", "$(", "rm -rf /", 
    "mkfs", "dd if=", "chmod 777", "chown root",
    "systemctl stop", "iptables -F", "nc -l",
    "curl | bash", "wget | sh", "/dev/sda",
    // ... 共 30+ 种模式
};
```

路径白名单机制：

```cpp
static const QVector<QString> kApprovedPaths = {
    QDir::homePath(),           // ~/
    QDir::tempPath(),           // /tmp
    QDir::homePath() + "/Desktop",
    QDir::homePath() + "/Documents",
    // 用户批准路径动态添加
};
```

#### 3.2.3 原生文件操作与 Shell 执行

CommandExecutor 支持两类操作：

**原生文件操作（Qt API）**：
- `create_dir`: `QDir::mkpath()`
- `write_file`: `QFile::write()`
- `move_file`: `QFile::rename()`
- `copy_file`: `QFile::copy()`
- `delete_file`: `QFile::remove()`
- `search_files`: `QDir::entryList()` + 正则过滤

**Shell 命令执行**：
- 自动检测可用 Shell：Windows (pwsh→powershell→cmd)，Unix ($SHELL→zsh→bash→sh)
- 沙箱化执行：通过 `QProcess` 独立进程
- 输出捕获：stdout/stderr 合并返回

#### 3.2.4 操作撤销机制

OperationUndo 维护操作栈，支持撤销最近执行的文件操作：

```cpp
struct UndoRecord {
    OperationType type;       // WRITE/MOVE/COPY/DELETE
    QString sourcePath;       // 源路径
    QString targetPath;       // 目标路径
    QByteArray content;       // 原文件内容（用于恢复）
    QDateTime timestamp;      // 执行时间
};
```

### 3.3 知识库模块

#### 3.3.1 文档导入流程

DocImporter 支持 TXT/MD/PDF/DOCX 四种格式：

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  文件输入   │───▶│  文本解析   │───▶│  文本切分   │
└─────────────┘    └─────────────┘    └─────────────┘
                                           │
                                           ▼
                    ┌─────────────┐    ┌─────────────┐
                    │  向量存储   │───▶│  向量嵌入   │
                    └─────────────┘    └─────────────┘
```

PDF 解析依赖 Poppler 库 (`page->text()`)，DOCX 解析依赖 libzip + pugixml（解压 ZIP + 解析 XML）。

#### 3.3.2 文本切分策略

TextChunker 实现段落优先切分：

```cpp
QVector<TextChunk> TextChunker::chunk(const QString& text, int maxTokens) {
    // 1. 段落切分：\n\n+ 分隔
    QVector<QString> paragraphs = text.split(QRegularExpression("\\n\\n+"));
    
    // 2. 段落内句子切分（fallback）
    // 3. Token 估算：中文 1 token/字，英文 0.25 token/word
    // 4. 合并小块避免碎片化
}
```

#### 3.3.3 向量嵌入与检索

Embedder 采用 ONNX Runtime 加载 all-MiniLM-L6-v2 模型：

```cpp
QVector<float> Embedder::embed(const QString& text) {
    // 1. 预处理：中文字符处理、标点归一化
    // 2. WordPiece Tokenization：[CLS] + tokens + [SEP]
    // 3. ONNX Inference：input_ids, attention_mask
    // 4. L2 Normalization：cosine similarity 预处理
}
```

VectorDB 实现两层存储策略：
- **HNSW 索引**：hnswlib 实现，适合大规模向量
- **Flat 存储**：小规模数据直接内存搜索，SQLite 元数据管理

检索流程：

```cpp
QVector<SearchResult> KnowledgeBase::search(const QString& query, int topK) {
    QVector<float> queryVector = m_embedder->embed(query);
    QVector<int> ids = m_vectorDb->search(queryVector, topK);
    return m_chunkDb->getChunks(ids);  // SQLite 元数据查询
}
```

#### 3.3.4 记忆增强机制

MemoryEnhancer 实现跨会话记忆提取与注入：

- **记忆提取**：从对话中识别用户关键信息（偏好、习惯、重要事件）
- **语义检索**：根据当前查询检索相关历史记忆
- **上下文注入**：将检索结果作为系统提示补充

### 3.4 AI 女友模块

#### 3.4.1 情绪系统架构

PersonalityEngine 实现 14 种情绪识别与心情计算：

| 情绪类型 | 触发关键词示例 |
|----------|----------------|
| happy | "开心", "高兴", "哈哈", "太棒了" |
| shy | "害羞", "不好意思", "脸红" |
| love | "喜欢你", "爱你", "亲爱的" |
| angry | "生气", "讨厌", "烦人" |
| sad | "难过", "伤心", "不开心" |
| crying | "哭", "眼泪", "想哭" |
| afraid | "害怕", "担心", "恐惧" |
| worried | "焦虑", "不安", "紧张" |

心情值计算：

```cpp
double PersonalityEngine::calculateMood(const QString& response) {
    Emotion emotion = detectEmotion(response);
    double baseMood = emotionToMoodValue(emotion);
    double influence = m_moodInfluenceLevel;  // Low/Medium/High
    return m_currentMood + (baseMood - m_currentMood) * influence;
}
```

#### 3.4.2 语音交互系统

VoiceManager 通过讯飞 WebSocket API 实现 ASR/TTS：

**ASR（语音识别）流程**：
```
QAudioSource → 音频帧 → WebSocket 分块发送 → 讯飞识别 → 文本返回
```

**TTS（语音合成）流程**：
```
文本输入 → WebSocket 请求 → 讯飞合成 → 音频流 → QAudioOutput 播放
```

认证机制：HMAC-SHA256 签名生成 WebSocket URL：

```cpp
QString VoiceManager::generateAuthUrl(const QString& baseUrl) {
    QString date = QDateTime::currentDateTimeUtc().toString("ddd, dd MMM yyyy HH:mm:ss UTC");
    QString signatureOrigin = "host: " + host + "\ndate: " + date + "\nGET " + path;
    QString signature = QMessageAuthenticationCode::hash(
        signatureOrigin, m_apiSecret, QCryptographicHash::Sha256).toBase64();
    QString authorization = "api_key=\"" + m_apiKey + "\", algorithm=\"hmac-sha256\", "
                            "headers=\"host date request-line\", signature=\"" + signature + "\"";
    return baseUrl + "?authorization=" + authorization.toBase64() + "&date=" + date.toPercentEncoding();
}
```

#### 3.4.3 记忆持久化系统

MemoryManager 实现长期记忆存储：

- **记忆标记**：AI 通过 `[memory:category|content]` 标记输出记忆内容
- **分类存储**：按类别（偏好、事件、关系）组织记忆
- **持久化**：存储为 `memory.md` 文件

#### 3.4.4 头像等级系统

| 等级 | 资源类型 | 表情数量 | 特点 |
|------|----------|----------|------|
| Level 1 (Belle) | PNG 静态图片 | 14 种 | 经典风格，适合办公环境 |
| Level 2 (Hot) | PNG 静态图片 | 14 种 | 更具吸引力，适合个人使用 |
| Level 3 (Hotter) | MP4 动态视频 | 14 种 | 动画效果，沉浸体验 |

### 3.5 用户界面设计

#### 3.5.1 主题系统

AppTheme 定义约 40 个语义颜色 Token：

```cpp
enum ColorToken {
    // Background
    BackgroundPrimary, BackgroundSecondary, BackgroundCard,
    // Text
    TextPrimary, TextSecondary, TextAccent,
    // Interactive
    ButtonPrimary, ButtonSecondary, ButtonHover,
    // Status
    Success, Warning, Error, Info,
    // ... 共 40 个
};
```

StyleSheetManager 根据当前主题生成 QSS：

```cpp
QString StyleSheetManager::generateStyleSheet(ThemeType theme) {
    AppTheme& t = AppTheme::instance();
    t.setTheme(theme);
    return QString(
        "QWidget { background: %1; color: %2; }"
        "QPushButton { background: %3; border-radius: 4px; }"
    ).arg(t.color(BackgroundPrimary), t.color(TextPrimary), t.color(ButtonPrimary));
}
```

#### 3.5.2 Markdown 渲染

MarkdownRenderer 实现主题感知的 Markdown 转 HTML：

```cpp
QString MarkdownRenderer::toHtml(const QString& markdown) {
    // 1. 解析 Markdown 语法
    // 2. 应用主题 CSS 类名
    // 3. 处理代码块语法高亮
    // 4. 返回 HTML 字符串供 QTextBrowser 显示
}
```

#### 3.5.3 多语言支持

TranslationManager 集成 Qt Linguist：

- 翻译文件：`translations/localaiassistant_zh_CN.ts`
- 动态切换：`QApplication::installTranslator()`
- 支持：简体中文、English

### 3.6 CLI 命令行界面

#### 3.6.1 交互模式

CLIApplication 提供三种运行模式：

| 模式 | 命令 | 功能 |
|------|------|------|
| **交互式聊天** | `./LocalAIAssistant-CLI chat` | 持续对话，支持会话管理 |
| **单次查询** | `./LocalAIAssistant-CLI ask "问题"` | 一次问答，自动退出 |
| **配置管理** | `./LocalAIAssistant-CLI config --show` | 显示/修改配置 |

#### 3.6.2 CLI 命令集

| 命令 | 功能 |
|------|------|
| `/help` | 显示帮助信息 |
| `/new` | 新建会话 |
| `/list` | 列出所有会话 |
| `/switch <id>` | 切换到指定会话 |
| `/delete <id>` | 删除指定会话 |
| `/config` | 显示当前配置 |
| `/file <path>` | 添加文件附件 |
| `/confirm` | 确认执行待定任务计划 |
| `/cancel` | 取消待定任务计划 |
| `/undo` | 撤销上次执行的操作 |
| `/exit` | 退出程序 |

---

## 四、代码验证思路

### 4.1 单元测试框架

采用 Qt Test 框架进行单元测试，测试文件位于 `tests/` 目录：

| 测试文件 | 测试目标 | 测试内容 |
|----------|----------|----------|
| `test_safetychecker.cpp` | SafetyChecker | 危险命令检测、路径验证、白名单逻辑 |
| `test_agentloop.cpp` | AgentLoop | 状态机转换、迭代次数限制、反馈注入 |
| `test_commandexecutor.cpp` | CommandExecutor | 文件操作执行、Shell 命令、输出捕获 |
| `test_sessionmanager.cpp` | SessionManager | 会话 CRUD、JSON 持久化、自动截断 |
| `test_fileparser.cpp` | FileParser | TXT/PDF/DOCX 解析、编码检测 |
| `test_apptheme.cpp` | AppTheme | 颜色 Token 获取、主题切换 |
| `test_stylesheetmanager.cpp` | StyleSheetManager | QSS 生成、Token 替换 |
| `test_markdownrenderer.cpp` | MarkdownRenderer | Markdown 转 HTML、代码块处理 |

### 4.2 测试执行方式

```bash
# 编译测试
cmake --build build --target all

# 运行测试
cd build && ctest --output-on-failure

# 单独运行测试
./build/test_safetychecker
./build/test_agentloop
```

### 4.3 核心模块验证示例

#### 4.3.1 SafetyChecker 测试逻辑

```cpp
void TestSafetyChecker::testBlockedCommands() {
    SafetyChecker checker;
    
    // Tier 1 检测
    QVERIFY(checker.isBlocked("sudo rm -rf /"));
    QVERIFY(checker.isBlocked("eval $(cat file)"));
    QVERIFY(checker.isBlocked("curl | bash"));
    
    // Tier 2 检测
    OperationPlan plan;
    plan.addOperation("write_file", "/etc/passwd", "content");
    QVector<PathViolation> violations = checker.checkPlan(plan);
    QCOMPARE(violations.size(), 1);
    QCOMPARE(violations[0].tier, 2);
    
    // Tier 3 通过
    plan.clear();
    plan.addOperation("write_file", QDir::homePath() + "/test.txt", "content");
    violations = checker.checkPlan(plan);
    QVERIFY(violations.isEmpty());
}
```

#### 4.3.2 AgentLoop 状态机测试

```cpp
void TestAgentLoop::testStateTransitions() {
    AgentLoop loop;
    QSignalSpy spy(&loop, &AgentLoop::stateChanged);
    
    // Idle -> Running
    loop.startIteration(userRequest);
    QCOMPARE(loop.state(), AgentLoop::Running);
    QCOMPARE(spy.count(), 1);
    
    // Running -> AwaitingUserConfirm
    loop.onPlanReceived(plan);
    QCOMPARE(loop.state(), AgentLoop::AwaitingUserConfirm);
    
    // 确认后继续 Running
    loop.confirmPlan();
    QCOMPARE(loop.state(), AgentLoop::Running);
    
    // [TASK_COMPLETE] 后 Completed
    loop.onCompleteReceived();
    QCOMPARE(loop.state(), AgentLoop::Completed);
}
```

### 4.4 CI/CD 验证流程

GitHub Actions 工作流定义于 `.github/workflows/build.yml`：

```yaml
jobs:
  test-linux:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4
      - name: Install dependencies
        run: sudo apt install qt6-base-dev qt6-multimedia-dev ...
      - name: Build
        run: cmake -B build && cmake --build build
      - name: Test
        run: cd build && ctest --output-on-failure
        
  test-macos:
    runs-on: macos-latest
    # ... 类似步骤
    
  test-windows:
    runs-on: windows-2022
    # ... 类似步骤
    
  create-release:
    needs: [test-linux, test-macos, test-windows]
    if: startsWith(github.ref, 'refs/tags/v')
    # ... 打包发布步骤
```

验证流程：
1. 三平台并行编译
2. 自动运行 `ctest`
3. 任一平台测试失败则中止发布
4. 全部通过后打包 DMG/ZIP/tar.gz
5. 自动上传 GitHub Release

### 4.5 代码质量保障

#### 4.5.1 代码规范检查

- **clang-format**：统一代码风格
- **clang-tidy**：静态分析检查
- **文件行数限制**：≤ 500 行（ROADMAP.md 记录违规项）
- **函数行数限制**：≤ 50 行

#### 4.5.2 构建验证

```bash
# 编译验证
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4

# 运行验证
./build/LocalAIAssistant --debug
./build/LocalAIAssistant-CLI chat
```

---

## 五、优缺点分析和提示方向

### 5.1 技术优势分析

| 优势 | 说明 |
|------|------|
| **模块化架构** | 清晰的分层设计，Core → Task → Knowledge → Girlfriend，便于维护和扩展 |
| **跨平台兼容** | 统一代码库支持 macOS/Windows/Linux，CMake 自动处理平台差异 |
| **安全防护完善** | 三级安全架构 + 30+ 危险模式检测，有效防止命令注入和权限提升 |
| **本地化优先** | 支持 Ollama/llama.cpp 本地推理，保护用户隐私 |
| **Agent 循环执行** | 自主迭代完成任务，无需用户手动干预中间步骤 |
| **知识库语义检索** | ONNX 嵌入 + HNSW 检索，实现智能文档检索 |
| **语音交互体验** | 讯飞 WebSocket API 提供高质量 ASR/TTS |
| **CI/CD 自动化** | GitHub Actions 实现三平台自动构建、测试、发布 |

### 5.2 已知技术局限

| 局限 | 原因分析 | 改进方向 |
|------|----------|----------|
| **数学 PDF 检索效果差** | Poppler 文字层缺失公式、WordPiece 词表无 LaTeX、分块策略不适配数学内容 | 引入 OCR 公式识别、数学专用嵌入模型 |
| **Windows ASR 不支持** | Windows Media Foundation 与 Qt 6 QAudioSource 兼容问题 | 重构音频采集模块或使用平台原生 API |
| **Level 3 视频 UI 隐藏** | Qt QVideoWidget 原生窗口渲染与 QWidget 层叠问题 | 使用 QOpenGLWidget 或自定义视频渲染 |
| **GirlfriendWindow 文件过大** | 单文件 1,851 行，违反代码规范 | 按 ROADMAP.md 规划拆分为多个文件 |
| **HNSW 索引无增量更新** | hnswlib 不支持动态插入删除 | 考虑替换为支持增量的向量库 |

### 5.3 性能优化建议

| 优化点 | 当前状态 | 建议 |
|--------|----------|------|
| **嵌入推理延迟** | ONNX CPU 推理约 50ms/query | 启用 ONNX GPU 加速或缓存常用查询向量 |
| **会话持久化 IO** | 每次消息都写入 JSON | 改为定期批量写入或 SQLite 存储 |
| **向量检索内存** | 全量向量内存加载 | 实现分页加载或 mmap 文件映射 |
| **SSE 解析效率** | QString 分割处理 | 改用 QByteArray 原生解析 |

### 5.4 功能扩展建议

| 扩展方向 | 实现思路 |
|----------|----------|
| **多语言嵌入模型** | 集成 multilingual-e5 或 BGE 模型，提升中文检索效果 |
| **RAG 增强** | 将知识库检索结果作为上下文注入，提升回答质量 |
| **工具调用 API** | 支持 OpenAI Function Calling 或 Anthropic Tool Use |
| **多模态支持** | 集成图像理解（LLaVA）、语音理解（Whisper） |
| **协作模式** | 支持多用户共享知识库和会话 |
| **插件系统** | 定义插件 API，允许用户扩展功能 |

### 5.5 代码重构规划

根据 ROADMAP.md 规划，优先重构项：

| 优先级 | 文件 | 当前行数 | 目标 |
|--------|------|----------|------|
| P0 | `girlfriendwindow.cpp` | 1,851 | 拆分为 4-5 个文件 |
| P1 | `mainwindow.cpp` | 1,746 | 提取会话列表、消息列表为独立组件 |
| P2 | `cli_application.cpp` | 1,318 | 提取命令处理、会话管理为独立类 |
| P3 | `commandexecutor.cpp` | 约 200 | 保持当前规模，优化错误处理 |

---

## 六、参考文献和代码

### 6.1 核心技术文档

| 技术 | 文档链接 |
|------|----------|
| Qt 6 Framework | https://doc.qt.io/qt-6/ |
| CMake Build System | https://cmake.org/documentation/ |
| ONNX Runtime | https://onnxruntime.ai/docs/ |
| hnswlib (HNSW Algorithm) | https://github.com/nmslib/hnswlib |
| Poppler PDF Library | https://poppler.freedesktop.org/ |
| 讯飞开放平台 API | https://www.xfyun.cn/doc/ |

### 6.2 AI 服务 API 文档

| API | 文档链接 |
|-----|----------|
| OpenAI API | https://platform.openai.com/docs/api-reference/chat |
| Anthropic Claude API | https://docs.anthropic.com/claude/reference |
| Ollama API | https://github.com/ollama/ollama/blob/main/docs/api.md |
| llama.cpp Server | https://github.com/ggerganov/llama.cpp/tree/master/examples/server |

### 6.3 项目源码结构

```
LocalAIAssistant-main/
├── src/
│   ├── core/                    # 核心模块 (约 800 行)
│   │   ├── networkmanager.cpp/h    # 网络管理器 (268 行)
│   │   ├── sessionmanager.cpp/h    # 会话管理 (101 行 header)
│   │   ├── filemanager.cpp/h       # 文件处理
│   │   ├── promptmanager.cpp/h     # Prompt 管理
│   │   ├── envconfig.cpp/h         # .env 配置加载
│   │   ├── openai_provider.cpp/h   # OpenAI 适配器
│   │   ├── ollama_provider.cpp/h   # Ollama 适配器
│   │   ├── llamacpp_provider.cpp/h # llama.cpp 适配器
│   │   ├── anthropic_provider.cpp/h # Anthropic 适配器
│   │   └── datamodels.h            # 数据结构定义
│   ├── tasks/                   # 任务模块 (约 1,500 行)
│   │   ├── taskengine.cpp/h        # 任务解析引擎
│   │   ├── agentloop.cpp/h         # Agent 循环 (203 行)
│   │   ├── commandexecutor.cpp/h   # 命令执行器
│   │   ├── safetychecker.cpp/h     # 安全检查器 (774 行)
│   │   ├── operationplan.cpp/h     # 操作计划定义
│   │   └── operationundo.cpp/h     # 操作撤销
│   ├── knowledge/               # 知识库模块 (约 1,000 行)
│   │   ├── knowledgebase.cpp/h     # 知识库管理
│   │   ├── embedder.cpp/h          # 向量嵌入
│   │   ├── vectordb.cpp/h          # 向量数据库
│   │   ├── textchunker.cpp/h       # 文本切分
│   │   ├── docimporter.cpp/h       # 文档导入
│   │   └── memoryenhancer.cpp/h    # 记忆增强
│   ├── girlfriend/              # AI 女友模块 (约 2,000 行)
│   │   ├── girlfriendwindow.cpp/h  # 女友窗口 (1,851 行)
│   │   ├── avatarwidget.cpp/h      # 头像组件
│   │   ├── personalityengine.cpp/h # 情绪引擎
│   │   ├── voicemanager.cpp/h      # 语音管理
│   │   ├── memorymanager.cpp/h     # 记忆管理
│   │   ├── girlfriendsettings.cpp/h # 设置管理
│   │   └── girlfriendsessionmanager.cpp/h # 会话管理
│   ├── ui/                      # GUI 界面 (约 2,500 行)
│   │   ├── mainwindow.cpp/h        # 主窗口 (1,746 行)
│   │   ├── settingsdialog.cpp/h    # 设置对话框
│   │   ├── markdownrenderer.cpp/h  # Markdown 渲染
│   │   ├── apptheme.cpp/h          # 主题系统
│   │   ├── stylesheetmanager.cpp/h # 样式管理
│   │   ├── translationmanager.cpp/h # 多语言
│   │   └── operationconfirmdialog.cpp/h # 操作确认对话框
│   ├── cli/                     # CLI 界面 (约 1,300 行)
│   │   └── cli_application.cpp/h   # CLI 应用 (1,318 行)
│   ├── parsers/                 # 文件解析器
│   │   └── fileparser.cpp/h        # TXT/PDF/DOCX 解析
│   └── prompts/                 # Prompt 模板
│       ├── system.md               # 系统提示词
│       ├── task.md                 # 任务提示词
│       └── girlfriend.md           # 女友提示词
├── tests/                       # 单元测试 (约 800 行)
│   ├── test_safetychecker.cpp
│   ├── test_agentloop.cpp
│   ├── test_commandexecutor.cpp
│   ├── test_sessionmanager.cpp
│   ├── test_fileparser.cpp
│   ├── test_apptheme.cpp
│   ├── test_stylesheetmanager.cpp
│   └── test_markdownrenderer.cpp
├── scripts/                     # 构建脚本
│   ├── build.sh                   # 统一构建脚本
│   ├── package.sh                 # 打包脚本
│   ├── setup.sh                   # 初始化脚本
│   └── version.sh                 # 版本提取
├── AIGirlfriend/                # 头像资源
│   ├── level-1-belle/             # Level 1 PNG
│   ├── level-2-hot/               # Level 2 PNG
│   └── level-3-hotter/            # Level 3 MP4
├── third_party/                 # 第三方库
│   └── hnswlib/                   # HNSW 算法 (header-only)
├── translations/                # 国际化
│   └── localaiassistant_zh_CN.ts
├── resources/                   # 资源文件
│   ├── icons/                     # 应用图标
│   └── models/                    # ONNX 模型
├── cmake/                       # CMake 配置
│   ├── Info.plist.in              # macOS GUI bundle
│   └── CLI-Info.plist.in          # macOS CLI bundle
├── .github/workflows/           # CI/CD
│   └── build.yml                  # GitHub Actions
├── CMakeLists.txt               # 主配置 (892 行)
├── CLAUDE.md                    # Claude Code 指南
├── README.md                    # 中文文档
├── README_EN.md                 # 英文文档
├── .env.example                 # 环境变量模板
└── LICENSE                      # MIT 许可证
```

### 6.4 项目仓库地址

- GitHub: https://github.com/nathanpenny520/LocalAIAssistant.git
- Gitee: https://gitee.com/nathanpenny520/LocalAIAssistant.git

### 6.5 关键代码片段索引

| 功能 | 文件路径 | 关键行号 |
|------|----------|----------|
| SSE 流式解析 | `src/core/networkmanager.cpp` | L150-180 |
| Provider 抽象接口 | `src/core/apiprovider.h` | L25-45 |
| Agent 状态机 | `src/tasks/agentloop.cpp` | L30-120 |
| 三级安全检查 | `src/tasks/safetychecker.cpp` | L50-200 |
| 文本切分逻辑 | `src/knowledge/textchunker.cpp` | L40-100 |
| ONNX 嵌入推理 | `src/knowledge/embedder.cpp` | L80-150 |
| 情绪检测算法 | `src/girlfriend/personalityengine.cpp` | L60-120 |
| 讯飞 WebSocket 连接 | `src/girlfriend/voicemanager.cpp` | L100-180 |
| 主题 Token 定义 | `src/ui/apptheme.h` | L20-60 |
| CLI 命令处理 | `src/cli/cli_application.cpp` | L200-400 |

---

## 附录：构建与运行指南

### A.1 快速构建命令

```bash
# 标准构建
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4

# 测试运行
cd build && ctest --output-on-failure

# GUI 运行
./build/LocalAIAssistant

# CLI 运行
./build/LocalAIAssistant-CLI chat
```

### A.2 打包发布命令

```bash
# 构建并打包
./scripts/build.sh build -p

# 单独打包
./scripts/package.sh --build-dir build
```

### A.3 数据存储位置

| 平台 | 路径 |
|------|------|
| macOS | `~/Library/Application Support/LocalAIAssistant/` |
| Windows | `C:\Users\<USER>\AppData\Local\LocalAIAssistant\` |
| Linux | `~/.local/share/LocalAIAssistant/` |

---

### A.4 GitHub Release 首次运行说明

本安装包未进行开发者数字签名，系统会触发安全拦截，属正常现象。以下为各平台首次运行指引：

#### macOS 用户

首次打开程序后若提示无法验证开发者，操作步骤：

1. 双击程序图标；
2. 打开「系统设置」→「隐私与安全性」，下滑找到对应程序；
3. 点击「仍要打开 / 允许」，确认后即可正常启动。

#### Windows 用户

首次运行若弹出 Windows Defender SmartScreen 安全拦截：

1. 点击「更多信息」；
2. 选择「仍要运行」，即可正常打开程序。

#### For macOS Users (English)

If you encounter "unverified developer" warnings when first launching the application:

1. Double-click the application icon;
2. Open "System Settings" → "Privacy & Security", scroll down to find the application;
3. Click "Open Anyway / Allow", then confirm to launch normally.

#### For Windows Users (English)

If Windows Defender SmartScreen displays a security warning:

1. Click "More info";
2. Select "Run anyway" to launch the application.

---

*文档版本：1.1.0 | 更新日期：2026-05-11*