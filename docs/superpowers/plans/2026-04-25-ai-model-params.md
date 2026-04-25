# AI 模型参数配置扩展实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 扩展 CLI 模型参数配置，支持 top_p、frequency_penalty、seed 等参数，修复参数硬编码问题。

**Architecture:** 在 NetworkManager 中添加新参数成员变量，通过 QSettings 持久化；CLI 添加参数选项和范围校验；统一使用 OpenAI 格式发送请求，Ollama 改用 /api/chat 接口。

**Tech Stack:** C++17, Qt6 (QSettings, QNetworkAccessManager), CLI (QCommandLineParser)

---

## 文件结构

| 文件 | 操作 | 职责 |
|------|------|------|
| `src/core/networkmanager.h` | 修改 | 添加新成员变量声明 |
| `src/core/networkmanager.cpp` | 修改 | 初始化、加载/保存、构建请求 |
| `src/cli/cli_application.cpp` | 修改 | CLI 选项、参数校验、显示配置 |
| `cmake/config.example.json` | 修改 | 更新参数示例 |
| `docs/README.md` | 修改 | 更新参数说明表格 |
| `docs/CHANGELOG.md` | 修改 | 记录版本更新 |

---

### Task 1: 扩展 NetworkManager 头文件

**Files:**
- Modify: `src/core/networkmanager.h:56-59`

- [ ] **Step 1: 添加新成员变量声明**

在 `networkmanager.h` 的私有成员区域（第 56-59 行附近）添加：

```cpp
private:
    // ... 现有成员 ...
    QString m_systemPrompt;
    double m_temperature;
    double m_topP;                  // 新增
    int m_maxContext;
    int m_maxTokens;
    double m_presencePenalty;
    double m_frequencyPenalty;      // 新增
    std::optional<int> m_seed;      // 新增

    bool m_streamingEnabled;
    QString m_streamBuffer;
```

需要添加 `<optional>` 头文件：

```cpp
#include <optional>
```

- [ ] **Step 2: 验证头文件语法正确**

检查修改后的文件确保无语法错误。

---

### Task 2: 初始化新参数默认值

**Files:**
- Modify: `src/core/networkmanager.cpp:5-25`

- [ ] **Step 1: 在构造函数中初始化新参数**

修改 `networkmanager.cpp` 构造函数初始化列表（第 5-14 行）：

```cpp
NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_isLocalMode(true)
    , m_temperature(0.4)
    , m_topP(1.0)                   // 新增
    , m_maxContext(10)
    , m_maxTokens(8192)
    , m_presencePenalty(0.2)
    , m_frequencyPenalty(0.0)       // 新增
    , m_seed(std::nullopt)          // 新增
    , m_streamingEnabled(true)
```

- [ ] **Step 2: 验证构造函数编译正确**

---

### Task 3: 加载和保存新参数设置

**Files:**
- Modify: `src/core/networkmanager.cpp:40-69`

- [ ] **Step 1: 在 loadSettings() 中添加新参数加载**

修改 `loadSettings()` 函数（第 40-54 行），添加：

```cpp
void NetworkManager::loadSettings()
{
    QSettings settings("LocalAIAssistant", "Settings");

    m_isLocalMode = settings.value("localMode", true).toBool();
    m_apiBaseUrl = settings.value("apiBaseUrl", "http://127.0.0.1:8080").toString();
    m_apiKey = settings.value("apiKey", "").toString();
    m_modelName = settings.value("modelName", "local-model").toString();

    m_temperature = settings.value("temperature", 0.4).toDouble();
    m_topP = settings.value("topP", 1.0).toDouble();                  // 新增
    m_maxContext = settings.value("maxContext", 10).toInt();
    m_maxTokens = settings.value("maxTokens", 8192).toInt();
    m_presencePenalty = settings.value("presencePenalty", 0.2).toDouble();
    m_frequencyPenalty = settings.value("frequencyPenalty", 0.0).toDouble();  // 新增

    // seed 处理：-1 表示未设置（null）
    int seedValue = settings.value("seed", -1).toInt();
    if (seedValue >= 0) {
        m_seed = seedValue;
    } else {
        m_seed = std::nullopt;
    }

    m_streamingEnabled = settings.value("streamingEnabled", true).toBool();
}
```

- [ ] **Step 2: 在 saveSettings() 中添加新参数保存**

修改 `saveSettings()` 函数（第 56-69 行），添加：

```cpp
void NetworkManager::saveSettings()
{
    QSettings settings("LocalAIAssistant", "Settings");

    settings.setValue("localMode", m_isLocalMode);
    settings.setValue("apiBaseUrl", m_apiBaseUrl);
    settings.setValue("apiKey", m_apiKey);
    settings.setValue("modelName", m_modelName);
    settings.setValue("temperature", m_temperature);
    settings.setValue("topP", m_topP);                              // 新增
    settings.setValue("maxContext", m_maxContext);
    settings.setValue("maxTokens", m_maxTokens);
    settings.setValue("presencePenalty", m_presencePenalty);
    settings.setValue("frequencyPenalty", m_frequencyPenalty);      // 新增

    // seed 保存：-1 表示 null
    settings.setValue("seed", m_seed.has_value() ? m_seed.value() : -1);

    settings.setValue("streamingEnabled", m_streamingEnabled);
}
```

- [ ] **Step 3: 验证设置加载保存逻辑正确**

---

### Task 4: 修复请求构建中的硬编码参数

**Files:**
- Modify: `src/core/networkmanager.cpp:89-179`

- [ ] **Step 1: 修改 API 路径判断逻辑**

修改 `sendChatRequestWithContext()` 函数中的 API 路径构建（第 97-120 行）：

```cpp
QString fullUrl = m_apiBaseUrl;

if (fullUrl.isEmpty()) {
    emit errorOccurred("API URL is empty");
    return;
}

if (!fullUrl.startsWith("http://") && !fullUrl.startsWith("https://")) {
    fullUrl = "https://" + fullUrl;
}

if (!fullUrl.endsWith("/")) {
    fullUrl += "/";
}

// 检查是否是 Ollama
bool isOllama = fullUrl.contains("11434");

// Ollama 使用 /api/chat 接口（支持 messages 格式和参数）
// 其他使用 OpenAI 格式 /v1/chat/completions
if (isOllama) {
    fullUrl += "api/chat";
} else {
    fullUrl += "v1/chat/completions";
}
```

- [ ] **Step 2: 修改请求 JSON payload 构建**

修改 JSON payload 构建部分（第 129-157 行）：

```cpp
QJsonObject jsonPayload;
jsonPayload["model"] = m_modelName;
jsonPayload["temperature"] = m_temperature;        // 使用成员变量而非硬编码
jsonPayload["top_p"] = m_topP;                     // 新增
jsonPayload["max_tokens"] = m_maxTokens;
jsonPayload["presence_penalty"] = m_presencePenalty;
jsonPayload["frequency_penalty"] = m_frequencyPenalty;  // 新增
jsonPayload["stream"] = m_streamingEnabled;

// seed 仅在有值时传递
if (m_seed.has_value()) {
    jsonPayload["seed"] = m_seed.value();
}

// 统一使用 messages 格式（Ollama /api/chat 也支持）
jsonPayload["messages"] = buildMessagesArray(messages, m_maxContext);
```

- [ ] **Step 3: 验证请求构建逻辑正确**

---

### Task 5: 添加 CLI 参数选项

**Files:**
- Modify: `src/cli/cli_application.cpp:35-80`

- [ ] **Step 1: 添加新的 CLI 命令行选项定义**

在 `run()` 函数的选项定义区域（第 44-80 行附近）添加：

```cpp
QCommandLineOption temperatureOpt(QStringList() << "temperature",
    "设置温度参数 (0.0-2.0)", "value");
QCommandLineOption topPOpt(QStringList() << "top-p",
    "设置 top_p 参数 (0.0-1.0)", "value");
QCommandLineOption maxTokensOpt(QStringList() << "max-tokens",
    "设置最大输出 tokens (1-128000)", "value");
QCommandLineOption presencePenaltyOpt(QStringList() << "presence-penalty",
    "设置存在惩罚 (-2.0-2.0)", "value");
QCommandLineOption frequencyPenaltyOpt(QStringList() << "frequency-penalty",
    "设置频率惩罚 (-2.0-2.0)", "value");
QCommandLineOption seedOpt(QStringList() << "seed",
    "设置随机种子 (整数，-1 表示清除)", "value");
QCommandLineOption maxContextOpt(QStringList() << "max-context",
    "设置最大上下文消息数 (1-100)", "value");

parser.addOption(temperatureOpt);
parser.addOption(topPOpt);
parser.addOption(maxTokensOpt);
parser.addOption(presencePenaltyOpt);
parser.addOption(frequencyPenaltyOpt);
parser.addOption(seedOpt);
parser.addOption(maxContextOpt);
```

- [ ] **Step 2: 验证选项定义正确**

---

### Task 6: 添加 CLI 参数范围校验

**Files:**
- Modify: `src/cli/cli_application.cpp`

- [ ] **Step 1: 在 handleConfigCommand() 前添加校验辅助函数**

在文件中添加校验函数（在 `handleConfigCommand()` 函数前）：

```cpp
namespace {
    bool validateDoubleParam(const std::string &name, double value, double min, double max) {
        if (value < min || value > max) {
            std::cerr << "错误: " << name << " 值 " << value
                      << " 超出范围 [" << min << ", " << max << "]" << std::endl;
            return false;
        }
        return true;
    }

    bool validateIntParam(const std::string &name, int value, int min, int max) {
        if (value < min || value > max) {
            std::cerr << "错误: " << name << " 值 " << value
                      << " 超出范围 [" << min << ", " << max << "]" << std::endl;
            return false;
        }
        return true;
    }
}
```

- [ ] **Step 2: 在 handleConfigCommand() 中添加参数处理和校验**

修改 `handleConfigCommand()` 函数（第 397-438 行），添加新参数处理：

```cpp
int CLIApplication::handleConfigCommand(const QCommandLineParser &parser)
{
    QSettings settings("LocalAIAssistant", "Settings");

    bool hasChanges = false;
    bool validationFailed = false;

    // 处理 API 相关参数（保持现有逻辑）
    QString apiUrl = settings.value("apiBaseUrl", "http://127.0.0.1:8080").toString();
    QString apiKey = settings.value("apiKey", "").toString();
    QString modelName = settings.value("modelName", "local-model").toString();
    bool isLocalMode = settings.value("localMode", true).toBool();

    if (parser.isSet("api-url")) {
        apiUrl = parser.value("api-url");
        hasChanges = true;
    }
    if (parser.isSet("api-key")) {
        apiKey = parser.value("api-key");
        hasChanges = true;
    }
    if (parser.isSet("model")) {
        modelName = parser.value("model");
        hasChanges = true;
    }
    if (parser.isSet("local")) {
        isLocalMode = true;
        hasChanges = true;
    }
    if (parser.isSet("external")) {
        isLocalMode = false;
        hasChanges = true;
    }

    // 处理模型参数（新增）
    if (parser.isSet("temperature")) {
        double temp = parser.value("temperature").toDouble();
        if (!validateDoubleParam("temperature", temp, 0.0, 2.0)) {
            validationFailed = true;
        } else {
            settings.setValue("temperature", temp);
            hasChanges = true;
        }
    }

    if (parser.isSet("top-p")) {
        double topP = parser.value("top-p").toDouble();
        if (!validateDoubleParam("top_p", topP, 0.0, 1.0)) {
            validationFailed = true;
        } else {
            settings.setValue("topP", topP);
            hasChanges = true;
        }
    }

    if (parser.isSet("max-tokens")) {
        int maxTokens = parser.value("max-tokens").toInt();
        if (!validateIntParam("max_tokens", maxTokens, 1, 128000)) {
            validationFailed = true;
        } else {
            settings.setValue("maxTokens", maxTokens);
            hasChanges = true;
        }
    }

    if (parser.isSet("presence-penalty")) {
        double penalty = parser.value("presence-penalty").toDouble();
        if (!validateDoubleParam("presence_penalty", penalty, -2.0, 2.0)) {
            validationFailed = true;
        } else {
            settings.setValue("presencePenalty", penalty);
            hasChanges = true;
        }
    }

    if (parser.isSet("frequency-penalty")) {
        double penalty = parser.value("frequency-penalty").toDouble();
        if (!validateDoubleParam("frequency_penalty", penalty, -2.0, 2.0)) {
            validationFailed = true;
        } else {
            settings.setValue("frequencyPenalty", penalty);
            hasChanges = true;
        }
    }

    if (parser.isSet("seed")) {
        int seed = parser.value("seed").toInt();
        // seed 范围校验：任意整数有效，-1 表示清除
        settings.setValue("seed", seed);
        hasChanges = true;
    }

    if (parser.isSet("max-context")) {
        int maxContext = parser.value("max-context").toInt();
        if (!validateIntParam("max_context", maxContext, 1, 100)) {
            validationFailed = true;
        } else {
            settings.setValue("maxContext", maxContext);
            hasChanges = true;
        }
    }

    if (validationFailed) {
        std::cerr << "配置未更新，请修正参数值后重试" << std::endl;
        return 1;
    }

    if (hasChanges) {
        m_networkManager->updateSettings(apiUrl, apiKey, modelName, isLocalMode);
        std::cout << "配置已更新" << std::endl;
    }

    if (parser.isSet("show-config") || !hasChanges) {
        showConfig();
    }

    return 0;
}
```

- [ ] **Step 3: 验证校验逻辑正确**

---

### Task 7: 更新 showConfig 显示新参数

**Files:**
- Modify: `src/cli/cli_application.cpp:298-317`

- [ ] **Step 1: 修改 showConfig() 显示所有模型参数**

修改 `showConfig()` 函数（第 298-317 行）：

```cpp
void CLIApplication::showConfig()
{
    QSettings settings("LocalAIAssistant", "Settings");
    std::cout << "\n当前配置:\n";
    std::cout << "----------------------------------------\n";
    std::cout << "模式: "
              << (settings.value("localMode", true).toBool() ? "本地模式" : "外部API模式")
              << "\n";
    std::cout << "API URL: "
              << settings.value("apiBaseUrl", "http://127.0.0.1:8080").toString().toStdString()
              << "\n";
    std::cout << "模型: "
              << settings.value("modelName", "local-model").toString().toStdString()
              << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << "模型参数:\n";
    std::cout << "  temperature: " << settings.value("temperature", 0.4).toDouble() << "\n";
    std::cout << "  top_p: " << settings.value("topP", 1.0).toDouble() << "\n";
    std::cout << "  max_tokens: " << settings.value("maxTokens", 8192).toInt() << "\n";
    std::cout << "  presence_penalty: " << settings.value("presencePenalty", 0.2).toDouble() << "\n";
    std::cout << "  frequency_penalty: " << settings.value("frequencyPenalty", 0.0).toDouble() << "\n";

    int seed = settings.value("seed", -1).toInt();
    if (seed >= 0) {
        std::cout << "  seed: " << seed << "\n";
    } else {
        std::cout << "  seed: 未设置 (null)\n";
    }

    std::cout << "  max_context: " << settings.value("maxContext", 10).toInt() << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << "流式输出: " << (m_networkManager->isStreamingEnabled() ? "开启" : "关闭") << "\n";
    std::cout << "----------------------------------------\n";
}
```

- [ ] **Step 2: 验证显示格式正确**

---

### Task 8: 更新 printUsage 帮助信息

**Files:**
- Modify: `src/cli/cli_application.cpp:117-147`

- [ ] **Step 1: 修改 printUsage() 添加新参数说明**

修改 `printUsage()` 函数（第 117-147 行），在配置管理选项部分添加：

```cpp
void CLIApplication::printUsage()
{
    std::cout << "\n本地AI助手 - 命令行版本 v1.0.0\n\n";
    std::cout << "用法:\n";
    std::cout << "  ai chat                    进入交互式对话模式\n";
    std::cout << "  ai ask <问题>              单次查询\n";
    std::cout << "  ai sessions [选项]         会话管理\n";
    std::cout << "  ai config [选项]           配置管理\n\n";
    std::cout << "会话管理选项:\n";
    std::cout << "  -l, --list                 列出所有会话\n";
    std::cout << "  -n, --new                  创建新会话\n";
    std::cout << "  -d, --delete <id>          删除指定会话\n";
    std::cout << "  --show <id>                显示会话内容\n";
    std::cout << "  -s, --session <id>         切换到指定会话\n\n";
    std::cout << "配置管理选项:\n";
    std::cout << "  --api-url <url>            设置API基础URL\n";
    std::cout << "  --api-key <key>            设置API密钥\n";
    std::cout << "  --model <name>             设置模型名称\n";
    std::cout << "  --local                    使用本地模式\n";
    std::cout << "  --external                 使用外部API模式\n";
    std::cout << "  --show-config              显示当前配置\n\n";
    std::cout << "模型参数选项:\n";
    std::cout << "  --temperature <value>      设置温度 (0.0-2.0)\n";
    std::cout << "  --top-p <value>            设置 top_p (0.0-1.0)\n";
    std::cout << "  --max-tokens <value>       设置最大输出 tokens (1-128000)\n";
    std::cout << "  --presence-penalty <value> 设置存在惩罚 (-2.0-2.0)\n";
    std::cout << "  --frequency-penalty <value> 设置频率惩罚 (-2.0-2.0)\n";
    std::cout << "  --seed <value>             设置随机种子 (整数，-1清除)\n";
    std::cout << "  --max-context <value>      设置最大上下文消息数 (1-100)\n\n";
    std::cout << "输出选项:\n";
    std::cout << "  --no-stream                禁用流式输出，等待完整响应\n\n";
    std::cout << "示例:\n";
    std::cout << "  ai chat                    # 启动交互式对话\n";
    std::cout << "  ai ask \"什么是量子计算?\"   # 单次查询\n";
    std::cout << "  ai config --show-config    # 显示当前配置\n";
    std::cout << "  ai config --temperature 0.7 --top-p 0.9  # 设置模型参数\n";
    std::cout << std::endl;
}
```

- [ ] **Step 2: 验证帮助信息格式正确**

---

### Task 9: 更新配置示例文件

**Files:**
- Modify: `cmake/config.example.json`

- [ ] **Step 1: 更新 config.example.json**

修改 `cmake/config.example.json` 内容：

```json
{
  "api": {
    "baseUrl": "http://127.0.0.1:8080",
    "apiKey": "",
    "modelName": "local-model",
    "localMode": true
  },
  "model": {
    "temperature": 0.4,
    "top_p": 1.0,
    "maxContext": 10,
    "maxTokens": 8192,
    "presencePenalty": 0.2,
    "frequencyPenalty": 0.0,
    "seed": null
  },
  "systemPrompt": "你是一个集成在电脑桌面端的专业 AI 助手。\n1. 遇到技术问题，请先进行逻辑拆解（Thought），再给出结论。\n2. 如果是代码建议，请确保符合现代代码规范。\n3. 保持回答简洁有力，避免过度废话。"
}
```

- [ ] **Step 2: 验证 JSON 格式正确**

---

### Task 10: 更新 README 文档

**Files:**
- Modify: `docs/README.md:269-277`

- [ ] **Step 1: 更新参数说明表格**

修改 `docs/README.md` 中的参数说明表格（第 269-277 行）：

```markdown
### 参数说明

| 参数 | 默认值 | 推荐值 | 说明 |
|---|---|---|---|
| API URL | `http://127.0.0.1:8080` | 本地: `http://127.0.0.1:11434` / OpenAI: `https://api.openai.com` | API 服务地址 |
| 温度 (temperature) | 0.4 | 0.3(精确) ~ 0.7(创意) | 控制输出随机性 |
| top_p | 1.0 | 0.9(限制) ~ 1.0(不限制) | nucleus sampling 参数 |
| 最大上下文 (max_context) | 10 条消息 | 5~20 | 保留的历史消息数 |
| 最大 Token (max_tokens) | 8192 | 根据模型调整 | 单次回复最大长度 |
| 存在惩罚 (presence_penalty) | 0.2 | -2.0~2.0 | 鼓励讨论新话题 |
| 频率惩罚 (frequency_penalty) | 0.0 | -2.0~2.0 | 减少重复内容 |
| 随机种子 (seed) | 未设置 | 可选 | 固定输出一致性 |
```

- [ ] **Step 2: 在 CLI 使用部分添加参数设置示例**

在 CLI 配置管理部分（第 363-366 行附近）添加示例：

```markdown
#### 模型参数设置

```bash
# 设置单个参数
./build/LocalAIAssistant-CLI config --temperature 0.7

# 设置多个参数
./build/LocalAIAssistant-CLI config --temperature 0.7 --top-p 0.9 --seed 42

# 查看所有配置
./build/LocalAIAssistant-CLI config --show-config
```
```

- [ ] **Step 3: 验证文档格式正确**

---

### Task 11: 更新 CHANGELOG

**Files:**
- Modify: `docs/CHANGELOG.md`

- [ ] **Step 1: 添加版本更新记录**

在 `docs/CHANGELOG.md` 开头添加：

```markdown
## v1.2.0 (2026-04-25)

### 新增功能

- CLI 支持完整模型参数配置：
  - 新增 `--top-p` 参数 (0.0-1.0)
  - 新增 `--frequency-penalty` 参数 (-2.0-2.0)
  - 新增 `--seed` 参数（可选随机种子）
  - 新增 `--max-context` 参数 (1-100)
- 参数范围校验，超出范围拒绝并提示错误

### 修复

- 修复 `temperature` 等参数发送时硬编码问题，改为使用配置值
- Ollama 后端改用 `/api/chat` 接口，支持 messages 格式和参数传递

### 文档更新

- 更新 README 参数说明表格
- 更新 config.example.json 示例
```

- [ ] **Step 2: 验证 CHANGELOG 格式正确**

---

### Task 12: 编译验证

- [ ] **Step 1: 编译项目**

```bash
cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant
./scripts/build.sh
```

预期：编译成功，无错误。

- [ ] **Step 2: 测试 CLI 参数设置**

```bash
./build/LocalAIAssistant-CLI config --show-config
./build/LocalAIAssistant-CLI config --temperature 0.7 --top-p 0.9
./build/LocalAIAssistant-CLI config --show-config
```

预期：参数正确显示和更新。

- [ ] **Step 3: 测试参数校验**

```bash
./build/LocalAIAssistant-CLI config --temperature 3.0
```

预期：输出错误提示 "错误: temperature 值 3.0 超出范围 [0.0, 2.0]"，配置未更新。

---

### Task 13: 提交更改

- [ ] **Step 1: 提交所有更改**

```bash
git add src/core/networkmanager.h src/core/networkmanager.cpp \
        src/cli/cli_application.cpp cmake/config.example.json \
        docs/README.md docs/CHANGELOG.md \
        docs/superpowers/specs/2026-04-25-ai-model-params-design.md \
        docs/superpowers/plans/2026-04-25-ai-model-params.md
git commit -m "feat: 扩展 CLI 模型参数配置

- 新增 top_p、frequency_penalty、seed 参数支持
- 修复 temperature 等参数硬编码问题
- Ollama 改用 /api/chat 接口
- CLI 添加参数范围校验
- 更新文档和配置示例

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

## 完成标准

1. CLI 可设置和显示所有模型参数
2. 参数范围校验正常工作
3. 参数正确传递到 API 请求（不再硬编码）
4. Ollama 和 OpenAI 后端都能正常工作
5. 文档和配置示例已更新