# 本地AI助手 (LocalAIAssistant)

一个基于 Qt 6 的跨平台 AI 助手桌面应用，支持 GUI 和 CLI 双模式，内置 AI 女友语音交互模块。

![Demo](AIGirlfriend/demo.png)

## 功能特点

### 核心功能
- **双模式支持** — GUI 图形界面 + CLI 命令行
- **文件上传** — 支持文本、图片、PDF 文件附件
- **流式输出** — SSE 实时显示，AI 回复逐字呈现
- **会话管理** — 多会话切换、历史持久化
- **多语言** — 简体中文 / English 切换
- **主题切换** — 亮色 / 暗色 / 跟随系统
- **跨平台** — macOS / Windows / Linux（暂未进行测试，可能不可用）

### AI 女友模块 🎀
- **独立窗口** — 沉浸式全屏头像背景
- **情绪系统** — 11种表情实时切换（开心、害羞、爱意、撒娇等）
- **记忆系统** — 通过文本标记自动记录用户信息，长期记忆持久化
- **语音交互** — 语音输入（ASR）+ 语音播报（TTS）
- **人设定制** — 可修改 personality.md 自定义性格
- **快捷键** — Command/Ctrl+G 快速打开/关闭女友窗口

> ⚠️ **平台兼容性说明**：
> - **macOS**: 语音输入/输出完整支持 ✅
> - **Windows**: 语音输出（TTS）正常，语音输入（ASR）暂不支持 ⚠️
> - **Linux**: 作者没有Linux笔记本，暂未测试具体实现情况

## 技术栈

| 项目 | 技术 |
|------|------|
| 语言 | C++17 |
| 框架 | Qt 6.x (Widgets, Network, Multimedia, WebSockets) |
| 构建 | CMake 3.16+ |
| PDF解析 | Poppler 26.x（不安装则上传文件时不支持PDF解析） |
| 语音服务 | 讯飞开放平台 (WebSocket API) |

## 项目结构

```
sourcecode-ai-assistant/
├── src/
│   ├── core/           # 核心业务逻辑（网络请求、会话管理、文件处理）
│   ├── ui/             # GUI 界面（主窗口、设置对话框）
│   ├── cli/            # CLI 命令行界面
│   └── girlfriend/     # AI 女友模块
│       ├── girlfriendwindow.cpp  # 女友窗口
│       ├── avatarwidget.cpp       # 头像/表情组件
│       ├── personalityengine.cpp  # 人设引擎、情绪检测
│       ├── voicemanager.cpp       # 语音管理（讯飞 ASR/TTS）
│       ├── memorymanager.cpp      # 长期记忆管理
│       ├── personality.md         # 人设 Prompt（可自定义）
│       └── voice_config.json      # 语音配置模板
├── AIGirlfriend/       # 表情图片资源（11张）
├── scripts/            # 构建脚本
│   ├── build.sh        # 统一跨平台构建脚本
│   └── setup.sh        # 首次克隆初始化脚本
├── translations/       # 国际化翻译文件
├── resources/          # 资源文件（图标、配置）
├── cmake/              # CMake 配置模板
├── .gitattributes      # Git 换行符配置
├── .env.example        # 讯飞语音凭证模板
└── README.md           # 项目说明文档
```

---

## 编译步骤

### 1. 安装依赖

| 软件 | 版本 | macOS | Windows | Linux |
|------|------|-------|---------|-------|
| C++编译器 | C++17 | Xcode CLT | MinGW（Qt自带）或 MSVC | GCC 9+ |
| Qt | 6.x | 官网或 Homebrew | 官网安装（MinGW 或 MSVC） | 包管理器 |
| Qt Multimedia | ⚠️ 需额外勾选 | Homebrew 自动安装 | Qt Maintenance Tool 勾选 | `qt6-multimedia-dev` |
| Qt WebSockets | ⚠️ 需额外勾选 | Homebrew 自动安装 | Qt Maintenance Tool 勾选 | `qt6-websockets-dev` |
| CMake | 3.16+ | `brew install cmake` | 官网下载 | `sudo apt install cmake` |
| Poppler | 26.x | `brew install poppler` | MSYS2 或 vcpkg | `sudo apt install poppler` |

> **Qt 模块说明**：Multimedia 和 WebSockets 需在 Qt Maintenance Tool 中额外勾选（语音功能必需）

#### macOS 快速安装

```bash
# 安装 Xcode 命令行工具
xcode-select --install

# 安装 Homebrew（如未安装）
# 参考: https://brew.sh

# 安装依赖（qt@6 已包含 Multimedia 和 WebSockets）
brew install cmake qt@6 poppler

# 注：官网安装 Qt 时需在 Maintenance Tool 中额外勾选 Multimedia 和 WebSockets
```

#### Linux 快速安装 (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-base-dev-tools qt6-multimedia-dev qt6-websockets-dev libpoppler-cpp-dev
```

#### Windows 快速安装

**方式一：MinGW（推荐，无需 Visual Studio）**

1. 安装 **Git for Windows**：https://git-scm.com/download/win（包含 Git Bash）
2. 安装 **CMake**：https://cmake.org/download/
3. 安装 **Qt 6**：https://www.qt.io/download
   - 选择 `Qt 6.x.x for MinGW 11.2 64-bit`（Qt 自带编译器，无需额外安装其他编译器如visual studio）
   - ⚠️ **重要**：在 Qt Maintenance Tool 中额外勾选 **Qt Multimedia** 和 **Qt WebSockets**（语音功能必需）
4. 安装 **Poppler**：通过 MSYS2 (`pacman -S poppler`) 或 vcpkg 

**方式二：MSVC（需 Visual Studio）**

1. 安装 **Visual Studio 2019+**（含 C++ 开发工具）
2. 安装 **CMake**：https://cmake.org/download/
3. 安装 **Qt 6**：https://www.qt.io/download
   - 选择 `Qt 6.x.x for MSVC 2019 64-bit`
   - ⚠️ **重要**：在 Qt Maintenance Tool 中额外勾选 **Qt Multimedia** 和 **Qt WebSockets**（语音功能必需）
4. 安装 **Poppler**：通过 MSYS2 或 vcpkg

> **提示**：MinGW 版本更轻量，Qt 安装包自带编译器；MSVC 版本调试体验更好。

### 2. 编译项目

```bash
cd scripts
./build.sh
```

> **Windows 提示**：
> - 需在 **Git Bash** 中运行（安装 Git for Windows 时自带）
> - 脚本会自动检测 Qt 和 MinGW 编译器路径，无需手动配置环境变量

### 编译选项

```bash
# 清理后重新编译
./build.sh -c

# 编译 Debug 版本
./build.sh -d

# 指定 Qt 路径
./build.sh -q /path/to/qt

# 仅编译 CLI 版本
./build.sh LocalAIAssistant-CLI

# 仅编译 GUI 版本
./build.sh LocalAIAssistant

# 查看帮助
./build.sh help
```

### 编译产物

| 平台 | GUI | CLI |
|------|-----|-----|
| macOS | `build/LocalAIAssistant.app` | `build/LocalAIAssistant-CLI` |
| Windows | `build/LocalAIAssistant.exe` | `build/LocalAIAssistant-CLI.exe` |
| Linux | `build/LocalAIAssistant` | `build/LocalAIAssistant-CLI` |

---

## 使用方法

### 运行 GUI 版本

```bash
# macOS
open build/LocalAIAssistant.app

# Windows
build\LocalAIAssistant.exe

# Windows 调试模式（显示日志控制台）
build\LocalAIAssistant.exe --debug

# Linux
./build/LocalAIAssistant
```

> **Windows 调试提示**：使用 `--debug` 参数可显示调试控制台窗口，查看运行日志。也可设置环境变量 `LOCALAI_DEBUG=1` 启用。

### 运行 CLI 版本

```bash
# macOS / Linux
./build/LocalAIAssistant-CLI

# Windows (Git Bash)
./build/LocalAIAssistant-CLI.exe

# Windows (CMD/PowerShell)
build\LocalAIAssistant-CLI.exe
```

**CLI 命令示例**：

```bash
# 进入交互式聊天
./build/LocalAIAssistant-CLI chat

# 单次查询
./build/LocalAIAssistant-CLI ask "什么是人工智能？"

# 会话管理
./build/LocalAIAssistant-CLI sessions -l    # 列出会话
./build/LocalAIAssistant-CLI sessions -n    # 新建会话

# 配置管理
./build/LocalAIAssistant-CLI config --show-config
./build/LocalAIAssistant-CLI config --api-url "http://127.0.0.1:11434"
```

### CLI 交互命令

在 CLI 聊天模式下可使用：

| 命令 | 功能 |
|------|------|
| `/help` | 显示帮助 |
| `/new` | 新建会话 |
| `/list` | 列出所有会话 |
| `/switch <id>` | 切换会话 |
| `/delete <id>` | 删除会话 |
| `/config` | 显示配置 |
| `/file <path>` | 添加文件附件 |
| `/listfiles` | 查看待发送文件 |
| `/clearfiles` | 清空文件列表 |
| `/exit` | 退出程序 |

---

## 配置 AI 服务

本程序需要连接 AI 服务才能工作。

### 本地部署：Ollama（部分功能可能不支持）

1. 下载安装 Ollama：https://ollama.com/download
2. 下载模型：`ollama pull llama3`
3. 在程序设置中配置：
   - API URL: `http://127.0.0.1:11434`
   - 模型名: `llama3`

### 使用云端API（推荐，已验证）

| 服务 | API URL | 说明 |
|------|---------|------|
| LM Studio | `http://127.0.0.1:1234` | 图形界面管理模型 |
| OpenAI | `https://api.openai.com` | 需要 API Key |
| 并行科技 | `https://llmapi.paratera.com` | 国内 API 代理服务 |
| 其他 OpenAI 兼容服务 | 按服务商文档配置 | — |

---

## AI 女友模块配置

AI 女友模块提供语音交互体验，需要配置讯飞语音服务。

### 第一步：注册讯飞开放平台账号

1. 访问讯飞开放平台：https://www.xfyun.cn
2. 注册账号并登录
3. 进入「控制台」→「创建应用」

### 第二步：开通语音服务

在应用中开通以下服务：

| 服务 | 名称 | 用途 |
|------|------|------|
| **语音听写（识别）** | 流式版（WebSocket） | 语音转文字 |
| **语音合成** | 超拟人版（WebSocket） | 文字转语音 |

### 第三步：获取 API 凭证

创建应用后，在控制台获取三个凭证：

```
APPID     - 应用ID
API Key   - API密钥
API Secret - API密钥密文
```

### 第四步：配置凭证

在项目根目录创建 `.env` 文件：

```bash
# 复制模板
cp .env.example .env

# 编辑填入你的凭证
```

`.env` 文件内容：
```
XFYUN_APP_ID=你的APPID
XFYUN_API_KEY=你的APIKey
XFYUN_API_SECRET=你的APISecret
```

> **安全提示**：`.env` 文件已在 `.gitignore` 中，不会被提交到 Git。

### TTS 音色选择

可通过修改.env中TTS音色选择不同音色，如：

| 音色参数 | 名称 | 特点 |
|----------|------|------|
| `x6_wumeinv_pro` | 妩媚姐姐 | 自然逼真、情感丰富 ⭐推荐，但需要自己添加 |
| `x6_lingfeiyi_pro` | 聆飞逸  | 青春温暖，男声 ⭐推荐，开通后自带 |

---

## 使用 AI 女友模块

### 打开 AI 女友窗口

在窗口view（视图）中选择「AI女友」，或使用快捷键 `Ctrl/Cmd+G`。

### 语音交互流程

```
1. 点击 🎤 按钮开始录音（按钮变红 🔴）
2. 对着麦克风说话
3. 再次点击按钮停止录音
4. 等待识别完成，文字自动发送
5. AI 回复后自动语音播报
```

> **Windows 用户注意**：当前版本语音输入（ASR）在 Windows 上暂不可用。您仍可使用文字输入，语音播报（TTS）功能正常。

### 自定义人设

编辑 `src/girlfriend/personality.md` 可自定义 AI 女友的性格和回复风格。
修改后需要重新编译或将文件复制到应用资源目录。

### 记忆系统工作原理

AI 女友的记忆系统通过文本标记实现（在personality.md中通过系统提示词定义实现**记忆系统**这部分代码不建议删去），无需 API 工具调用支持。

---

## 安装依赖补充说明

### Qt 6 Multimedia 和 WebSockets 模块

AI 女友语音功能需要 Qt Multimedia（音频录制/播放）和 Qt WebSockets（讯飞 API 连接）模块。

**macOS（Qt 官方安装）**：
1. 打开 `/Applications/Qt/MaintenanceTool.app`
2. 选择「Add or remove components」
3. 找到 Qt 6.x → Additional Libraries
4. 勾选「Qt Multimedia」和「Qt WebSockets」
5. 点击安装

> **注**：Homebrew 安装的 `qt@6` 已自动包含这两个模块。

**Linux（包管理器）**：
```bash
# Ubuntu/Debian
sudo apt install qt6-multimedia-dev qt6-websockets-dev

# Fedora
sudo dnf install qt6-qtmultimedia-devel qt6-qtwebsockets-devel

# Arch Linux
sudo pacman -S qt6-multimedia qt6-websockets
```

**Windows（Qt 官方安装）**：
同 macOS，在 Qt Maintenance Tool 中勾选 Multimedia 和 WebSockets。

---

## 开发环境说明

### Qt 版本要求

- 最低版本：Qt 6.10.3
- 推荐版本：Qt 6.10.3

### 编译器要求

- **C++17 支持**（必需）
- macOS：AppleClang 10.0+（Xcode 10+）
- Windows：MSVC 2019+ 或 MinGW GCC 9+（Qt 自带）
- Linux：GCC 9+ 或 Clang 10+

---

## 数据存储位置

AI 女友数据文件存储在用户数据目录的 `girlfriend/` 子目录中：

| 平台 | 数据目录完整路径 |
|------|------------------|
| macOS | `~/Library/Application Support/LocalAIAssistant/girlfriend/` |
| Windows | `%APPDATA%\LocalAIAssistant\girlfriend\` |
| Linux | `~/.local/share/LocalAIAssistant/girlfriend/` |

子目录中的文件：

| 文件 | 内容 |
|------|------|
| `girlfriend_session.json` | 对话历史 + 情绪状态 |
| `memory.md` | 用户记忆档案（基本信息、喜好、事件） |

---

## 常见问题

### 语音功能不工作

**问题**：点击语音按钮提示「语音未配置」

**解决**：
1. 检查 `.env` 文件是否存在且凭证正确
2. 确认已在讯飞控制台开通「语音听写」和「超拟人语音合成」服务
3. 确认 Qt Multimedia 和 WebSockets 模块已安装
4. 重新编译应用

**Windows 语音输入问题**：

当前版本语音输入（ASR）在 Windows 上暂不支持，这是由于 Windows Media Foundation 音频子系统与 Qt 6 QAudioSource 的兼容性问题。后续版本会尝试修复。

临时解决方案：
- 使用文字输入代替语音输入
- 但语音播报（TTS）功能应该正常可用

### 编译找不到 WebSockets

**问题**：`Could NOT find Qt6WebSockets`

**解决**：安装 Qt WebSockets 模块（见上方「安装依赖补充说明」）

### 讯飞 API 报错

**问题**：语音识别返回错误码

**常见错误码**：
| 错误码 | 原因 | 解决方案 |
|--------|------|----------|
| 10005 | API Key 错误 | 检查凭证是否正确 |
| 10006 | 无效参数 | 检查 APPID 格式 |
| 10007 | 非法参数 | 检查 API Secret |
| 10010 | 无授权 | 开通相应服务 |
| 10014 | 引擎未开通 | 在控制台开通语音服务 |
| 10700 | 引擎错误 | 联系讯飞技术支持 |

### AI 回复太长像客服

**问题**：回复超过 50 字，语气机械

**解决**：编辑 `personality.md` 调整人设，确保包含：
- 回复长度限制（30字以内）
- 口语化表达规则
- 禁止使用"您"、"根据我的理解"等客服用语

### 表情不切换

**问题**：头像表情始终是默认状态

**解决**：
1. 检查 AI 回复是否包含 `[情绪:xxx]` 标记
2. 确认 `AIGirlfriend/` 目录图片完整（11张）
3. 查看控制台日志确认情绪检测是否触发

### 记忆未被记录

**问题**：AI 没有记住之前透露的信息

**解决**：
1. 检查 `memory.md` 文件是否有内容（位于用户数据目录）
2. 确认 AI 回复中是否包含 `[更新记忆:xxx]` 标记
3. 部分模型不支持输出特殊标记，可尝试更换模型
4. 在 `personality.md` 中强调记忆规则，引导 AI 输出标记

---

## 许可证

MIT License

---

