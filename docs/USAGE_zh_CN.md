# 本地AI助手 — 使用帮助

## 概述

本地AI助手是一款基于 Qt
6 的跨平台桌面 AI 应用，支持 GUI 图形界面和 CLI 命令行两种模式，内置 AI 女友语音交互模块。

### 主要功能

- **双模式**：GUI 桌面应用 + CLI 命令行界面
- **文件上传**：支持文本、图片、PDF 附件
- **流式输出**：AI 回复实时逐字显示
- **会话管理**：多会话切换，历史持久化
- **多语言**：简体中文 / English
- **主题切换**：亮色 / 暗色 / 跟随系统
- **知识库**：AI 驱动的文档语义搜索
- **AI 女友**：语音交互与情绪系统
- **跨平台**：macOS / Windows / Linux

---

## 快速开始

### 启动应用

**macOS：**

```bash
open build/LocalAIAssistant.app
```

**Windows：**

```bash
build\LocalAIAssistant.exe
# 调试模式（显示日志控制台）：
build\LocalAIAssistant.exe --debug
```

**Linux：**

```bash
./build/LocalAIAssistant
```

> **macOS 首次启动**：右键点击 App →「打开」来绕过 Gatekeeper（应用未签名）。 **Windows
> SmartScreen**：点击「更多信息」→「仍要运行」。

---

## 主窗口（GUI）

### 与 AI 对话

1. 在底部输入框中输入消息
2. 按 **回车** 或点击 **发送** 提交
3. AI 回复将实时流式显示
4. AI 回复期间可以继续输入，消息会自动排队

### 文件附件

发送前点击 **📎** 按钮添加附件：

| 格式                    | 说明                     |
| ----------------------- | ------------------------ |
| `.txt`, `.md`           | 文本文件                 |
| `.png`, `.jpg`, `.jpeg` | 图片（需要模型支持识图） |
| `.pdf`                  | PDF 文档                 |

### 会话管理

左侧面板显示所有对话会话：

- 点击 **+ 新建会话** 开始新对话
- 点击任意会话切换
- 右键点击会话可重命名、置顶或删除
- 对话自动保存

### 快捷键

| 快捷键         | 功能                  |
| -------------- | --------------------- |
| `Ctrl/Cmd + N` | 新建会话              |
| `Ctrl/Cmd + G` | 打开/关闭 AI 女友窗口 |
| `Enter`        | 发送消息              |

---

## 设置

从菜单栏或工具栏齿轮图标打开设置。

### 通用选项卡

| 设置项       | 说明                                                                   |
| ------------ | ---------------------------------------------------------------------- |
| **API 地址** | AI 服务端点（如 `https://api.openai.com`）                             |
| **API 密钥** | 您的认证密钥                                                           |
| **模型名称** | 使用的 AI 模型（如 `gpt-4o`、`claude-sonnet-4-6`）                     |
| **API 类型** | 选择后端：OpenAI 兼容、Ollama、llama.cpp (本地 OpenAI)、Anthropic 兼容 |
| **流式输出** | 启用实时逐字回复                                                       |
| **主题**     | 亮色 / 暗色 / 跟随系统                                                 |
| **语言**     | 界面语言（中文 / English）                                             |

### 知识库选项卡

详见下方[知识库](#知识库)章节。

### 安全选项卡

详见下方[安全设置](#安全设置)章节。

---

## AI 服务配置

应用需要连接 AI 后端服务才能工作，两种方式可选：

### 方式一：云端 API（推荐）

| 服务商               | API 地址                      | 说明          |
| -------------------- | ----------------------------- | ------------- |
| OpenAI               | `https://api.openai.com`      | 需要 API Key  |
| 并行科技             | `https://llmapi.paratera.com` | 国内 API 代理 |
| 其他 OpenAI 兼容服务 | 按服务商文档                  | —             |

在设置 → 通用中填入 **API 地址**、**API 密钥** 和 **模型名称**。

### 方式二：本地部署

#### Ollama

1. 安装 [Ollama](https://ollama.com/download)
2. 下载模型：`ollama pull llama3`
3. 在设置中配置：
    - API 地址：`http://127.0.0.1:11434`
    - API 类型：`Ollama`
    - 模型名：`llama3`

#### llama.cpp

1. 下载并编译 [llama.cpp](https://github.com/ggerganov/llama.cpp)
2. 下载 GGUF 模型文件
3. 启动 server：`llama-server -m model.gguf --port 8080`
4. 在设置中配置：
    - API 地址：`http://127.0.0.1:8080`
    - API 类型：`llama.cpp (本地 OpenAI 兼容)`
    - 模型名：`local-model`

> llama.cpp server 使用 OpenAI 兼容协议 (`/v1/chat/completions`)，默认端口 8080。选择 `llama.cpp`
> 类型会自动启用本地模式（HTTP、无需认证）。

---

## 知识库

知识库支持通过 AI 驱动的语义搜索来检索文档。

### 导入文档

1. 打开设置 → **知识库** 选项卡
2. 点击 **导入文档到知识库**
3. 选择 `.txt`、`.md`、`.pdf` 或 `.docx` 文件

文档将自动切分为片段、向量化并建立索引。

### 搜索

在主聊天中，AI 回答问题时自动搜索知识库以获取相关上下文。

### 依赖要求

- **未安装 ONNX Runtime**：使用占位向量（仍可搜索，精度较低）
- **已安装 ONNX Runtime**：真实嵌入推理，语义搜索更精准

---

## AI 女友模块

通过 **Ctrl/Cmd + G** 或菜单打开 AI 女友窗口。

### 头像等级

| 等级             | 类型     | 说明               |
| ---------------- | -------- | ------------------ |
| Level 1 (Belle)  | 静态 PNG | 经典风格           |
| Level 2 (Hot)    | 静态 PNG | 更加火辣           |
| Level 3 (Hotter) | MP4 视频 | 动态视频，跃然屏上 |

> **Level
> 3 视频模式注意**：视频模式下聊天输入区会隐藏——这是刻意设计，用于纯粹的视频观赏体验。设置按钮、情绪标签和心情条仍然会显示在视频上方。这不是 bug。

### 情绪系统

头像根据对话内容实时表达
**14 种情绪**：开心、害羞、爱意、生气、伤心、哭泣、害怕、担心、讨厌、期待、说话、思考、默认、旅行。

情绪基于消息中的关键词检测。当没有匹配到特定关键词时，头像会从**心情对应的情绪池中随机**选取——因此同样的对话不会每次看起来都一样。

**空闲轮换**：当你超过 30 秒未发送消息，头像会在默认、思考、期待情绪之间缓慢轮换（每 5 秒切换）。发送新消息会立即停止轮换。

### 心情系统

- **心情条**：左上角显示心情进度条和百分比。颜色从暗灰色（低落）→ 中性灰 → 暖粉色 → 亮粉色（极开心）渐变——心情越好，颜色越亮。
- **心情影响等级**：可设置为低/中/高，控制情绪检测对心情的影响程度
- 心情通过对话自然演变

### 设置菜单（⚙️）

在女友窗口右上角打开：

| 菜单项          | 说明                 |
| --------------- | -------------------- |
| **语音输出**    | 开启/关闭文字转语音  |
| **配置语音...** | 填写讯飞语音服务凭证 |
| **清空历史**    | 删除当前会话的消息   |

### 语音交互

AI 女友通过讯飞开放平台 WebSocket API 支持语音输入（语音识别）和语音输出（语音合成）。

#### 配置语音服务

1. 注册[讯飞开放平台](https://www.xfyun.cn)账号
2. 创建应用并开通服务：
    - **语音听写**（流式版，WebSocket）
    - **语音合成**（超拟人版，WebSocket）
3. 获取凭证：**APP ID**、**API Key**、**API Secret**
4. 在女友窗口中点击 ⚙️ → **配置语音...** 填入凭证
5. APP ID、API Key、API Secret 为必填项；URL 和音色为可选项（有内置默认值）

#### 语音输入

点击女友窗口中的**麦克风按钮**开始说话，语音将被识别并作为文本消息发送。

#### 语音输出

开启后，AI 的文字回复将自动合成为语音并播放。

> **平台支持**：
>
> - **macOS**：语音输入 + 语音输出 ✅
> - **Windows**：仅语音输出（TTS）；语音输入暂不支持 ⚠️
> - **Linux**：仅语音输出（TTS）；语音输入未充分测试

### 自定义人设

编辑 `personality.md`
文件可自定义 AI 女友的性格、说话风格和行为模式。文件支持以下模板变量（程序自动替换，无需手动修改）：

| 变量                | 说明                            |
| ------------------- | ------------------------------- |
| `{{user_nickname}}` | 用户昵称，默认"你"              |
| `{{mood_hint}}`     | 根据心情自动填充提示词          |
| `{{time_context}}`  | 根据当前时间自动填充场景描述    |
| `{{user_memories}}` | 从 `memory.md` 注入用户记忆档案 |

文件底部的 `<!-- CONFIG_START -->` 配置块可自定义心情和时间提示词文本，格式为
`key=值`，支持以下键名：

| 配置键           | 触发条件   | 默认值示例                |
| ---------------- | ---------- | ------------------------- |
| `mood_low`       | 心情 < 0.3 | 心情很差，说话带着哭腔... |
| `mood_mid`       | 心情 < 0.5 | 有点不开心，说话简短...   |
| `mood_high`      | 心情 > 0.8 | 开开心心，语气特别甜...   |
| `time_morning`   | 6-10 点    | 早上%1点，用户刚起床...   |
| `time_noon`      | 10-14 点   | 中午%1点，该吃午饭了      |
| `time_evening`   | 18-22 点   | 晚上%1点，用户可能在休息  |
| `time_night`     | 22-6 点    | 深夜%1点，用户该睡觉了... |
| `time_afternoon` | 14-18 点   | 下午%1点                  |

修改等号右侧文本即可自定义提示词，`%1`
会被替换为当前小时数。若删除整个 CONFIG 块，程序会使用内置默认值。

### 记忆系统

AI 女友通过持久化记忆系统记住关于你的信息。记忆存储在 `memory.md` 中，跨会话引用。

---

## CLI 命令行模式

启动命令行界面：

```bash
# macOS / Linux
./build/LocalAIAssistant-CLI

# Windows (Git Bash)
./build/LocalAIAssistant-CLI.exe
```

### CLI 交互命令

| 命令           | 功能                 |
| -------------- | -------------------- |
| `/help`        | 显示帮助             |
| `/new`         | 新建会话             |
| `/list`        | 列出所有会话         |
| `/switch <id>` | 切换会话             |
| `/delete <id>` | 删除会话             |
| `/config`      | 显示当前配置         |
| `/file <path>` | 添加文件附件         |
| `/listfiles`   | 查看待发送文件       |
| `/clearfiles`  | 清空文件列表         |
| `/confirm`     | 确认执行待定任务计划 |
| `/cancel`      | 取消待定任务计划     |
| `/undo`        | 撤销上次操作         |
| `/exit`        | 退出程序             |

### 单次查询

```bash
# 单次提问
./build/LocalAIAssistant-CLI ask "什么是人工智能？"

# 会话管理
./build/LocalAIAssistant-CLI sessions -l    # 列出
./build/LocalAIAssistant-CLI sessions -n    # 新建

# 配置
./build/LocalAIAssistant-CLI config --show-config
./build/LocalAIAssistant-CLI config --api-url "http://127.0.0.1:11434"
```

### 任务执行与 Agent 迭代循环

AI 可以自主执行多步骤任务，通过反馈循环持续工作：

```
用户提问 → AI 生成 [TASK_PLAN] → 计划执行 → [ITERATION_FEEDBACK] 反馈给 AI
    → AI 检查结果 → 如果还有工作要做：生成新的 [TASK_PLAN]
                  → 如果任务完成：[TASK_COMPLETE]
```

此循环重复执行，直到 AI 声明 `[TASK_COMPLETE]` 或达到最大迭代次数（默认 10 次）。

**AI 使用的标签：**

| 标签 | 用途 |
|------|------|
| `[TASK_PLAN]...[/TASK_PLAN]` | 发出命令计划（JSON 格式） |
| `[TASK_COMPLETE]` | 表示任务完全完成 |
| `[TASK_FINISHED]` | `[TASK_COMPLETE]` 的别名 |
| `[ITERATION_FEEDBACK]` | 由程序注入——展示上一轮执行结果 |

#### TASK_PLAN JSON 格式

AI 以 JSON 块的形式生成任务计划。程序支持文本和 JSON 两种格式，自动检测并解析。新模型默认生成 JSON：

```json
{
    "description": "简要描述正在执行的操作",
    "requiresConfirmation": false,
    "operations": [
        {
            "type": "create_dir",
            "target": "~/myproject/src",
            "description": "创建源代码目录"
        },
        {
            "type": "write_file",
            "target": "~/myproject/README.md",
            "command": "# 我的项目\n\n项目描述内容",
            "description": "编写 README 文件"
        }
    ]
}
```

**可用的操作类型：**

| 类型 | 用途 | 关键字段 |
|------|------|---------|
| `create_dir` | 创建目录（跨平台） | `target` |
| `write_file` | 创建或覆盖文件 | `target`, `command`（内容） |
| `move_file` | 移动或重命名文件/目录 | `source`, `target` |
| `copy_file` | 递归复制文件/目录 | `source`, `target` |
| `delete_file` | 删除文件或目录 | `source` |
| `search_files` | 按模式搜索文件 | `source`, `command`（通配符） |
| `shell_command` | 运行 shell 命令 | `command`, `workingDir`（可选） |
| `shell_script` | 运行多行脚本 | `command` |

> 优先使用原生类型（`create_dir`、`write_file` 等）——它们跨平台、更安全，不依赖 shell。
> 仅在需要使用 `git`、`npm`、`brew` 等工具时才使用 `shell_command`。

#### 交互模式示例（多轮迭代）

测试项目脚手架请求的实际输出：

```
> 帮我创建 ~/myproject，包含 src、tests、docs 子目录和 README

*** Command plan requires confirmation ***
Description: 创建项目目录结构和 README
Commands (3):
  1. create_dir → ~/myproject/src
  2. create_dir → ~/myproject/tests
  3. create_dir → ~/myproject/docs

Type /confirm to execute, /cancel to abort.
> /confirm
Executing...
  [1] OK  (5ms)
  [2] OK  (3ms)
  [3] OK  (4ms)

--- Command plan finished: 3/3 succeeded ---

# AI 检查反馈后，决定继续工作，生成新计划：
*** Command plan requires confirmation ***
Description: 创建 README.md
Commands (1):
  1. write_file → ~/myproject/README.md

> /confirm
Executing...
  [1] OK  (2ms)

--- Command plan finished: 1/1 succeeded ---

[TASK_COMPLETE] 项目脚手架已创建完毕，包含 src、tests、docs 和 README。
--- Task completed ---
```

#### Ask 模式（单次查询，带确认）

```bash
$ ./LocalAIAssistant-CLI ask "在 ~/test 创建 hello.txt"

*** Command plan requires confirmation ***
Commands (1):
  1. write_file → ~/test/hello.txt

Execute? [Y/n]: y
Executing...

[TASK_COMPLETE] 文件已创建。
--- Task completed ---
```

#### Ask 模式 + --yes（脚本自动化）

```bash
$ ./LocalAIAssistant-CLI ask --yes "在 ~/test 创建 hello.txt"

Auto-confirming (--yes)...
Executing...
[TASK_COMPLETE] 文件已创建。
```

#### 性能注意事项

- **启用流式输出模式**以获得响应迅速的 CLI 输出。非流式模式下，AI 的完整响应会一次性到达，
  可能需要 30-60 秒才能看到任何输出。
- **任务执行时间**因模型而异——有些模型生成 TASK_PLAN 的速度比其他模型快。
- **首次迭代**包含初始 API 调用（30-60 秒）。同一 Agent 循环内的后续迭代复用对话上下文，
  API 调用更短。
- 部分 AI 模型有自己的安全层，可能会拒绝生成 `sudo` 等命令——
  这是在程序自身 SafetyChecker 之上的纵深防御。

> **重要提示**：`--yes` 仅跳过 Tier 2（路径确认）。Tier 1（危险命令如 `sudo`、`rm -rf`、
> `eval`）**始终被拦截**，即使用 `--yes` 也不会放行。

---

## 安全设置

### 三级安全架构

SafetyChecker 在执行前对每个文件操作和 Shell 命令进行校验：

| 等级 | 名称 | 行为 | 示例 | `--yes` 效果 |
|------|------|------|------|-------------|
| **Tier 1** | 永久拦截 | 立即拒绝，并显示具体原因。无用户覆盖选项。 | `sudo`、`rm -rf`、`eval`、反引号注入、`cmd /c` | **无效** — Tier 1 永不被绕过 |
| **Tier 2** | 需用户确认 | 用户必须明确批准。选项：允许本次 / 永久允许 / 拒绝 | 系统路径（`/etc`、`C:\Windows`）、白名单外路径 | **自动确认** — 所有违规路径临时允许 |
| **Tier 3** | 自动批准 | 自动执行，无需提示。 | `~/`、`/tmp`、桌面、文档、当前目录 | 不适用 — 已自动批准 |

**Tier 1 拦截原因消息**（实际输出，语言取决于应用语言设置）：

| 检测到的模式 | 拦截原因（中文环境） |
|-------------|---------------------|
| 命令注入（eval、反引号等） | `检测到潜在的命令注入` |
| `sudo` 前缀 | `禁止使用 sudo 提权` |
| `rm -rf /` 或 `rm -rf /*` | `禁止递归删除根目录或系统目录` |
| `runas` 或提权命令 | `禁止使用提权命令` |
| 磁盘操作（`dd`、`format`） | `禁止磁盘操作命令` |
| 系统服务操作 | `禁止操作系统服务` |
| 强制关机/重启 | `禁止强制关机/重启` |
| 关闭防火墙 | `禁止关闭防火墙` |

> 部分 AI 模型也会在自身安全层拒绝生成危险命令。这提供了纵深防御：即使 AI 生成了命令，
> SafetyChecker 也会拦截；反之，即使 SafetyChecker 有 bug，模型自身的拒绝也能阻止执行。

### 路径违规响应

当 AI 尝试访问白名单外的路径时，应用会显示详细警告。

**操作类型标签：**
- `[READ]` — 只读操作（`ls`、`cat`、`grep`、`dir`、`find`） — 黄色显示
- `[WRITE]` — 写入操作（`touch`、`mkdir`、`rm`、文件创建） — 红色显示

**路径类别标签：**
- `[SYSTEM PATH]` — 受保护的系统目录（`/etc`、`/bin`、`C:\Windows` 等）
- `[OUTSIDE WHITELIST]` — 不在默认白名单或用户已批准列表中的路径

#### CLI ask 模式（`ask` 命令）

```
*** Path access warnings ***
  1. [READ] [SYSTEM PATH] /etc/
  2. [WRITE] [OUTSIDE WHITELIST] /opt/config.ini
Execute? [Y/n]: y
```

- `y` 或回车 — 允许本次会话所有违规路径，执行计划
- `n` — 取消计划。打印 "Cancelled."

#### CLI 交互模式（`chat` 命令）— 逐项切换

```
*** Path access toggles ***
  1. [READ] [SYSTEM PATH] /etc/ → ALLOW ONCE
  2. [WRITE] [OUTSIDE WHITELIST] /opt/config.ini → ALLOW ONCE
a=allow all once  p=always allow all  d=deny all  number=toggle single

Type /confirm to execute, /cancel to abort.
```

| 按键 | 操作 |
|------|------|
| `a` | 全部设为**允许本次**（会话范围，列表重新渲染） |
| `p` | 全部设为**永久允许**（保存到 QSettings，列表重新渲染） |
| `d` | 全部设为**拒绝**（列表重新渲染，计划仍在待定——未取消） |
| `1`–`9` | 逐项循环切换：拒绝 → 允许本次 → 永久允许（列表重新渲染） |
| `/confirm` | 应用逐项选择并执行计划 |
| `/cancel` | 取消计划并清除待定状态 |
| （聊天消息） | 多字符输入穿透——停止待定计划，开始新对话 |

无效的单字符输入（如 `x`）打印：`"Invalid input. Use a/p/d/number, /confirm, or /cancel."` —— 计划仍保持待定状态。

#### GUI 确认对话框

为每个路径违规提供独立的 `[允许本次]` / `[永久允许]` / `[拒绝]` 按钮。
只读操作用黄色图标，系统路径写入操作用红色图标。

### 路径允许模式

| 模式 | 范围 | 是否持久化？ |
|------|------|-------------|
| **允许本次** | 当前应用会话 | 否——重启后重置 |
| **永久允许** | 保存到 QSettings | 是——重启后仍有效 |

### 操作确认

**所有任务计划均需用户确认后才执行。** CLI 交互模式输入 `/confirm` 或 `/cancel`，CLI
ask 模式响应内联 `[Y/n]` 提示，GUI 弹出确认对话框。CLI ask 模式可用 `--yes`
标志跳过 Tier 2 警告（适合脚本）。Tier 1（危险命令）不会被 `--yes` 绕过。

### 重置永久路径批准

当你为某个路径选择"永久允许"后，该设置会保存到 QSettings 并在应用重启后保持。重置方法：

- **GUI**：打开设置 → 安全选项卡，在列表中找到对应路径并删除
- **CLI / 手动**：从 QSettings 中删除 `SafetyChecker/PersistentlyAllowedPaths` 键：
  - macOS: `~/Library/Preferences/LocalAIAssistant.plist`（NativeFormat）或
    `~/.config/LocalAIAssistant/Settings.conf`（IniFormat）
  - Windows: 注册表 `HKEY_CURRENT_USER\Software\LocalAIAssistant\Settings`
  - Linux: `~/.config/LocalAIAssistant/Settings.conf`

---

## 主题

设置 → 通用中提供三种主题：

- **亮色**：清爽白色界面
- **暗色**：护眼暗色模式
- **跟随系统**：自动跟随系统偏好

---

## 常见问题

### macOS 无法打开应用

右键点击 App →「打开」。仅首次需要。

### AI 无响应

检查设置中的 API 地址、API 密钥和网络连接。

### 非流式模式导致 CLI 输出缓慢

如果 CLI 在 TASK_PLAN 请求期间似乎卡住，检查设置 → 通用中是否启用了**流式输出**。
非流式模式下，AI 的完整响应一次性到达，可能需要 30-60 秒且没有进度提示。
启用流式输出以获得实时响应。

### 管道测试 CLI 的局限性

使用 shell 管道测试 CLI（如 `echo y | ./LocalAIAssistant-CLI ask "..."`）可能无法可靠地工作：
- 确认后，应用进入 Agent 循环，stdin 行为可能与管道冲突
- 自动化脚本请改用 `--yes` 标志：`./LocalAIAssistant-CLI ask --yes "..."`
- 交互模式（`chat`）无法通过管道测试——需使用真实终端

### AI 生成了相关文字但没有 TASK_PLAN

部分模型可能会用散文描述它们打算做什么，而不生成实际的 `[TASK_PLAN]` JSON 块。
如果你看到 AI 说"我将创建文件..."但没有执行：
- 尝试更明确地表达："使用 [TASK_PLAN] 创建文件..."
- 不同模型在 TASK_PLAN 生成能力上有差异

### AI 拒绝生成危险命令

部分 AI 模型（尤其是较新的模型）有自己的安全层，会拒绝生成 `sudo` 等命令，
即使你明确要求。这是预期行为，提供了纵深防御——应将其视为安全特性而非 bug。

### 语音功能不可用

1. 在 ⚙️ →「配置语音...」中检查讯飞凭证
2. 确认讯飞账号中已开通相应服务
3. 检查网络是否能访问讯飞服务器
