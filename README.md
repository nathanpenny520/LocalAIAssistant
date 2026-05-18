# 本地AI助手和AI女友

**中文** | [English](README_EN.md)

一个基于 Qt 6 的跨平台 AI 助手桌面应用，支持 GUI 和 CLI 双模式，内置 AI 女友语音交互模块。

GitHub仓库地址：https://github.com/nathanpenny520/LocalAIAssistant.git

Gitee 仓库地址：https://gitee.com/nathanpenny520/LocalAIAssistant.git

![Level 1 Demo](resources/girlfriend/level-1-belle/demo-belle.png)


## 功能特点

### 本地AI助手核心功能

- **双模式支持** — GUI 图形界面 + CLI 命令行
- **文件上传** — 支持文本、图片（需要模型是识图模型）、PDF、docx 文件附件
- **流式输出** — SSE 实时显示，AI 回复逐字呈现
- **会话管理** — 多会话切换、历史持久化
- **多语言** — 简体中文 / English 切换
- **主题切换** — 亮色 / 暗色 / 跟随系统
- **跨平台** — macOS / Windows / Linux

### AI 女友模块 🎀

- **独立窗口** — 沉浸式全屏头像背景，9:16 窗口比例
- **头像等级系统** — 三种等级可选：
    - Level 1 (Belle): PNG 静态图片，经典风格
    - Level 2 (Hot): PNG 静态图片，更加动人
    - Level 3 (Hotter): MP4 动态视频，跃然屏上
- **情绪系统** — 14种表情实时切换（开心、害羞、爱意、撒娇、哭泣、旅行等）
- **心情值显示** — 左上角实时显示心情进度条和百分比
- **心情影响等级** — 可设置情绪检测的心情影响程度（低/中/高）
- **记忆系统** — 通过文本标记自动记录用户信息，长期记忆持久化
- **多会话管理** — 创建、切换、删除多个独立会话
- **语音交互** — 语音输入（ASR）+ 语音播报（TTS）
- **人设定制** — 可修改 prompts 目录下的 girlfriend.md 自定义性格
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

## 技术栈

| 项目     | 技术                                                                                    |
| -------- | --------------------------------------------------------------------------------------- |
| 语言     | C++17                                                                                   |
| 框架     | [Qt 6.x](https://www.qt.io) (Widgets, Network, Multimedia, WebSockets, Sql, Concurrent) |
| 构建     | [CMake](https://cmake.org) 3.16+                                                        |
| PDF解析  | [Poppler](https://poppler.freedesktop.org)（不安装则不支持PDF解析）                        |
| DOCX解析 | [libzip](https://libzip.org) + [pugixml](https://pugixml.org)（不安装则不支持DOCX解析） |
| 嵌入模型 | [ONNX Runtime](https://onnxruntime.ai) ≥1.16（可选，不安装使用占位向量）                |
| 向量检索 | [hnswlib](https://github.com/nmslib/hnswlib)（header-only，自动包含）                   |
| 语音服务 | [讯飞开放平台](https://www.xfyun.cn) (WebSocket API)                                    |

## 项目结构

```
sourcecode-ai-assistant/
├── .github/
│   ├── ISSUE_TEMPLATE/
│   │   ├── bug_report.md         # Bug 报告模板
│   │   └── feature_request.md    # 功能请求模板
│   ├── PULL_REQUEST_TEMPLATE.md  # PR 模板
│   └── workflows/
│       ├── build.yml             # CI 构建与发布
│       └── format-check.yml      # 代码格式检查
├── cmake/
│   ├── Info.plist.in             # GUI .app bundle 配置
│   └── CLI-Info.plist.in         # CLI .app bundle 配置
├── docs/
│   ├── README.md                 # 文档索引
│   ├── development/              # 开发文档
│   │   ├── ROADMAP.md                 # 路线图
│   │   ├── code-improvement-plan.md   # 代码改进计划
│   │   ├── code-review-report.md      # 代码评审报告
│   │   ├── mcp-integration-plan.md    # MCP 集成方案
│   │   ├── agent-runtime-review.md    # Agent 运行时评审
│   │   └── known-issues.md            # 已知问题
│   └── user/                     # 用户文档
│       ├── USAGE.md              # 英文使用指南
│       └── USAGE_zh_CN.md        # 中文使用指南
├── resources/
│   ├── girlfriend/               # AI 女友头像资源
│   │   ├── level-1-belle/        # Level 1 PNG 图片
│   │   ├── level-2-hot/          # Level 2 PNG 图片
│   │   └── level-3-hotter/       # Level 3 MP4 视频
│   ├── icons/                    # 应用图标
│   │   ├── app.icns              # macOS 图标
│   │   ├── app.ico               # Windows 图标
│   │   ├── app.png               # Linux 图标
│   │   └── app.rc                # Windows 资源文件
│   ├── installer/
│   │   └── installer.nsi.in      # NSIS 安装包模板
│   ├── models/                   # ONNX 嵌入模型文件
│   ├── prompts/                  # AI 提示词模板（构建时同步到应用资源）
│   │   ├── en/                   #   英文提示词
│   │   │   ├── girlfriend.md     #     女友人设
│   │   │   ├── knowledge.md      #     知识库提示词
│   │   │   ├── system.md         #     系统提示词
│   │   │   └── task.md           #     任务提示词
│   │   ├── zh_CN/                #   中文提示词
│   │   │   ├── girlfriend.md     #     女友人设
│   │   │   ├── knowledge.md      #     知识库提示词
│   │   │   ├── system.md         #     系统提示词
│   │   │   └── task.md           #     任务提示词
│   │   └── girlfriend_memory.md  # 记忆增强模板
│   ├── localaiassistant.desktop  # Linux 桌面入口
│   ├── en.lproj/                 # macOS 英文本地化
│   └── zh_CN.lproj/              # macOS 中文本地化
├── scripts/
│   ├── README.md                 # 脚本使用文档
│   ├── build.sh                  # 统一跨平台构建脚本
│   ├── package.sh                # 跨平台打包脚本
│   ├── setup.sh                  # 首次克隆初始化脚本
│   ├── format.sh                 # 代码格式化工具
│   ├── version.sh                # 共享版本号提取
│   └── cli-wrapper.sh            # macOS CLI 启动器（检测 iTerm2）
├── src/
│   ├── cli/
│   │   ├── cli_main.cpp          # CLI 入口
│   │   ├── cli_application.cpp   # CLI 应用逻辑
│   │   └── cli_application.h     # CLI 应用头文件
│   ├── core/                     # 核心业务逻辑
│   │   ├── apiprovider.cpp/h     #   API 提供者基类（OpenAI/Ollama/Anthropic/LlamaCpp）
│   │   ├── networkmanager.cpp/h  #   网络请求管理（SSE 流式）
│   │   ├── sessionmanager.cpp/h  #   会话管理（CRUD、持久化）
│   │   ├── filemanager.cpp/h     #   文件附件管理
│   │   ├── envconfig.cpp/h       #   .env 配置加载
│   │   ├── datamodels.h          #   数据模型定义
│   │   ├── openai_provider.cpp/h     #   OpenAI API 适配
│   │   ├── ollama_provider.cpp/h     #   Ollama API 适配
│   │   ├── anthropic_provider.cpp/h  #   Anthropic API 适配
│   │   ├── llamacpp_provider.cpp/h   #   LlamaCpp API 适配
│   │   └── version.h.in          #   版本号模板
│   ├── prompts/                  # 提示词管理器
│   │   ├── promptmanager.cpp/h   #   提示词加载与管理
│   │   ├── en/                   #   英文提示词（源）
│   │   └── zh_CN/                #   中文提示词（源）
│   ├── parsers/                  # 文件解析器
│   │   ├── fileparser.cpp        #   PDF/DOCX/文本解析
│   │   └── fileparser.h          #   解析器头文件
│   ├── ui/                       # GUI 界面
│   │   ├── main.cpp              #   GUI 入口
│   │   ├── mainwindow.cpp/h      #   主窗口（聊天、会话、女友集成）
│   │   ├── settingsdialog.cpp/h  #   设置对话框（API、主题、语言）
│   │   ├── apptheme.cpp/h        #   主题管理（亮色/暗色/跟随系统）
│   │   ├── markdownrenderer.cpp/h    #   Markdown 渲染
│   │   ├── stylesheetmanager.cpp/h   #   Qt 样式表管理
│   │   ├── translationmanager.cpp/h  #   多语言切换
│   │   └── operationconfirmdialog.cpp/h  # 任务确认对话框
│   ├── tasks/                    # 任务执行模块
│   │   ├── taskengine.cpp/h      #   任务引擎（AI 响应解析、计划调度）
│   │   ├── agentloop.cpp/h       #   Agent 迭代循环（计划→执行→反馈→继续）
│   │   ├── commandexecutor.cpp/h #   命令执行器（原生文件操作 + Shell）
│   │   ├── safetychecker.cpp/h   #   安全检查器（三级安全架构）
│   │   ├── operationplan.cpp/h   #   操作计划定义
│   │   └── operationundo.cpp/h   #   操作撤销
│   ├── knowledge/                # 知识库模块
│   │   ├── textchunker.cpp/h     #   文本切分器
│   │   ├── embedder.cpp/h        #   文本转向量（ONNX）
│   │   ├── vectordb.cpp/h        #   向量数据库（HNSW）
│   │   ├── docimporter.cpp/h     #   文档导入器（TXT/MD/PDF/DOCX）
│   │   ├── knowledgebase.cpp/h   #   知识库管理
│   │   └── memoryenhancer.cpp/h  #   对话记忆增强器
│   └── girlfriend/               # AI 女友模块
│       ├── girlfriendwindow.cpp/h    #   女友独立窗口
│       ├── avatarwidget.cpp/h        #   头像/表情/视频组件
│       ├── personalityengine.cpp/h   #   人设引擎、情绪检测、心情计算
│       ├── voicemanager.cpp/h        #   语音管理（讯飞 ASR/TTS）
│       ├── memorymanager.cpp/h       #   长期记忆管理
│       ├── girlfriendsettings.cpp/h  #   设置管理（头像等级、心情影响等）
│       ├── girlfriendsessionmanager.cpp/h  #   多会话管理
│       ├── girlfriendsession.cpp/h   #   单个会话数据
│       └── girlfriend_translations.h #   翻译辅助类
├── tests/                        # 单元测试
│   ├── CMakeLists.txt            #   测试构建配置
│   ├── test_agentloop.cpp        #   Agent 循环测试
│   ├── test_apiprovider.cpp      #   API 提供者测试
│   ├── test_apptheme.cpp         #   主题测试
│   ├── test_commandexecutor.cpp  #   命令执行器测试
│   ├── test_embedder.cpp         #   嵌入器测试
│   ├── test_fileparser.cpp       #   文件解析器测试
│   ├── test_knowledgebase.cpp    #   知识库测试
│   ├── test_markdownrenderer.cpp #   Markdown 渲染测试
│   ├── test_safetychecker.cpp    #   安全检查器测试
│   ├── test_sessionmanager.cpp   #   会话管理测试
│   ├── test_stylesheetmanager.cpp #  样式表管理测试
│   └── test_vectordb.cpp         #   向量数据库测试
├── third_party/
│   └── hnswlib/                  # 高性能向量检索库（header-only）
├── translations/
│   ├── localai_en.ts             # 英文翻译源
│   └── localai_zh_CN.ts          # 中文翻译源
├── .clang-format                 # C++ 代码格式化配置
├── .clangd                       # clangd LSP 配置
├── .cmake-format.json            # CMake 格式化配置
├── .editorconfig                 # 编辑器通用配置
├── .gitattributes                # Git 换行符配置
├── .gitignore                    # Git 忽略规则
├── .prettierignore               # Prettier 忽略规则
├── .prettierrc                   # Prettier 格式化配置
├── .env.example                  # 讯飞语音凭证模板
├── CMakeLists.txt                # CMake 主构建文件
├── CHANGELOG.md                  # 变更日志
├── CLAUDE.md                     # Claude Code 配置
├── CONTRIBUTING.md               # 贡献指南
├── LICENSE                       # MIT 许可证
├── README.md                     # 中文说明文档
└── README_EN.md                  # 英文说明文档
```

---

## 首次克隆初始化

```bash
./scripts/setup.sh
```

检测构建依赖（CMake、编译器、Qt、Poppler 等）并提示缺失项及安装指南。

> 各脚本的详细参数见 [scripts/README.md](scripts/README.md)。

---

## 编译步骤

### 开发环境要求

- **Qt**: 最低 6.x，推荐 6.10.3
- **编译器**: 需支持 C++17（macOS: AppleClang 10.0+ / Windows: MSVC 2019+ 或 MinGW GCC 9+ / Linux: GCC 9+ 或 Clang 10+）

### 1. 安装依赖

| 软件          | 版本          | macOS                              | Windows                                                             | Linux                                 |
| ------------- | ------------- | ---------------------------------- | ------------------------------------------------------------------- | ------------------------------------- |
| C++编译器     | C++17         | Xcode CLT                          | MinGW（Qt自带）或 MSVC                                              | GCC 9+                                |
| Qt            | 6.x           | 官网或 [Homebrew](https://brew.sh) | 官网安装（MinGW 或 MSVC）                                           | 包管理器                              |
| Qt Multimedia | ⚠️ 需额外勾选 | Homebrew 自动安装                  | Qt Maintenance Tool 勾选                                            | `qt6-multimedia-dev`                  |
| Qt WebSockets | ⚠️ 需额外勾选 | Homebrew 自动安装                  | Qt Maintenance Tool 勾选                                            | `qt6-websockets-dev`                  |
| CMake         | 3.16+         | `brew install cmake`               | [官网下载](https://cmake.org/download/)                             | `sudo apt install cmake`              |
| Readline      | —             | 系统自带                           | 不适用                                                              | `sudo apt install libreadline-dev`    |
| Poppler       | —             | `brew install poppler`             | [MSYS2](https://www.msys2.org) 或 [vcpkg](https://vcpkg.io)         | `sudo apt install libpoppler-cpp-dev` |
| libzip        | ≥1.5 (可选)   | `brew install libzip`              | [MSYS2](https://www.msys2.org) 或 [vcpkg](https://vcpkg.io)         | `sudo apt install libzip-dev`         |
| pugixml       | — (可选)      | `brew install pugixml`             | [MSYS2](https://www.msys2.org) 或 [vcpkg](https://vcpkg.io)         | `sudo apt install libpugixml-dev`     |
| ONNX Runtime  | ≥1.16 (可选)  | `brew install onnxruntime`         | [GitHub Release](https://github.com/microsoft/onnxruntime/releases) | `sudo apt install libonnxruntime-dev` |

> **Linux 发行版说明**：表格中 `apt` 为 Ubuntu/Debian。Fedora 用户请用 `dnf install qt6-qtmultimedia-devel qt6-qtwebsockets-devel libpoppler-cpp-devel libzip-devel libpugixml-devel`，Arch 用户请用 `pacman -S qt6-multimedia qt6-websockets poppler libzip pugixml`。
>
> **Qt 模块说明**：Multimedia 和 WebSockets 需在 Qt Maintenance Tool 中额外勾选（语音功能必需）。
> **可选依赖**：Readline（CLI 输入增强）、Poppler（PDF 解析）、libzip+pugixml（DOCX 解析）、ONNX
> Runtime（知识库嵌入模型），不安装不影响核心功能。

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
sudo apt install build-essential cmake qt6-base-dev qt6-base-dev-tools qt6-multimedia-dev qt6-websockets-dev libpoppler-cpp-dev libzip-dev libpugixml-dev
# 可选：sudo apt install libreadline-dev       # CLI 输入增强
# 可选：sudo apt install libonnxruntime-dev    # 知识库嵌入推理
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

> **提示**：MinGW 版本更轻量，Qt 安装包自带编译器；MSVC 版本调试体验更好。推荐使用MinGW。

### 2. 编译项目

```bash
cd scripts && ./build.sh
```

> 各脚本的详细参数见 [scripts/README.md](scripts/README.md)。

### 编译产物

| 平台    | GUI                          | CLI                                                              |
| ------- | ---------------------------- | ---------------------------------------------------------------- |
| macOS   | `build/LocalAIAssistant.app` | `build/LocalAIAssistant-CLI` 或 `build/LocalAIAssistant-CLI.app` |
| Windows | `build/LocalAIAssistant.exe` | `build/LocalAIAssistant-CLI.exe`                                 |
| Linux   | `build/LocalAIAssistant`     | `build/LocalAIAssistant-CLI`                                     |

> **macOS CLI .app**：双击 `LocalAIAssistant-CLI.app`
> 会自动检测 iTerm2 并优先使用它打开，解决中文输入删除问题。

### 打包分发

```bash
./build.sh build -p     # 构建 + 打包
./build.sh package      # 单独打包已有构建
```

| 平台 | 格式 | 产出路径 |
|------|------|----------|
| macOS | DMG | `release/LocalAIAssistant-x.x.x-macOS.dmg` |
| Windows | ZIP | `release/LocalAIAssistant-x.x.x-Windows-x64.zip` |
| Linux | tar.gz | `release/LocalAIAssistant-x.x.x-Linux-x86_64.tar.gz` |

> Release 包**不包含**开发者的 `.env`。未进行代码签名，macOS 首次需右键打开，Windows SmartScreen 点击「仍要运行」。
> 详见 [scripts/README.md](scripts/README.md)。

### 自动发布 Release（GitHub Actions CI）

推送 `v` 开头 tag 触发三平台编译→测试→打包→发布：

```bash
git tag v1.0.0 && git push origin v1.0.0
```

三平台全部通过后自动发布到 [GitHub Releases](https://github.com/nathanpenny520/LocalAIAssistant/releases)。

---

## 使用方法

### 运行

```bash
# GUI
open build/LocalAIAssistant.app        # macOS
build\LocalAIAssistant.exe             # Windows
./build/LocalAIAssistant               # Linux

# CLI
./build/LocalAIAssistant-CLI chat      # 交互式聊天
./build/LocalAIAssistant-CLI ask "问题" # 单次查询
```

> 完整使用指南、CLI 命令、安全设置、女友配置等详见 [docs/user/USAGE_zh_CN.md](docs/user/USAGE_zh_CN.md)。

## 配置 AI 服务

本程序支持两种配置方式，启动时按以下顺序加载：

```
应用启动
  → 1. 加载 .env 文件（基线默认值）
  → 2. 加载 QSettings（GUI 设置，非空字段覆盖 .env）
  → 生效
```

**GUI 设置对话框中的值会覆盖 `.env` 中的同名配置。** 如果只使用 CLI 模式，配置 `.env` 即可。

### 方式一：GUI 设置对话框（推荐）

打开程序 → 设置 → 填入 API URL、API Key、模型名 → 保存。设置自动持久化到 QSettings。

### 方式二：.env 文件（CLI / 便携部署 / 基线默认值）

在可执行文件同目录或用户数据目录创建 `.env` 文件。支持的 API 类型：

| `AI_API_TYPE` | 对应服务 | 说明 |
|---|---|---|
| `openai` | OpenAI 或兼容 API | 默认类型 |
| `ollama` | [Ollama](https://ollama.com/download) 本地部署 | URL 含 `11434` 端口时自动检测切换 |
| `anthropic` | [Anthropic Claude](https://www.anthropic.com) | 需 API Key |
| `llamacpp` | [LlamaCpp](https://github.com/ggerganov/llama.cpp) | 兼容 OpenAI 格式，本地部署 |

> **Ollama 自动检测**：即使 `AI_API_TYPE` 设为 `openai`，如果 API URL 包含 `11434`（Ollama 默认端口），程序会自动切换为 Ollama 模式。

```bash
cp .env.example .env
```

`.env` 文件中的 AI 配置项：

```
AI_API_TYPE=openai                     # openai | ollama | llamacpp | anthropic
AI_API_URL=http://127.0.0.1:8080       # API 地址
AI_API_KEY=sk-your-api-key-here        # API 密钥（Ollama/LlamaCpp 可留空）
AI_MODEL_NAME=local-model              # 模型名称
```

---

## AI 女友模块

通过 `Ctrl/Cmd+G` 打开 AI 女友窗口。配置讯飞语音凭证后可使用语音交互（ASR 语音识别 + TTS 语音合成）。

人设可通过 `resources/prompts/<语言>/girlfriend.md` 自定义，记忆系统自动跨会话保持。

> 详见 [docs/user/USAGE_zh_CN.md](docs/user/USAGE_zh_CN.md) 的 AI 女友章节。

---

## 数据存储位置

本应用将数据分为两类存储：

| 数据类型 | 存储机制 | 内容 |
|---|---|---|
| 应用配置 | **QSettings** | API URL/Key、模型名、主题、语言等 |
| 运行数据 | **AppDataLocation** | 会话记录、知识库、女友设置、记忆等 |

### QSettings（应用配置）

GUI 设置对话框中修改的选项保存在此：

| 平台    | 存储位置 |
| ------- | -------- |
| macOS   | `~/Library/Preferences/com.localaiassistant.LocalAIAssistant.plist` |
| Windows | 注册表 `HKEY_CURRENT_USER\Software\LocalAIAssistant\Settings` |
| Linux   | `~/.config/LocalAIAssistant/Settings.conf` |

常用 key：`apiBaseUrl`、`apiKey`、`modelName`、`apiType`、`temperature`、`topP`、`maxTokens`、`maxContext`、`theme`、`language`、`streamingEnabled`。

### AppDataLocation（运行数据）

| 平台    | 目录路径 |
| ------- | -------- |
| macOS   | `~/Library/Application Support/LocalAIAssistant/` |
| Windows | `C:\Users\<USER>\AppData\Local\LocalAIAssistant\` |
| Linux   | `~/.local/share/LocalAIAssistant/` |

目录结构：

```
<AppDataLocation>/
├── .env                          # 用户 .env 配置（搜索路径 #4）
├── sessions/                     # 主会话 JSON（SessionManager）
│   ├── sessions.json             #   会话元数据列表
│   └── session_<id>.json         #   单个会话（对话历史）
├── girlfriend/                   # AI 女友模块
│   ├── settings.json             #   全局设置（头像、心情、语音、XFYUN 凭证）
│   ├── sessions/                 #   女友会话
│   ├── session_<id>.json         #   女友单次会话
│   └── memory.md                 #   用户记忆档案
├── knowledge/                    # 知识库
│   ├── chunks.db                 #   SQLite 文本块与元数据
│   └── vectors.bin               #   向量索引
├── tasks/                        # 任务撤销栈
├── prompts/                      # 用户自定义 prompt
└── memories.json                 # MemoryEnhancer 跨会话记忆
```

### 为什么找不到？

三个平台的数据目录**默认都是隐藏的**：

- **macOS**: `~/Library/` 自 10.7 起被系统隐藏
- **Windows**: `AppData` 是隐藏文件夹
- **Linux**: `.local` 以 `.` 开头，是隐藏目录

**如何访问：**

| 平台    | 方法 |
| ------- | ---- |
| macOS   | Finder → 前往 → 前往文件夹... → 粘贴 `~/Library/Application Support/LocalAIAssistant/` |
| Windows | 文件资源管理器地址栏输入 `%LOCALAPPDATA%\LocalAIAssistant` |
| Linux   | 终端执行 `xdg-open ~/.local/share/LocalAIAssistant/` |

### .env 搜索顺序

启动时按以下顺序查找 `.env`，挑**第一个存在的**加载：

| # | 路径 | 适用场景 |
|---|------|----------|
| 1 | `<exe 同目录>/.env` | 解压即用（Windows/Linux portable） |
| 2 | `<exe>/../Resources/.env` | macOS App Bundle |
| 3 | `<CWD>/.env` | 开发调试 |
| 4 | `<AppDataLocation>/.env` | 安装后用户级配置 |

---

## 常见问题

- **语音不工作** — 检查讯飞凭证和 Qt Multimedia/WebSockets 模块
- **多会话数据丢失** — 检查 `sessions.json` 和 `session_<id>.json` 文件
- **记忆未被记录** — 确认 AI 回复包含 `[更新记忆:xxx]` 标记，部分模型需更换

> 详见 [docs/user/USAGE_zh_CN.md](docs/user/USAGE_zh_CN.md) 的故障排除章节。

---

## 许可证

[MIT License](LICENSE)

---
