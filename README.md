# 本地AI助手和AI女友

**中文** | [English](README_EN.md)

一个基于 Qt 6 的跨平台 AI 助手桌面应用，支持 GUI 和 CLI 双模式，内置 AI 女友语音交互模块。

GitHub仓库地址：https://github.com/nathanpenny520/LocalAIAssistant.git
Gitee 仓库地址：https://gitee.com/nathanpenny520/LocalAIAssistant.git

![Level 1 Demo](AIGirlfriend/level-1-belle/demo-belle.png)

![Level 2 Demo](AIGirlfriend/level-2-hot/demo-hot.png)

## 功能特点

### 本地AI助手核心功能

- **双模式支持** — GUI 图形界面 + CLI 命令行
- **文件上传** — 支持文本、图片（需要模型是识图模型）、PDF 文件附件
- **流式输出** — SSE 实时显示，AI 回复逐字呈现
- **会话管理** — 多会话切换、历史持久化
- **多语言** — 简体中文 / English 切换
- **主题切换** — 亮色 / 暗色 / 跟随系统
- **跨平台** — macOS / Windows / Linux

### AI 女友模块 🎀

- **独立窗口** — 沉浸式全屏头像背景，9:16 窗口比例
- **头像等级系统** — 三种等级可选：
    - Level 1 (Belle): PNG 静态图片，经典风格
    - Level 2 (Hot): PNG 静态图片，更加火辣
    - Level 3 (Hotter): MP4 动态视频，跃然屏上
- **情绪系统** — 14种表情实时切换（开心、害羞、爱意、撒娇、哭泣、旅行等）
- **心情值显示** — 左上角实时显示心情进度条和百分比
- **心情影响等级** — 可设置情绪检测的心情影响程度（低/中/高）
- **记忆系统** — 通过文本标记自动记录用户信息，长期记忆持久化
- **多会话管理** — 创建、切换、删除多个独立会话
- **语音交互** — 语音输入（ASR）+ 语音播报（TTS）
- **人设定制** — 可修改 personality.md 自定义性格
- **语音输出开关** — 可在设置中开启/关闭语音播报
- **视频声音开关** — Level 3 视频模式下可开启/关闭背景声音
- **快捷键** — Command/Ctrl+G 快速打开/关闭女友窗口

### 知识库模块 📚

- **文档导入** — 支持 TXT/MD/PDF/DOCX 文件导入，自动切分和向量化
- **语义检索** — 基于向量相似度的智能搜索
- **嵌入模型** — 支持 ONNX Runtime 本地推理，无需联网
- **HNSW 索引** — 高性能近似最近邻搜索
- **异步导入** — 后台线程处理，不阻塞 UI 操作
- **记忆增强** — 跨会话记忆提取、语义检索、上下文注入

#### 已知局限性：数学 PDF 支持

数学/公式密集型 PDF（如习题集、论文）的检索效果显著低于纯文本 PDF。原因来自三个环节的叠加：

| 环节             | 文件                             | 问题                                                                                                                                                                                                                |
| ---------------- | -------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **PDF 文字提取** | `src/parsers/fileparser.cpp:111` | [Poppler](https://poppler.freedesktop.org) 的 `page->text()` 只读取 PDF 文字层。公式若以矢量图形、嵌入图片或缺少 Unicode 映射的字体渲染，则完全丢失（无 OCR 能力）                                                  |
| **文本分块**     | `src/knowledge/textchunker.cpp`  | 段落拆分依赖 `\n\n+`（双换行），数学 PDF 很少产生这种分隔；回退的句子拆分仅识别 `.?!。！？`，数学内容缺少这些标点；Token 估算将数学符号按 0.25 token 计算（视为英文 ASCII），严重低估，导致整份文档被塞进一个超长块 |
| **嵌入模型**     | `src/knowledge/embedder.cpp`     | `all-MiniLM-L6-v2` 的 WordPiece 词表不含任何 LaTeX 命令（`\frac`、`\int`、`\sqrt` 等均无）；模型在自然语言句子相似度任务上训练，不具备数学语义理解                                                                  |

**适合的知识库文档**：商业计划书、技术文档、Markdown 笔记、教程等以自然语言为主的 PDF/TXT/MD/DOCX 文件。

**未来改进方向**：

- 引入 OCR（如 Tesseract）对 PDF 图片区域进行公式识别
- 增加数学感知的分块策略（按章节/公式边界分割，单换行符回退）
- 替换为数学专用嵌入模型（如
  [MathBERT](https://github.com/tbs17/MathBERT)）或支持 LaTeX 的多语言模型
- 对数学符号的 Token 估算进行修正

### 任务执行模块 🔧

- **Agent 迭代循环** — AI 观察执行结果并自主继续工作，通过 `[ITERATION_FEEDBACK]` → 新
  `[TASK_PLAN]` → 执行 → ... → `[TASK_COMPLETE]` 循环（默认最大 10 轮）
- **JSON 任务计划** — AI 以 JSON 格式生成操作计划（`create_dir`/`write_file`/`move_file`/`delete_file`/`search_files`/`shell_command`），程序自动解析并执行
- **原生文件操作** — 通过 Qt API 直接执行文件操作，无需依赖 shell（跨平台、更安全）
- **跨平台 Shell 支持** — 自动检测可用 Shell（Windows: pwsh→powershell→cmd，Unix:
  $SHELL→zsh→bash→sh）
- **三级安全架构** — Tier 1: 永久拦截（危险命令如 `sudo`、`eval`、`rm -rf /`——不可绕过，
  即使用 `--yes` 也不行）。Tier 2: 需用户确认（系统路径、白名单外路径——用户可选择
  允许本次/永久允许/拒绝，支持逐项切换）。Tier 3: 自动批准（白名单路径——直接执行）
- **命令注入防护** — 检测 eval、反引号替换、PowerShell 注入、Living-off-the-Land 攻击、
  磁盘操作、系统服务操作、防火墙关闭等 30+ 危险模式
- **操作撤销** — 支持撤销已执行的文件操作
- **用户确认** — 所有任务计划执行前均需用户审查确认。CLI 交互模式用单键（a/p/d/数字）
  逐项切换路径许可 + `/confirm` 执行。ask 模式内联 `[Y/n]` 提示。`--yes` 标志可跳过
  Tier 2 警告（Tier 1 永不被绕过）。GUI 确认对话框含逐路径按钮。

> ⚠️ **已知问题：Agent Loop 循环反馈卡死（macOS）**：
>
> Agent Loop 执行完计划后，需要将结果反馈给 AI 以继续循环（如报告操作成功或发起
> 下一步计划）。**在 macOS 上，这个反馈请求会永远挂起**，不报错、不超时、不返回数据，
> 界面卡死。
>
> **原因**：Qt 在 macOS 底层使用 NSURLSession 发送 HTTP 请求。第一次 SSE 流式请求
> （用户最初的消息）正常工作，但紧接着发送第二个 SSE 流式请求（Agent Loop 的反馈消息）
> 时，NSURLSession 存在已知缺陷——连续两个 SSE 流会挂起。代码中已添加
> `forceNonStreaming` 参数（强制关闭流式）来规避此问题，但经实测，该方案未能解决此 bug。
>
> **影响范围**：所有需要 Agent Loop 执行的任务（含文件操作的任务计划）。
> **当前状态**：未修复，待进一步调查。
> **跟踪文件**：`src/core/apiprovider.cpp:105-106`、`src/tasks/agentloop.cpp`

> ⚠️ **平台兼容性说明**：
>
> - **macOS**: 语音输入/输出完整支持 ✅
> - **Windows**: 语音输出（TTS）正常，语音输入（ASR）暂不支持 ⚠️
> - **Linux**: 语音输出（TTS）正常，语音输入（ASR）依赖系统音频设备，暂未充分测试

## 技术栈

| 项目     | 技术                                                                                    |
| -------- | --------------------------------------------------------------------------------------- |
| 语言     | C++17                                                                                   |
| 框架     | [Qt 6.x](https://www.qt.io) (Widgets, Network, Multimedia, WebSockets, Sql, Concurrent) |
| 构建     | [CMake](https://cmake.org) 3.16+                                                        |
| PDF解析  | [Poppler](https://poppler.freedesktop.org) 26.x（不安装则不支持PDF解析）                |
| DOCX解析 | [libzip](https://libzip.org) + [pugixml](https://pugixml.org)（不安装则不支持DOCX解析） |
| 嵌入模型 | [ONNX Runtime](https://onnxruntime.ai) ≥1.16（可选，不安装使用占位向量）                |
| 向量检索 | [hnswlib](https://github.com/nmslib/hnswlib)（header-only，自动包含）                   |
| 语音服务 | [讯飞开放平台](https://www.xfyun.cn) (WebSocket API)                                    |

## 项目结构

```
sourcecode-ai-assistant/
├── src/
│   ├── core/           # 核心业务逻辑（网络请求、会话管理、文件处理）
│   │   └── datamodels.h    # 数据模型定义
│   ├── ui/             # GUI 界面（主窗口、设置对话框）
│   ├── cli/            # CLI 命令行界面
│   ├── tasks/          # 任务执行模块（文件操作、安全检查、撤销、Agent 循环）
│   │   ├── taskengine.cpp/h       # 任务执行引擎（AI响应解析、计划调度）
│   │   ├── agentloop.cpp/h        # Agent 迭代循环（计划→执行→反馈→继续 循环）
│   │   ├── commandexecutor.cpp/h  # 命令执行器（原生文件操作 + Shell 命令执行）
│   │   ├── safetychecker.cpp/h    # 安全检查器（跨平台危险命令/路径检测）
│   │   ├── operationplan.cpp/h    # 操作计划定义（ShellOperation 类型）
│   │   └── operationundo.cpp/h    # 操作撤销
│   ├── knowledge/      # 知识管理模块（文本切分、向量化、检索、文档导入、记忆增强）
│   │   ├── textchunker.cpp/h      # 文本切分器
│   │   ├── embedder.cpp/h         # 文本转向量
│   │   ├── vectordb.cpp/h         # 向量数据库
│   │   ├── docimporter.cpp/h      # 文档导入器（TXT/MD/PDF/DOCX）
│   │   ├── knowledgebase.cpp/h    # 知识库管理
│   │   └── memoryenhancer.cpp/h   # 对话记忆增强器
│   └── girlfriend/     # AI 女友模块
│       ├── girlfriendwindow.cpp   # 女友窗口
│       ├── girlfriendwindow.h     # 女友窗口头文件
│       ├── avatarwidget.cpp       # 头像/表情/视频组件
│       ├── avatarwidget.h         # 头像组件头文件
│       ├── personalityengine.cpp  # 人设引擎、情绪检测、心情计算
│       ├── personalityengine.h    # 人设引擎头文件
│       ├── voicemanager.cpp       # 语音管理（讯飞 ASR/TTS）
│       ├── voicemanager.h         # 语音管理头文件
│       ├── memorymanager.cpp      # 长期记忆管理
│       ├── memorymanager.h        # 记忆管理头文件
│       ├── girlfriendsettings.cpp # 设置管理（头像等级、心情影响等）
│       ├── girlfriendsettings.h   # 设置管理头文件
│       ├── girlfriendsessionmanager.cpp # 多会话管理
│       ├── girlfriendsessionmanager.h   # 会话管理头文件
│       ├── girlfriendsession.cpp  # 单个会话数据
│       ├── girlfriendsession.h    # 会话数据头文件
│       ├── girlfriend_translations.h # 翻译辅助类
│       ├── personality.md         # 人设 Prompt（可自定义）
│       └── memory.md              # 用户记忆档案
├── AIGirlfriend/       # 头像资源目录
│   ├── level-1-belle/  # Level 1 PNG 图片
│   ├── level-2-hot/    # Level 2 PNG 图片
│   └── level-3-hotter/ # Level 3 MP4 视频
├── scripts/            # 构建脚本
│   ├── build.sh        # 统一跨平台构建脚本
│   ├── package.sh      # 跨平台打包脚本（CI 友好）
│   ├── setup.sh        # 首次克隆初始化脚本
│   ├── version.sh      # 共享版本号提取工具
│   └── cli-wrapper.sh  # macOS CLI 启动脚本（检测 iTerm2）
├── translations/       # 国际化翻译文件
├── resources/          # 资源文件
│   ├── icons/          # 应用图标（icns, ico, png）
│   ├── models/         # ONNX 嵌入模型文件
│   └── *.lproj/        # macOS 本地化字符串
├── third_party/        # 第三方库
│   └── hnswlib/        # 高性能向量检索库（header-only）
├── cmake/              # CMake 配置模板
│   ├── Info.plist.in   # GUI .app bundle 配置
│   └── CLI-Info.plist.in # CLI .app bundle 配置
├── CMakeLists.txt      # CMake 主配置文件
├── .gitattributes      # Git 换行符配置
├── .gitignore          # Git 忽略规则
├── .env.example        # 讯飞语音凭证模板
├── LICENSE             # MIT 许可证
├── README.md           # 中文说明文档
└── README_EN.md        # 英文说明文档
```

---

## 首次克隆初始化

克隆项目后，建议先运行初始化脚本检测依赖环境：

```bash
./scripts/setup.sh
```

该脚本会：

1. 自动复制 `.env.example` → `.env`（讯飞语音凭证模板）
2. 检测构建依赖（CMake、编译器、Qt、Poppler、Readline、ONNX Runtime）
3. 提示缺少的依赖及安装指南

> **提示**：运行此脚本可快速了解当前环境是否满足编译要求。

---

## 编译步骤

### 1. 安装依赖

| 软件          | 版本          | macOS                              | Windows                                                             | Linux                                 |
| ------------- | ------------- | ---------------------------------- | ------------------------------------------------------------------- | ------------------------------------- |
| C++编译器     | C++17         | Xcode CLT                          | MinGW（Qt自带）或 MSVC                                              | GCC 9+                                |
| Qt            | 6.x           | 官网或 [Homebrew](https://brew.sh) | 官网安装（MinGW 或 MSVC）                                           | 包管理器                              |
| Qt Multimedia | ⚠️ 需额外勾选 | Homebrew 自动安装                  | Qt Maintenance Tool 勾选                                            | `qt6-multimedia-dev`                  |
| Qt WebSockets | ⚠️ 需额外勾选 | Homebrew 自动安装                  | Qt Maintenance Tool 勾选                                            | `qt6-websockets-dev`                  |
| CMake         | 3.16+         | `brew install cmake`               | [官网下载](https://cmake.org/download/)                             | `sudo apt install cmake`              |
| Readline      | —             | 系统自带                           | 不适用                                                              | `sudo apt install libreadline-dev`    |
| Poppler       | 26.x          | `brew install poppler`             | [MSYS2](https://www.msys2.org) 或 [vcpkg](https://vcpkg.io)         | `sudo apt install libpoppler-cpp-dev` |
| libzip        | ≥1.5 (可选)   | `brew install libzip`              | [MSYS2](https://www.msys2.org) 或 [vcpkg](https://vcpkg.io)         | `sudo apt install libzip-dev`         |
| pugixml       | ≥1.11 (可选)  | `brew install pugixml`             | [MSYS2](https://www.msys2.org) 或 [vcpkg](https://vcpkg.io)         | `sudo apt install libpugixml-dev`     |
| ONNX Runtime  | ≥1.16 (可选)  | `brew install onnxruntime`         | [GitHub Release](https://github.com/microsoft/onnxruntime/releases) | `sudo apt install libonnxruntime-dev` |

> **Qt 模块说明**：Multimedia 和 WebSockets 需在 Qt Maintenance Tool 中额外勾选（语音功能必需）
> **可选依赖**：Readline（CLI 输入增强）、Poppler（PDF 解析）、libzip+pugixml（DOCX 解析）、ONNX
> Runtime（知识库嵌入模型），不安装不影响核心功能

#### macOS 快速安装

```bash
# 安装 Xcode 命令行工具
xcode-select --install

# 安装 Homebrew（如未安装）
# 参考: https://brew.sh

# 安装依赖（qt@6 已包含 Multimedia 和 WebSockets）
brew install cmake qt@6 poppler libzip pugixml

# 可选：安装 ONNX Runtime 启用真实嵌入推理
brew install onnxruntime

# 注：官网安装 Qt 时需在 Maintenance Tool 中额外勾选 Multimedia 和 WebSockets
```

#### Linux 快速安装 (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-base-dev-tools qt6-multimedia-dev qt6-websockets-dev libpoppler-cpp-dev libzip-dev libpugixml-dev libreadline-dev
# 可选：sudo apt install libonnxruntime-dev
```

#### Windows 快速安装

**方式一：MinGW（推荐，无需 Visual Studio）**

1. 安装 **[Git for Windows](https://git-scm.com/download/win)**（包含 Git Bash）
2. 安装 **[CMake](https://cmake.org/download/)**
3. 安装 **[Qt 6](https://www.qt.io/download)**：
    - 选择 `Qt 6.x.x for MinGW 11.2 64-bit`（Qt 自带编译器，无需额外安装其他编译器如visual studio）
    - ⚠️ **重要**：在 Qt Maintenance Tool 中额外勾选 **Qt Multimedia** 和 **Qt
      WebSockets**（语音功能必需）
4. 安装 **Poppler / libzip / pugixml**（可选，用于 PDF/DOCX 解析）：通过
   [MSYS2](https://www.msys2.org)
   (`pacman -S mingw-w64-x86_64-poppler mingw-w64-x86_64-libzip mingw-w64-x86_64-pugixml`) 或
   [vcpkg](https://vcpkg.io)

**方式二：MSVC（需 Visual Studio）**

1. 安装 **[Visual Studio 2019+](https://visualstudio.microsoft.com)**（含 C++ 开发工具）
2. 安装 **[CMake](https://cmake.org/download/)**
3. 安装 **[Qt 6](https://www.qt.io/download)**：
    - 选择 `Qt 6.x.x for MSVC 2019 64-bit`
    - ⚠️ **重要**：在 Qt Maintenance Tool 中额外勾选 **Qt Multimedia** 和 **Qt
      WebSockets**（语音功能必需）
4. 安装 **Poppler / libzip / pugixml**（可选）：通过 [MSYS2](https://www.msys2.org) 或
   [vcpkg](https://vcpkg.io)

> **提示**：MinGW 版本更轻量，Qt 安装包自带编译器；MSVC 版本调试体验更好。

### 2. 编译项目

```bash
cd scripts
./build.sh
```

> **Windows 提示**：
>
> - 需在 **Git Bash** 中运行（安装 Git for Windows 时自带）
> - 脚本会自动检测 Qt 和 MinGW 编译器路径，无需手动配置环境变量

### 编译选项

```bash
# 清理后重新编译
./build.sh -c

# 编译 Debug 版本
./build.sh -d

# 编译并打包为可分发的安装包
./build.sh build -p

# 单独打包已有构建产物
./build.sh package

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

| 平台    | GUI                          | CLI                                                              |
| ------- | ---------------------------- | ---------------------------------------------------------------- |
| macOS   | `build/LocalAIAssistant.app` | `build/LocalAIAssistant-CLI` 或 `build/LocalAIAssistant-CLI.app` |
| Windows | `build/LocalAIAssistant.exe` | `build/LocalAIAssistant-CLI.exe`                                 |
| Linux   | `build/LocalAIAssistant`     | `build/LocalAIAssistant-CLI`                                     |

> **macOS CLI .app**：双击 `LocalAIAssistant-CLI.app`
> 会自动检测 iTerm2 并优先使用它打开，解决中文输入删除问题。

### 打包分发

使用 `package` 命令生成用户可直接安装的发行包：

```bash
# 构建 + 打包一步完成
./build.sh build -p

# 或单独打包已有构建产物
./build.sh package
```

| 平台    | 格式                              | 产出路径                                      |
| ------- | --------------------------------- | --------------------------------------------- |
| macOS   | **DMG**（拖入 Applications 即用） | `release/LocalAIAssistant-x.x.x-macOS.dmg`    |
| Windows | **ZIP**（解压即用）               | `release/LocalAIAssistant-x.x.x-Windows.zip`  |
| Linux   | **tar.gz**（含 install.sh）       | `release/LocalAIAssistant-x.x.x-Linux.tar.gz` |

> **注意**：当前为免费软件，未进行代码签名。macOS 用户首次打开需右键点击 App
> →「打开」来绕过 Gatekeeper。Windows 用户运行时 SmartScreen 会警告，点击「更多信息」→「仍要运行」即可。
>
> Release 包**不包含**开发者的 `.env`
> 凭证文件，用户可通过 AI 女友窗口的设置菜单直接配置讯飞语音凭证，或参考 `.env.example`
> 模板创建自己的 `.env` 文件。

### 自动发布 Release（GitHub Actions CI）

推送版本 tag 即可触发 CI 自动编译、测试、打包，并将三平台安装包发布到 GitHub Release。

**触发条件**：推送 `v` 开头的 tag（如 `v1.0.0`）到 GitHub。

```bash
# 打 tag 并推送，自动触发发布流程
git tag v1.0.0
git push origin v1.0.0
```

**发布流程**：

```
git push v1.0.0
  → GitHub Actions 启动
    → Linux:   编译 → 测试 → 打包 tar.gz
    → macOS:   编译 → 测试 → 打包 DMG
    → Windows: 编译 → 测试 → 打包 ZIP
  → 三个平台全部通过后，自动创建 Release
  → 安装包自动上传到 Release 下载区
```

**条件**：
- 必须推送 **tag**（`v*`），普通 push 不会触发发布
- 三个平台的编译和测试**必须全部通过**，任一失败则不会发布
- 发布在 GitHub Releases 页面查看：`https://github.com/nathanpenny520/LocalAIAssistant/releases`

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

> **Windows 调试提示**：使用 `--debug` 参数可显示调试控制台窗口，查看运行日志。也可设置环境变量
> `LOCALAI_DEBUG=1` 启用。

### 运行 CLI 版本

```bash
# macOS / Linux
./build/LocalAIAssistant-CLI

# macOS .app bundle（双击运行，自动检测 iTerm2）
open build/LocalAIAssistant-CLI.app

# Windows (Git Bash)
./build/LocalAIAssistant-CLI.exe

# Windows (CMD/PowerShell)
build\LocalAIAssistant-CLI.exe
```

> **macOS 终端建议**：推荐使用 [iTerm2](https://iterm2.com)
> 替代 Terminal.app。原版 Terminal 对中文输入的删除处理可能存在问题（Backspace 删除中文字符不完整）。CLI
> .app bundle 会自动检测 iTerm2 并优先使用它打开。

> **readline 支持**：macOS 自带 readline 库，编译时自动启用，提供更好的输入体验（支持历史记录、多字节字符正确编辑）。

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

# 任务执行（自动确认）
./build/LocalAIAssistant-CLI ask --yes "帮我在 ~/test 创建 hello.txt"
```

### CLI 交互命令

在 CLI 聊天模式下可使用：

| 命令           | 功能                 |
| -------------- | -------------------- |
| `/help`        | 显示帮助             |
| `/new`         | 新建会话             |
| `/list`        | 列出所有会话         |
| `/switch <id>` | 切换会话             |
| `/delete <id>` | 删除会话             |
| `/config`      | 显示配置             |
| `/file <path>` | 添加文件附件         |
| `/listfiles`   | 查看待发送文件       |
| `/clearfiles`  | 清空文件列表         |
| `/confirm`     | 确认执行待定任务计划 |
| `/cancel`      | 取消待定任务计划     |
| `/undo`        | 撤销上次执行的操作   |
| `/exit`        | 退出程序             |

---

## 配置 AI 服务

本程序需要连接 AI 服务才能工作。

### 本地部署：[Ollama](https://ollama.com/download)（部分功能可能不支持）

1. 下载安装 Ollama：https://ollama.com/download
2. 下载模型：`ollama pull llama3`
3. 在程序设置中配置：
    - API URL: `http://127.0.0.1:11434`
    - 模型名: `llama3`

### 使用云端API（推荐，已验证）

| 服务                                 | API URL                       | 说明              |
| ------------------------------------ | ----------------------------- | ----------------- |
| [OpenAI](https://openai.com)         | `https://api.openai.com`      | 需要 API Key      |
| [并行科技](https://www.paratera.com) | `https://llmapi.paratera.com` | 国内 API 代理服务 |
| 其他 OpenAI 兼容服务                 | 按服务商文档配置              | —                 |

---

## AI 女友模块配置

AI 女友模块提供语音交互体验，需要配置讯飞语音服务。

### 方式一：应用内配置（推荐）

打开 AI 女友窗口，点击右上角 ⚙️ 按钮，选择「配置语音...」，在弹出的对话框中填入讯飞凭证即可。配置自动保存，无需编辑文件。

### 方式二：.env 文件配置（高级用户）

在项目根目录或用户数据目录创建 `.env` 文件，应用会自动读取。

### 第一步：注册[讯飞开放平台](https://www.xfyun.cn)账号

1. 访问讯飞开放平台：https://www.xfyun.cn
2. 注册账号并登录
3. 进入「控制台」→「创建应用」

### 第二步：开通语音服务

在应用中开通以下服务：

| 服务                 | 名称                  | 用途       |
| -------------------- | --------------------- | ---------- |
| **语音听写（识别）** | 流式版（WebSocket）   | 语音转文字 |
| **语音合成**         | 超拟人版（WebSocket） | 文字转语音 |

### 第三步：获取 API 凭证

创建应用后，在控制台获取三个凭证：

```
APPID     - 应用ID
API Key   - API密钥
API Secret - API密钥密文
```

### 第四步：配置凭证

**推荐：应用内配置** — 在 AI 女友窗口的 ⚙️ 设置菜单中点击「配置语音...」，直接在界面填写凭证并保存。

**备选：.env 文件** — 在项目根目录创建 `.env` 文件：

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

> **安全提示**：`.env` 文件已在 `.gitignore` 中，不会被提交到 Git。Release 包中已移除开发者
> `.env`，仅包含 `.env.example` 模板。

### TTS 音色选择

可通过修改.env中TTS音色选择不同音色，如：

| 音色参数              | 名称     | 特点                               |
| --------------------- | -------- | ---------------------------------- |
| `x6_lingxiaoxuan_pro` | 凌小璇   | 超拟人女声 ⭐默认                  |
| `x6_wumeinv_pro`      | 妩媚姐姐 | 自然逼真、情感丰富，但需要自己添加 |
| `x6_lingfeiyi_pro`    | 聆飞逸   | 青春温暖，男声 ⭐推荐，开通后自带  |

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

编辑 `src/girlfriend/personality.md`
可自定义 AI 女友的性格和回复风格。修改后需要重新编译或将文件复制到应用资源目录。

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

**Windows（Qt 官方安装）**：同 macOS，在 Qt Maintenance Tool 中勾选 Multimedia 和 WebSockets。

---

## 开发环境说明

### Qt 版本要求

- 最低版本：Qt 6.x
- 推荐版本：Qt 6.10.3

### 编译器要求

- **C++17 支持**（必需）
- macOS：AppleClang 10.0+（Xcode 10+）
- Windows：MSVC 2019+ 或 MinGW GCC 9+（Qt 自带）
- Linux：GCC 9+ 或 Clang 10+

---

## 数据存储位置

所有数据文件存储在用户数据目录下：

| 平台    | 数据目录路径                                      |
| ------- | ------------------------------------------------- |
| macOS   | `~/Library/Application Support/LocalAIAssistant/` |
| Windows | `%APPDATA%\LocalAIAssistant\`                     |
| Linux   | `~/.local/share/LocalAIAssistant/`                |

### AI 女友数据（`girlfriend/` 子目录）

| 文件                | 内容                                           |
| ------------------- | ---------------------------------------------- |
| `settings.json`     | 全局设置（头像等级、心情影响、语音输出开关等） |
| `sessions.json`     | 会话元数据列表（ID、名称、创建时间）           |
| `session_<id>.json` | 单个会话数据（对话历史、情绪状态）             |
| `memory.md`         | 用户记忆档案（基本信息、喜好、事件）           |

### 知识库数据（`knowledge/` 子目录）

| 文件            | 内容                                    |
| --------------- | --------------------------------------- |
| `chunks.db`     | SQLite 数据库，存储文档文本块和元数据   |
| `vectors.bin`   | 二进制向量索引文件                      |
| `memories.json` | 跨会话记忆条目（MemoryEnhancer 持久化） |

---

## 常见问题

### 语音功能不工作

**问题**：点击语音按钮提示「语音未配置」

**解决**：

1. 在 AI 女友窗口点击 ⚙️ →「配置语音...」填写讯飞凭证（推荐）
2. 或检查 `.env` 文件是否存在且凭证正确
3. 确认已在讯飞控制台开通「语音听写」和「超拟人语音合成」服务
4. 确认 Qt Multimedia 和 WebSockets 模块已安装

**Windows 语音输入问题**：

当前版本语音输入（ASR）在 Windows 上暂不支持，这是由于 Windows Media Foundation 音频子系统与 Qt 6
QAudioSource 的兼容性问题。后续版本会尝试修复。

临时解决方案：

- 使用文字输入代替语音输入
- 但语音播报（TTS）功能应该正常可用

### 编译找不到 WebSockets

**问题**：`Could NOT find Qt6WebSockets`

**解决**：安装 Qt WebSockets 模块（见上方「安装依赖补充说明」）

### 讯飞 API 报错

**问题**：语音识别返回错误码

**常见错误码**：| 错误码 | 原因 | 解决方案 | |--------|------|----------| | 10005 | API
Key 错误 | 检查凭证是否正确 | | 10006 | 无效参数 | 检查 APPID 格式 | | 10007 | 非法参数 | 检查 API
Secret | | 10010 | 无授权 | 开通相应服务 | | 10014 | 引擎未开通 | 在控制台开通语音服务 | | 10700
| 引擎错误 | 联系讯飞技术支持 |

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
2. 确认对应等级的 `AIGirlfriend/LevelX/` 目录资源完整
3. Level 1/2 需要 PNG 图片，Level 3 需要 MP4 视频
4. 查看控制台日志确认情绪检测是否触发

### 视频模式下UI不可见

**问题**：Level 3 视频模式下，情绪标签和设置按钮看不见

**说明**：这是 Qt
QVideoWidget 在 macOS 上使用原生窗口渲染的技术限制。当前版本暂未完全解决，建议使用 Level 1 或 Level
2 的图片模式。

### 多会话数据丢失

**问题**：切换会话后发现对话历史消失

**解决**：

1. 检查 `sessions.json` 和 `session_<id>.json` 文件是否存在
2. 确认切换会话前数据已自动保存
3. 避免手动删除会话数据文件

### 记忆未被记录

**问题**：AI 没有记住之前透露的信息

**解决**：

1. 检查 `memory.md` 文件是否有内容（位于用户数据目录）
2. 确认 AI 回复中是否包含 `[更新记忆:xxx]` 标记
3. 部分模型不支持输出特殊标记，可尝试更换模型
4. 在 `personality.md` 中强调记忆规则，引导 AI 输出标记

---

## 许可证

[MIT License](LICENSE)

---
