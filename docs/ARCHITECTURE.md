# 项目架构文档

本文档详细说明本地AI助手项目的架构设计、技术选型和实现细节。

## 📋 目录

- [架构概览](#架构概览)
- [目录结构](#目录结构)
- [分层架构](#分层架构)
- [核心组件](#核心组件)
- [数据流](#数据流)
- [技术栈](#技术栈)
- [设计模式](#设计模式)
- [扩展性设计](#扩展性设计)

***

## 架构概览

本地AI助手采用**分层架构**设计，将应用分为三个主要层次：

```
┌─────────────────────────────────────────────┐
│           表示层 (Presentation)              │
│  ┌──────────────┐      ┌────────────────┐  │
│  │   GUI (UI)   │      │   CLI (CLI)    │  │
│  │  Qt Widgets  │      │  Command Line  │  │
│  └──────────────┘      └────────────────┘  │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│         业务逻辑层 (Business Logic)          │
│  ┌──────────────────────────────────────┐  │
│  │     NetworkManager (网络/流式管理)    │  │
│  │     SessionManager (会话管理)         │  │
│  └──────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│           数据层 (Data Layer)                │
│  ┌──────────────────────────────────────┐  │
│  │     DataModels (数据模型)             │  │
│  │     Settings (配置/主题/语言存储)     │  │
│  │     File Storage (文件存储)           │  │
│  └──────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
```

### 架构优势

1. **关注点分离**: 每层专注于特定职责
2. **代码复用**: 核心业务逻辑在 GUI 和 CLI 之间共享
3. **易于测试**: 各层可独立测试
4. **易于维护**: 修改一层不影响其他层
5. **灵活扩展**: 可以轻松添加新的表示层（如 Web UI）

***

## 目录结构

```
sourcecode-ai-assistant/
├── src/                          # 源代码
│   ├── core/                     # 核心业务逻辑层
│   │   ├── networkmanager.*      # 网络请求管理 + SSE 流式响应
│   │   ├── sessionmanager.*      # 会话状态管理
│   │   └── datamodels.h          # 数据模型定义
│   │
│   ├── ui/                       # GUI 表示层
│   │   ├── main.cpp              # GUI 应用入口（含翻译加载）
│   │   ├── mainwindow.*          # 主窗口（含流式显示）
│   │   ├── settingsdialog.*      # 设置对话框（含主题/语言/流式选项）
│   │   └── stylesheetmanager.*   # 样式管理（亮色/暗色/跟随系统）
│   │
│   ├── cli/                      # CLI 表示层
│   │   ├── cli_main.cpp          # CLI 应用入口
│   │   └── cli_application.*     # CLI 应用类（含流式输出）
│   │
│   └── utils/                    # 工具类（预留）
│
├── tests/                        # 测试代码
│   ├── CMakeLists.txt
│   ├── test_sessionmanager.cpp
│   └── test_datamodels.cpp
│
├── scripts/                      # 自动化脚本
│   ├── build.sh                  # 构建脚本（支持多种选项）
│   ├── install.sh                # 安装/卸载脚本
│   └── demo.sh                   # 功能演示脚本
│
├── docs/                         # 项目文档
│   ├── ARCHITECTURE.md           # 架构文档（本文件）
│   ├── CHANGELOG.md              # 更新日志
│   └── CONTRIBUTING.md           # 贡献指南
│
├── translations/                 # 翻译文件
│   ├── localai_zh_CN.ts          # 简体中文翻译
│   └── localai_en.ts             # 英文翻译
│
├── cmake/                        # CMake 配置
│   ├── Info.plist.in             # macOS 应用信息
│   └── config.example.json       # 配置示例
│
├── CMakeLists.txt                # 项目构建配置
├── README.md                     # 项目说明
└── .gitignore                    # Git 忽略规则
```

***

## 分层架构

### 1. 表示层 (Presentation Layer)

#### GUI 层 (`src/ui/`)

**职责**: 提供图形用户界面，处理用户交互

**主要组件**:

- `MainWindow`: 主窗口，包含会话列表、聊天界面和流式输出显示
- `SettingsDialog`: 设置对话框，配置 API 参数、主题、语言、流式输出
- `StyleSheetManager`: 样式管理器，管理亮色/暗色/跟随系统主题

**技术**:

- Qt Widgets: GUI 组件
- Qt Layout: 布局管理
- Qt Style Sheets: 亮色/暗色主题样式
- Markdown 渲染: 消息格式化
- Qt Translator: 多语言翻译加载

**特点**:

- 使用信号槽机制与业务逻辑层通信
- 完全解耦，不包含业务逻辑
- 支持流式输出实时显示
- 支持亮色/暗色主题切换
- 支持多语言切换

#### CLI 层 (`src/cli/`)

**职责**: 提供命令行界面，处理命令解析和执行

**主要组件**:

- `CLIApplication`: CLI 应用主类
  - 命令解析
  - 交互式对话（含流式输出）
  - 单次查询
  - 会话管理
  - 配置管理
  - 流式输出开关 (`/stream` 命令)

**技术**:

- QCommandLineParser: 命令行参数解析
- QTextStream: 文本输入输出
- QTimer: 异步事件处理

**特点**:

- 支持交互式和非交互式模式
- 完整的命令行选项支持（含 `--no-stream`）
- 流式输出逐字显示
- 友好的错误提示

### 2. 业务逻辑层 (Business Logic Layer)

#### NetworkManager (`src/core/networkmanager.*`)

**职责**: 管理 API 请求和响应，支持流式和非流式模式

**主要功能**:

- 发送聊天请求（流式/非流式）
- 处理 SSE (Server-Sent Events) 流式响应
- 处理完整 JSON 响应
- 管理网络连接
- 配置管理（API URL、密钥、模型等）
- 流式输出开关控制

**关键方法**:

```cpp
void sendChatRequest(const QString &userMessage);
void sendChatRequestWithContext(const QVector<ChatMessage> &messages);
void updateSettings(const QString &apiBaseUrl, const QString &apiKey,
                    const QString &modelName, bool isLocalMode);
bool isStreamingEnabled() const;
void setStreamingEnabled(bool enabled);
```

**信号**:

```cpp
void responseReceived(const QString &content);      // 完整响应
void streamChunkReceived(const QString &chunk);      // 流式片段
void streamFinished(const QString &fullContent);     // 流式结束
void errorOccurred(const QString &error);            // 错误
```

**SSE 解析**:

```cpp
QString extractDeltaFromSSE(const QByteArray &data);
// 解析 "data: {json}" 格式的 SSE 事件
// 提取 choices[0].delta.content 字段
// 处理 "data: [DONE]" 终止标记
```

**设计特点**:

- 异步请求处理
- 流式/非流式双模式
- 自动取消前一个请求

#### SessionManager (`src/core/sessionmanager.*`)

**职责**: 管理会话状态和历史记录

**主要功能**:

- 创建、切换、删除会话
- 添加消息到会话
- 保存/加载会话到文件
- 管理当前活动会话

**关键方法**:

```cpp
static SessionManager* instance();
void createNewSession(const QString &title);
void switchToSession(const QString &sessionId);
void addMessageToCurrentSession(const QString &role, const QString &content);
void removeSession(const QString &sessionId);
void saveSessionsToFile();
void loadSessionsFromFile();
```

**设计特点**:

- 单例模式
- 自动持久化
- 线程安全（可扩展）

### 3. 数据层 (Data Layer)

#### DataModels (`src/core/datamodels.h`)

**职责**: 定义数据结构

**主要模型**:

```cpp
struct ChatMessage
{
    QString role;        // "user", "assistant", "system"
    QString content;     // 消息内容
};

struct ChatSession
{
    QString id;                           // 会话 ID
    QString title;                        // 会话标题
    QVector<ChatMessage> messages;        // 消息列表
};
```

#### Settings (QSettings)

**职责**: 存储应用配置

**配置项**:

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `localMode` | bool | true | 是否使用本地模式 |
| `apiBaseUrl` | string | http://127.0.0.1:8080 | API 基础 URL |
| `apiKey` | string | "" | API 密钥 |
| `modelName` | string | local-model | 模型名称 |
| `temperature` | double | 0.4 | 温度参数 |
| `maxContext` | int | 10 | 最大上下文数 |
| `maxTokens` | int | 2048 | 最大 Token 数 |
| `streamingEnabled` | bool | true | 流式输出开关 |
| `theme` | int | 0 | 主题 (0=跟随系统, 1=亮色, 2=暗色) |
| `language` | string | system | 语言 (system/zh_CN/en) |

#### StyleSheetManager (`src/ui/stylesheetmanager.*`)

**职责**: 管理应用主题

**支持主题**:

| 主题 | 枚举值 | 说明 |
|------|--------|------|
| SystemTheme | 0 | 跟随系统主题 |
| LightTheme | 1 | 亮色主题 |
| DarkTheme | 2 | 暗色主题 |

**特点**:

- 自动检测系统主题（通过 QPalette 亮度判断）
- 主题偏好自动持久化到 QSettings
- 提供完整的亮色/暗色 Qt Style Sheets

#### 翻译系统 (`translations/`)

**职责**: 多语言支持

**支持语言**:

| 语言 | 文件 | 说明 |
|------|------|------|
| 简体中文 | localai_zh_CN.ts | 默认语言 |
| English | localai_en.ts | 英文翻译 |

**加载流程**:

1. 读取 QSettings 中的语言设置
2. 如果是 "system"，使用 QLocale::system().name()
3. 尝试加载对应翻译文件
4. 安装 QTranslator

***

## 核心组件

### 组件交互图

```
┌─────────────┐
│   GUI/CLI   │
└──────┬──────┘
       │
       ↓
┌──────────────────┐
│ SessionManager   │ ←── 单例
└──────┬───────────┘
       │
       ↓
┌──────────────────┐
│ NetworkManager   │ ←── 流式/非流式双模式
└──────┬───────────┘
       │
       ↓
┌──────────────────┐
│   API Server     │ ←── SSE / JSON
└──────────────────┘
```

### 组件职责矩阵

| 组件 | 职责 | 依赖 |
|------|------|------|
| `MainWindow` | GUI 主窗口 | SessionManager, NetworkManager, StyleSheetManager |
| `CLIApplication` | CLI 应用 | SessionManager, NetworkManager |
| `NetworkManager` | 网络请求 + 流式响应 | QNetworkAccessManager |
| `SessionManager` | 会话管理 | DataModels, File Storage |
| `StyleSheetManager` | 主题管理 | QSettings, QApplication |
| `DataModels` | 数据结构 | Qt Core |

***

## 数据流

### 1. 流式响应流程（新增）

```
用户输入
    ↓
表示层 (GUI/CLI)
    ↓
SessionManager.addMessageToCurrentSession("user", message)
    ↓
NetworkManager.sendChatRequestWithContext(messages)
    ↓
发送 HTTP POST 请求 (stream: true)
    ↓
接收 SSE 事件流
    ↓
NetworkManager::onStreamReadyRead()
    ↓
extractDeltaFromSSE() → 解析 "data: {json}" 行
    ↓
emit streamChunkReceived(chunk) → 表示层实时显示
    ↓
NetworkManager::onStreamFinished()
    ↓
emit streamFinished(fullContent)
    ↓
SessionManager.addMessageToCurrentSession("assistant", fullContent)
    ↓
SessionManager.saveSessionsToFile()
```

### 2. 主题切换流程（新增）

```
用户选择主题
    ↓
SettingsDialog 保存到 QSettings
    ↓
StyleSheetManager.setTheme(theme)
    ↓
根据主题选择样式表
    ↓
rootWidget->setStyleSheet(styleSheet)
    ↓
界面即时更新
```

### 3. 语言切换流程（新增）

```
用户选择语言
    ↓
SettingsDialog 保存到 QSettings
    ↓
下次启动时 main.cpp 读取语言设置
    ↓
加载对应的 .qm 翻译文件
    ↓
app.installTranslator(translator)
    ↓
界面使用 tr() 字符串自动翻译
```

***

## 技术栈

### 核心技术

| 技术 | 版本 | 用途 |
|------|------|------|
| **C++** | 17 | 主要编程语言 |
| **Qt** | 6.x | 跨平台框架 |
| **CMake** | 3.16+ | 构建系统 |

### Qt 模块

| 模块 | 用途 |
|------|------|
| **Qt Widgets** | GUI 组件 |
| **Qt Network** | 网络请求（含 SSE 流式） |
| **Qt Core** | 核心功能（信号槽、容器、JSON、翻译等） |
| **Qt LinguistTools** | 翻译文件编译 |

### 第三方依赖

无第三方依赖，仅使用 Qt 框架。

***

## 设计模式

### 1. 单例模式 (Singleton)

**应用**: `SessionManager`, `StyleSheetManager`

### 2. 观察者模式 (Observer)

**应用**: Qt 信号槽机制

```cpp
// 流式响应
connect(m_networkManager, &NetworkManager::streamChunkReceived,
        this, &MainWindow::onStreamChunkReceived);
connect(m_networkManager, &NetworkManager::streamFinished,
        this, &MainWindow::onStreamFinished);
```

### 3. 策略模式 (Strategy)

**应用**: 流式/非流式双模式

```cpp
if (m_streamingEnabled) {
    connect(m_currentReply, &QNetworkReply::readyRead, this, &NetworkManager::onStreamReadyRead);
} else {
    connect(m_currentReply, &QNetworkReply::finished, this, &NetworkManager::onReplyFinished);
}
```

### 4. 模型-视图模式 (Model-View)

**应用**: GUI 会话列表

***

## 扩展性设计

### 1. 添加新语言

1. 在 `translations/` 目录添加新的 `.ts` 文件
2. 在 `CMakeLists.txt` 的 TS_FILES 中添加
3. 在 `SettingsDialog` 的语言下拉框中添加选项
4. 使用 `lupdate` 更新翻译文件，`lrelease` 编译

### 2. 添加新主题

1. 在 `StyleSheetManager` 中添加新的样式表方法
2. 在 Theme 枚举中添加新值
3. 在 `SettingsDialog` 的主题下拉框中添加选项

### 3. 添加新的 API 提供商

1. 在 `NetworkManager` 中添加 API 类型枚举
2. 实现不同的请求构建逻辑
3. 添加配置选项

***

## 未来架构演进

### 短期目标 (v1.2)

- 快捷键支持
- 会话导出/导入
- 代码高亮显示

### 中期目标 (v1.3)

- 插件系统
- 自定义系统提示词
- 模型参数调整界面

### 长期目标 (v2.0)

- 多模型支持
- RAG 支持
- 知识库管理

***

## 参考资料

- [Qt Documentation](https://doc.qt.io/)
- [CMake Documentation](https://cmake.org/documentation/)
- [Clean Architecture](https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html)
- [Design Patterns](https://refactoring.guru/design-patterns)

***
