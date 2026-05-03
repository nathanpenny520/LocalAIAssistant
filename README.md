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
- **跨平台** — macOS / Windows / Linux

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
> - **Linux**: 语音功能依赖 PulseAudio，部分发行版可能需要额外配置

## 技术栈

| 项目 | 技术 |
|------|------|
| 语言 | C++17 |
| 框架 | Qt 6.x (Widgets, Network, Multimedia, WebSockets) |
| 构建 | CMake 3.16+ |
| PDF解析 | Poppler 26.x |
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
│   └── build.sh        # 统一跨平台构建脚本
├── translations/       # 国际化翻译文件
├── resources/          # 资源文件（图标、配置）
└── cmake/              # CMake 配置模板
```

---

## 编译步骤

### 1. 安装依赖

| 软件 | 版本 | macOS | Windows | Linux |
|------|------|-------|---------|-------|
| C++编译器 | C++17 | Xcode CLT | MinGW（Qt自带）或 MSVC | GCC 9+ |
| Qt | 6.x | 官网或 Homebrew | 官网安装（MinGW 或 MSVC） | 包管理器 |
| CMake | 3.16+ | `brew install cmake` | 官网下载 | `sudo apt install cmake` |
| Poppler | 26.x | `brew install poppler` | MSYS2 或 vcpkg | `sudo apt install poppler` |

#### macOS 快速安装

```bash
# 安装 Xcode 命令行工具
xcode-select --install

# 安装 Homebrew（如未安装）
# 参考: https://brew.sh

# 安装依赖
brew install cmake qt@6 poppler
```

#### Linux 快速安装 (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-base-dev-tools libpoppler-cpp-dev
```

#### Windows 快速安装

**方式一：MinGW（推荐，无需 Visual Studio）**

1. 安装 **Git for Windows**：https://git-scm.com/download/win（包含 Git Bash）
2. 安装 **CMake**：https://cmake.org/download/
3. 安装 **Qt 6**：https://www.qt.io/download
   - 选择 `Qt 6.x.x for MinGW 11.2 64-bit`（Qt 自带编译器，无需额外安装）
4. 安装 **Poppler**：通过 MSYS2 (`pacman -S poppler`) 或 vcpkg

**方式二：MSVC（需 Visual Studio）**

1. 安装 **Visual Studio 2019+**（含 C++ 开发工具）
2. 安装 **CMake**：https://cmake.org/download/
3. 安装 **Qt 6**：https://www.qt.io/download
   - 选择 `Qt 6.x.x for MSVC 2019 64-bit`
4. 安装 **Poppler**：通过 MSYS2 或 vcpkg

> **提示**：MinGW 版本更轻量，Qt 安装包自带编译器；MSVC 版本调试体验更好。

### 2. 编译项目

**macOS / Linux / Windows (Git Bash)**：
```bash
cd scripts
./build.sh
```

> **Windows 提示**：脚本会自动检测 Qt 和 MinGW 编译器路径，并添加到 PATH 中，无需手动配置环境变量。

**Windows PowerShell / CMD（备选方案）**：
```powershell
# 需要在 Git Bash 中运行 build.sh
# 或直接使用 cmake 命令：
cmake -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2019_64"
cmake --build build --config Release
```

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
# 进入交互式聊天（双击 CLI 也可直接进入）
./build/LocalAIAssistant-CLI

# 或显式指定 chat 命令
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

### 推荐方案：Ollama（本地部署）

1. 下载安装 Ollama：https://ollama.com/download
2. 下载模型：`ollama pull llama3`
3. 在程序设置中配置：
   - API URL: `http://127.0.0.1:11434`
   - 模型名: `llama3`

### 其他支持的服务

| 服务 | API URL | 说明 |
|------|---------|------|
| LM Studio | `http://127.0.0.1:1234` | 图形界面管理模型 |
| OpenAI | `https://api.openai.com` | 需要 API Key |
| 硅基流动 | `https://api.siliconflow.cn` | 国内 API 代理服务 |
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
| **语音听写** | 流式版（WebSocket） | 语音转文字 |
| **语音合成** | 在线版（WebSocket） | 文字转语音 |

### 第三步：获取 API 凭证

创建应用后，在控制台获取三个凭证：

```
APPID     - 应用ID
API Key   - API密钥
API Secret - API密钥密文
```

### 第四步：配置凭证

**方式一：.env 文件（推荐，不写入系统）**

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

> **安全提示**：`.env` 文件已在 `.gitignore` 中，不会被提交到 Git。请勿将凭证写入系统环境变量或提交到代码仓库。

**方式二：环境变量（备选）**

将凭证添加到环境变量配置文件：

```bash
# macOS / Linux - 添加到 ~/.zshrc 或 ~/.bashrc
export XFYUN_APP_ID="你的APPID"
export XFYUN_API_KEY="你的APIKey"
export XFYUN_API_SECRET="你的APISecret"

# 应用环境变量
source ~/.zshrc  # 或重启终端
```

```powershell
# Windows PowerShell - 添加到用户环境变量
[System.Environment]::SetEnvironmentVariable('XFYUN_APP_ID', '你的APPID', 'User')
[System.Environment]::SetEnvironmentVariable('XFYUN_API_KEY', '你的APIKey', 'User')
[System.Environment]::SetEnvironmentVariable('XFYUN_API_SECRET', '你的APISecret', 'User')

# 重启 PowerShell 或重启电脑生效
```

### 讯飞免费额度

讯飞为开发者提供免费额度：

| 服务 | 免费额度 | 说明 |
|------|----------|------|
| 语音听写 | 每日 500 次 | 足够个人日常使用 |
| 语音合成 | 每日 200 次 | 足够个人日常使用 |

超出额度需购买付费套餐。

### TTS 音色选择

可通过修改配置选择不同女声音色：

| 音色参数 | 名称 | 特点 |
|----------|------|------|
| `x6_wumeinv_pro` | 超拟人女声 | 自然逼真、情感丰富 ⭐推荐 |
| `xiaoyan` | 晓燕 | 温柔亲和、年轻女声 |
| `aisxping` | 小萍 | 温柔甜美 |
| `aisjinger` | 婧儿 | 活泼可爱 |

---

## 使用 AI 女友模块

### 打开 AI 女友窗口

在主窗口菜单中选择「AI女友」，或使用快捷键 `Ctrl/Cmd+G`。

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

编辑 `src/girlfriend/personality.md` 可自定义 AI 女友的性格和回复风格：

```markdown
# 角色设定
你是小清，温柔体贴的AI女友，19岁。

## 回复规则
1. 回复要短！控制在30字以内
2. 多用口语化表达：嗯嗯、呀、啦、哼、~
3. 偶尔撒娇，说"哼！"或"不理你了"
...
```

修改后需要重新编译或将文件复制到应用资源目录。

### 记忆系统工作原理

AI 女友的记忆系统通过文本标记实现，无需 API 工具调用支持：

**记忆标记格式**：
```
[更新记忆:分类|内容]
```

**分类类型**：
| 分类 | 说明 | 示例 |
|------|------|------|
| `basic_info` | 基本信息 | 姓名、职业、年龄 |
| `preferences` | 喜好偏好 | 兴趣爱好、喜欢的食物 |
| `events` | 重要事件 | 考试、面试、约会 |
| `reminders` | 特别提醒 | 用户希望记住的事项 |

**工作流程**：
1. 用户透露个人信息（如"我叫小明，是程序员"）
2. AI 在回复中输出记忆标记：`[更新记忆:basic_info|昵称是小明]`
3. 系统自动解析标记，写入 `memory.md`
4. 下次对话时 AI 会参考记忆内容

**注意事项**：
- 记忆功能依赖 AI 正确输出标记，部分模型可能不支持
- 若记忆未被记录，可在 `personality.md` 中强调记忆规则
- 清空对话历史不会影响已保存的记忆

---

## 安装依赖补充说明

### Qt 6 WebSockets 模块

AI 女友语音功能需要 Qt WebSockets 模块。

**macOS（Qt 官方安装）**：
1. 打开 `/Applications/Qt/MaintenanceTool.app`
2. 选择「Add or remove components」
3. 找到 Qt 6.x → Additional Libraries
4. 勾选「Qt WebSockets」
5. 点击安装

**Linux（包管理器）**：
```bash
# Ubuntu/Debian
sudo apt install qt6-websockets-dev

# Fedora
sudo dnf install qt6-qtwebsockets-devel

# Arch Linux
sudo pacman -S qt6-websockets
```

**Windows（Qt 官方安装）**：
同 macOS，在 Qt Maintenance Tool 中勾选 WebSockets。

---

## 开发环境说明

### Qt 版本要求

- 最低版本：Qt 6.2
- 推荐版本：Qt 6.5+ 或最新稳定版

### 编译器要求

- **C++17 支持**（必需）
- macOS：AppleClang 10.0+（Xcode 10+）
- Windows：MSVC 2019+ 或 MinGW GCC 9+
- Linux：GCC 9+ 或 Clang 10+

### CMake 配置选项

程序使用以下 CMake 配置：

```cmake
# Qt 路径（必需）
-DCMAKE_PREFIX_PATH=/path/to/qt

# 构建类型
-DCMAKE_BUILD_TYPE=Release  # 或 Debug

# PDF 支持（自动检测 Poppler）
# 如未安装 Poppler，PDF 功能将被禁用
```

---

## 数据存储位置

| 平台 | 会话数据 | AI 女友数据 | 配置文件 |
|------|----------|-------------|----------|
| macOS | `~/Library/Application Support/LocalAIAssistant/` | `~/Library/Application Support/LocalAIAssistant/girlfriend/` | QSettings (plist) |
| Windows | `%APPDATA%/LocalAIAssistant/` | `%APPDATA%/LocalAIAssistant/girlfriend/` | 注册表 |
| Linux | `~/.local/share/LocalAIAssistant/` | `~/.local/share/LocalAIAssistant/girlfriend/` | `~/.config/` |

### AI 女友数据文件

| 文件 | 内容 |
|------|------|
| `girlfriend_session.json` | 对话历史 + 情绪状态 |
| `memory.md` | 用户记忆档案（基本信息、喜好、事件） |

---

## 常见问题

### 语音功能不工作

**问题**：点击语音按钮提示「语音未配置」

**解决**：
1. 检查环境变量是否设置：`echo $XFYUN_APP_ID`
2. 确认已在讯飞控制台开通「语音听写」和「语音合成」服务
3. 确认 Qt WebSockets 模块已安装
4. 重启终端或重新编译应用

**Windows 语音输入问题**：

当前版本语音输入（ASR）在 Windows 上暂不支持，这是由于 Windows Media Foundation 音频子系统与 Qt 6 QAudioSource 的兼容性问题。后续版本会尝试修复。

临时解决方案：
- 使用文字输入代替语音输入
- 语音播报（TTS）功能正常可用

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

## 更新日志

### v1.1.0 (2026-05)
- 新增 AI 女友模块：情绪系统、记忆系统、语音交互
- 新增超拟人语音合成支持
- 新增调试模式（`--debug` 参数）
- 优化语音输入架构：使用 QAudioSource 替代 QMediaRecorder
- 修复声道转换问题（多声道转单声道）
- 修复 macOS 语音输入兼容性
- Windows 语音输入暂不支持（待后续修复）

---

## 许可证

MIT License

---

## 作者

大作业项目