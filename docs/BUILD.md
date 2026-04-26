# 构建与安装指南

本文档详细说明如何在 macOS、Windows 和 Linux 上构建、安装和配置本地AI助手。

## 新手完整流程图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        从零开始 → 完整使用流程                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  第一步: 获取项目                                                            │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ git clone https://github.com/xxx/sourcecode-ai-assistant.git        │   │
│  │ 或直接下载 ZIP 解压                                                  │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                              ↓                                              │
│  第二步: 安装开发环境                                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ • macOS: Xcode CLT + CMake + Qt                                     │   │
│  │ • Windows: Visual Studio + Qt                                       │   │
│  │ • Linux: GCC + CMake + Qt6                                          │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                              ↓                                              │
│  第三步: 构建项目                                                            │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ ./scripts/build.sh   (macOS/Linux)                                  │   │
│  │ scripts\build.bat    (Windows CMD)                                  │   │
│  │ .\scripts\build.ps1  (Windows PowerShell)                           │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                              ↓                                              │
│  第四步: 安装 AI 服务（必需！）                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ • Ollama（推荐）: https://ollama.com/download                       │   │
│  │ • LM Studio: https://lmstudio.ai/                                   │   │
│  │ • 或使用 OpenAI API                                                 │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                              ↓                                              │
│  第五步: 配置并运行                                                          │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ GUI: 设置 → 填写 API URL → 开始对话                                  │   │
│  │ CLI: config --local --api-url "http://127.0.0.1:11434"              │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

> **重要**: 本程序只是一个客户端，需要连接 AI 服务才能工作。请确保在运行前已安装 AI 服务（如 Ollama）。

## 依赖要求

| 软件 | 版本要求 | macOS | Windows | Linux |
|------|----------|-------|---------|-------|
| C++编译器 | C++17支持 | Xcode CLT | VS2019+ / MSVC | GCC 9+ |
| Qt | 6.x | 官网或Homebrew | 官网安装 | 包管理器 |
| CMake | 3.16+ | Homebrew | 官网或VS | 包管理器 |
| Poppler | 26.x+ | Homebrew | MSYS2 | 包管理器 |

> **Poppler 说明**: PDF 文本提取库，用于解析 PDF 文件内容。如不需要 PDF 支持，可跳过安装。

### 第一步：检查现有环境

在安装依赖之前，先检查你的系统是否已经安装了必要的工具：

```bash
# macOS/Linux 终端
cmake --version          # 检查 CMake
clang++ --version        # macOS 检查编译器
g++ --version            # Linux 检查编译器
qmake6 --version         # 检查 Qt（如已安装）

# Windows PowerShell
cmake --version          # 检查 CMake
# 编译器需要在 VS Developer Command Prompt 中检查
```

**检查结果说明**:
- 如果命令成功执行并显示版本号 → 已安装，跳过对应步骤
- 如果显示 "command not found" 或 "未找到" → 需要安装

### 版本检查命令

```bash
# 检查编译器
clang++ --version   # macOS
g++ --version       # Linux
# Windows: 在 VS Developer Command Prompt 中运行 cl

# 检查 CMake
cmake --version

# 检查 Qt（如果已安装）
qmake6 --version    # 或 qmake -qt=6
```

## 依赖安装

### macOS

#### 方式一：Homebrew（推荐新手）

```bash
# 安装 Xcode Command Line Tools
xcode-select --install

# 安装 Homebrew（如未安装）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install cmake qt@6 poppler pkg-config
```

#### 方式二：Qt 官网安装（推荐开发者）

> **新手提示**: Qt 官网安装需要注册账号，过程稍复杂但能获得完整开发环境。

**详细步骤**:

1. 访问 [Qt 官网下载页](https://www.qt.io/download)
2. 点击 "Download" → 选择 "Qt Online Installer"
3. 运行安装程序，需要注册 Qt 账号（免费）
4. 在安装组件选择界面：
   - 展开 "Qt 6.x" → 选择最新版本（如 6.10.2）
   - 选择 "macOS" 组件
   - 点击 "Next" 完成安装
5. 安装完成后，Qt 通常位于 `~/Qt/<版本号>/macos`

**安装 CMake**：`brew install cmake`

**Qt 安装路径示例：**
- 官网安装：`~/Qt/<版本号>/macos`（如 `~/Qt/6.10.2/macos`）
- Homebrew：`/opt/homebrew/opt/qt@6` 或 `/usr/local/opt/qt@6`

### Windows

#### Visual Studio + MSVC（推荐）

**步骤 1: 安装 Visual Studio**

1. 访问 [Visual Studio 官网](https://visualstudio.microsoft.com/)
2. 下载 "Visual Studio Community"（免费版）
3. 运行安装程序
4. **关键**: 在工作负载选择界面，勾选 **"使用 C++ 的桌面开发"**
5. 点击 "安装"，等待完成（约 20-30 分钟）

> **新手提示**: "Developer Command Prompt for VS" 是专门配置了编译环境的命令行工具。

**Developer Command Prompt 位置**:
- 开始菜单 → 搜索 "Developer Command Prompt"
- 或: 开始菜单 → Visual Studio 2022 → Developer Command Prompt

**步骤 2: 安装 Qt**

> **新手提示**: Qt 安装需要注册账号，请耐心完成。

1. 访问 [Qt 官网](https://www.qt.io/download)
2. 下载 "Qt Online Installer"
3. 运行安装程序，注册 Qt 账号（免费）
4. 在组件选择界面：
   - 展开 "Qt 6.x" → 选择最新版本（如 6.10.2）
   - 选择 **"MSVC 2019 64-bit"**（重要！不是 MinGW）
   - 点击 "Next" 完成安装

**Qt 安装路径示例：**
- `C:\Qt\<版本号>\msvc2019_64`（如 `C:\Qt\6.10.2\msvc2019_64`）

#### MinGW（备选）

```powershell
# 使用 winget 安装
winget install Kitware.CMake
winget install MSYS2.MSYS2

# 在 MSYS2 中安装 Qt
pacman -S mingw-w64-x86_64-qt6-base mingw-w64-x86_64-cmake
```

**Qt 安装路径示例：**
- `C:\Qt\<版本号>\mingw1310_64`（如 `C:\Qt\6.10.2\mingw1310_64`）

### Linux

#### Ubuntu/Debian

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-base-dev-tools libpoppler-cpp-dev pkg-config
```

#### Fedora/RHEL

```bash
sudo dnf install cmake gcc-c++ qt6-qtbase-devel poppler-cpp-devel pkgconfig
```

#### Arch Linux

```bash
sudo pacman -S cmake gcc qt6-base poppler pkgconf
```

**Qt 安装路径：**
- 包管理器安装后 Qt 通常位于 `/usr/lib/qt6` 或 `/usr`
- 可通过 `qmake6 -query QT_INSTALL_PREFIX` 获取实际路径

## 构建步骤

### 使用构建脚本（推荐）

构建脚本会自动检测 Qt 安装路径，无需手动配置。

#### macOS/Linux

```bash
cd /path/to/sourcecode-ai-assistant
chmod +x scripts/*.sh

# 默认构建（Release 版本）
./scripts/build.sh

# 其他选项
./scripts/build.sh -c           # 清理后重新构建
./scripts/build.sh -d           # Debug 版本
./scripts/build.sh -j 8         # 8 并行任务
./scripts/build.sh LocalAIAssistant-CLI  # 仅构建 CLI
```

**验证构建成功**:

构建完成后，检查以下文件是否存在：

```bash
# macOS
ls build/LocalAIAssistant.app          # 应显示目录
ls build/LocalAIAssistant-CLI          # 应显示文件

# Linux
ls build/LocalAIAssistant              # 应显示文件
ls build/LocalAIAssistant-CLI          # 应显示文件
```

如果文件存在，说明构建成功！继续下一步。

**验证 Windows 构建**:

```cmd
# Windows CMD
dir build\LocalAIAssistant.exe          # 应显示文件
dir build\LocalAIAssistant-CLI.exe      # 应显示文件
```

> **提示**: 如果构建失败，请检查 [常见问题与故障排除](#常见问题与故障排除) 部分。

#### Windows CMD

```cmd
cd C:\path\to\sourcecode-ai-assistant

# 默认构建（Release 版本）
scripts\build.bat

# 其他选项
scripts\build.bat /C            # 清理后重新构建
scripts\build.bat /D            # Debug 版本
scripts\build.bat /J:8          # 8 并行任务
scripts\build.bat LocalAIAssistant-CLI  # 仅构建 CLI
```

#### Windows PowerShell

```powershell
cd C:\path\to\sourcecode-ai-assistant

# 默认构建（Release 版本）
.\scripts\build.ps1

# 其他选项
.\scripts\build.ps1 -Clean      # 清理后重新构建
.\scripts\build.ps1 -Debug      # Debug 版本
.\scripts\build.ps1 -Jobs 8     # 8 并行任务
.\scripts\build.ps1 -Target LocalAIAssistant-CLI  # 仅构建 CLI
```

### 手动构建

如果构建脚本无法检测到 Qt，可手动指定路径。

#### macOS

```bash
mkdir build && cd build

# 替换 <Qt版本> 为你的实际版本号
cmake .. -DCMAKE_PREFIX_PATH=~/Qt/<Qt版本>/macos
# 示例: cmake .. -DCMAKE_PREFIX_PATH=~/Qt/6.10.2/macos

cmake --build .
```

#### Windows (MSVC)

打开 "Developer Command Prompt for VS"，然后：

```cmd
mkdir build && cd build

# 替换 <Qt版本> 为你的实际版本号
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/<Qt版本>/msvc2019_64"
# 示例: cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.10.2/msvc2019_64"

cmake --build . --config Release
```

#### Windows (MinGW)

```cmd
mkdir build && cd build

cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/<Qt版本>/mingw1310_64" -G "MinGW Makefiles"
cmake --build .
```

#### Linux

```bash
mkdir build && cd build

# 包管理器安装的 Qt 通常自动检测
cmake ..

# 或手动指定
cmake .. -DCMAKE_PREFIX_PATH=/usr/lib/qt6
cmake --build .
```

## 构建脚本参数说明

### build.sh (macOS/Linux)

| 参数 | 说明 |
|------|------|
| `-h, --help` | 显示帮助信息 |
| `-c, --clean` | 清理后重新构建 |
| `-d, --debug` | Debug 版本 |
| `-r, --release` | Release 版本（默认） |
| `-v, --verbose` | 详细编译输出 |
| `-j, --jobs <n>` | 并行任务数 |
| `-q, --qt-path <path>` | 指定 Qt 路径 |
| `--list-targets` | 列出所有目标 |

**目标**: `all`(默认) / `LocalAIAssistant`(仅GUI) / `LocalAIAssistant-CLI`(仅CLI)

### build.bat (Windows CMD)

| 参数 | 说明 |
|------|------|
| `/?` | 显示帮助信息 |
| `/C` | 清理后重新构建 |
| `/D` | Debug 版本 |
| `/R` | Release 版本（默认） |
| `/V` | 详细编译输出 |
| `/J:n` | 并行任务数 |
| `/Q:path` | 指定 Qt 路径 |
| `/T` | 列出所有目标 |

**目标**: `all`(默认) / `LocalAIAssistant`(仅GUI) / `LocalAIAssistant-CLI`(仅CLI)

### build.ps1 (Windows PowerShell)

| 参数 | 说明 |
|------|------|
| `-Help` | 显示帮助信息 |
| `-Clean` | 清理后重新构建 |
| `-Debug` | Debug 版本 |
| `-Release` | Release 版本（默认） |
| `-Verbose` | 详细编译输出 |
| `-Jobs <n>` | 并行任务数 |
| `-QtPath <path>` | 指定 Qt 路径 |
| `-ListTargets` | 列出所有目标 |
| `-Target <name>` | 构建目标 |

**目标**: `all`(默认) / `LocalAIAssistant`(仅GUI) / `LocalAIAssistant-CLI`(仅CLI)

## 演示脚本

构建完成后，可运行演示脚本快速体验功能：

### demo.sh (macOS/Linux)

```bash
./scripts/demo.sh              # 运行完整演示
```

演示内容：显示帮助、查看配置、会话管理、单次查询。

### demo.bat (Windows CMD)

```cmd
scripts\demo.bat               # 运行完整演示
```

### demo.ps1 (Windows PowerShell)

```powershell
.\scripts\demo.ps1             # 运行完整演示
.\scripts\demo.ps1 -Help       # 显示帮助
```

## 安装到系统

### macOS/Linux

```bash
# 仅安装 CLI（默认安装到 /usr/local/bin/ai）
sudo ./scripts/install.sh

# 安装 GUI 到 /Applications（仅 macOS）
sudo ./scripts/install.sh --gui

# 全部安装
sudo ./scripts/install.sh --all

# 安装到用户目录（无需 sudo）
./scripts/install.sh --prefix ~/.local

# 卸载
sudo ./scripts/install.sh --uninstall --all
```

### Windows

#### 使用安装脚本（推荐）

**Windows PowerShell:**

```powershell
# 仅安装 CLI（默认安装到用户目录，无需管理员权限）
.\scripts\install.ps1

# 安装 GUI 版本
.\scripts\install.ps1 -Gui

# 全部安装
.\scripts\install.ps1 -All

# 安装到系统目录（需要管理员权限）
.\scripts\install.ps1 -Prefix "C:\Program Files\LocalAIAssistant"

# 卸载
.\scripts\install.ps1 -Uninstall
.\scripts\install.ps1 -Uninstall -All    # 卸载所有版本
```

**Windows CMD:**

```cmd
# 仅安装 CLI
scripts\install.bat

# 安装 GUI 版本
scripts\install.bat /GUI

# 全部安装
scripts\install.bat /ALL

# 安装到系统目录（需要管理员权限）
scripts\install.bat /PREFIX:"C:\Program Files\LocalAIAssistant"

# 卸载
scripts\install.bat /UNINSTALL
scripts\install.bat /UNINSTALL /ALL
```

#### 手动安装

如果需要手动安装：

```powershell
# CLI - 复制到 PATH 目录
copy build\LocalAIAssistant-CLI.exe C:\Windows\

# GUI - 复制到合适位置
xcopy /E /I build\LocalAIAssistant.exe "C:\Program Files\LocalAIAssistant\"
```

## 安装脚本参数说明

### install.sh (macOS/Linux)

| 参数 | 说明 |
|------|------|
| `-h, --help` | 显示帮助信息 |
| `--cli` | 安装 CLI 版本（默认） |
| `--gui` | 安装 GUI 版本到 /Applications |
| `--all` | 安装 CLI 和 GUI 版本 |
| `--prefix <path>` | 安装路径前缀（默认: /usr/local） |
| `--uninstall` | 卸载已安装的程序 |
| `-f, --force` | 强制安装/卸载，不询问确认 |

**示例**:
```bash
./scripts/install.sh                        # 安装 CLI 到 /usr/local/bin/ai
./scripts/install.sh --prefix ~/.local      # 安装到用户目录
sudo ./scripts/install.sh --all             # 安装全部版本
./scripts/install.sh --uninstall --all      # 卸载所有版本
```

### install.bat (Windows CMD)

| 参数 | 说明 |
|------|------|
| `/?` | 显示帮助信息 |
| `/CLI` | 安装 CLI 版本（默认） |
| `/GUI` | 安装 GUI 版本 |
| `/ALL` | 安装 CLI 和 GUI 版本 |
| `/PREFIX:path` | 安装路径前缀 |
| `/UNINSTALL` | 卸载已安装的程序 |
| `/F` | 强制安装/卸载，不询问确认 |

**示例**:
```cmd
scripts\install.bat                         # 安装 CLI 到默认位置
scripts\install.bat /PREFIX:"%LOCALAPPDATA%\LocalAIAssistant"  # 安装到用户目录
scripts\install.bat /UNINSTALL /ALL         # 卸载所有版本
```

### install.ps1 (Windows PowerShell)

| 参数 | 说明 |
|------|------|
| `-Help` | 显示帮助信息 |
| `-Cli` | 安装 CLI 版本（默认） |
| `-Gui` | 安装 GUI 版本 |
| `-All` | 安装 CLI 和 GUI 版本 |
| `-Prefix <path>` | 安装路径前缀（默认: `$env:LOCALAPPDATA\LocalAIAssistant`） |
| `-Uninstall` | 卸载已安装的程序 |
| `-Force` | 强制安装/卸载，不询问确认 |

**示例**:
```powershell
.\scripts\install.ps1                       # 安装 CLI 到用户目录
.\scripts\install.ps1 -Prefix "C:\Program Files\LocalAIAssistant"  # 安装到系统目录
.\scripts\install.ps1 -Uninstall -All       # 卸载所有版本
```

## 常见问题与故障排除

### 找不到 Qt

**症状**：CMake 报错 `Could not find Qt6`

**解决方案**：

1. 确认 Qt 已安装
2. 手动指定 Qt 路径：

```bash
# macOS/Linux
cmake .. -DCMAKE_PREFIX_PATH=/your/qt/path

# Windows
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2019_64"
```

3. 或使用环境变量：

```bash
export Qt6_DIR=/path/to/qt/lib/cmake/Qt6
# Windows CMD
set Qt6_DIR=C:\Qt\6.x.x\msvc2019_64\lib\cmake\Qt6
```

### 编译错误

**症状**：编译时报语法错误或链接错误

**排查步骤**：

1. 确认 C++17 支持：
   - macOS: Xcode CLT 已安装 (`xcode-select --install`)
   - Windows: VS2019+ 已安装 C++ 工具
   - Linux: GCC 9+ (`g++ --version`)

2. 清理后重新构建：
   ```bash
   ./scripts/build.sh -c
   # Windows: scripts\build.bat /C
   ```

3. 检查 Qt 模块完整性（需要 Widgets、Network、LinguistTools）

### 运行时找不到 Qt 库

**症状**：运行时报 `Library not loaded` 或类似错误

**macOS 解决方案**：

```bash
# 临时方案
export DYLD_LIBRARY_PATH=~/Qt/<Qt版本>/macos/lib:$DYLD_LIBRARY_PATH

# 永久方案（添加到 ~/.zshrc 或 ~/.bash_profile）
export DYLD_LIBRARY_PATH=~/Qt/<Qt版本>/macos/lib:$DYLD_LIBRARY_PATH
```

**Windows 解决方案**：

将 Qt 的 bin 目录添加到 PATH：
```cmd
set PATH=C:\Qt\6.x.x\msvc2019_64\bin;%PATH%
```

**Linux 解决方案**：

通常包管理器安装的 Qt 会自动配置库路径。如有问题：
```bash
export LD_LIBRARY_PATH=/usr/lib/qt6/lib:$LD_LIBRARY_PATH
```

### 无法连接 API

**症状**：运行时无法连接到 AI 服务

**排查步骤**：

1. 确认 AI 服务正在运行：
   ```bash
   curl http://127.0.0.1:11434/v1/models
   ```

2. 检查防火墙设置

3. 验证 API URL 和密钥是否正确

### Poppler/PDF 相关问题

**症状**：CMake 报错 `Could not find poppler-cpp`

**解决方案**：

1. 确认 Poppler 已安装：
   ```bash
   # macOS
   brew install poppler pkg-config

   # Ubuntu/Debian
   sudo apt install libpoppler-cpp-dev pkg-config
   ```

2. 确认 pkg-config 可检测到 Poppler：
   ```bash
   pkg-config --cflags poppler-cpp
   ```

**症状**：PDF 文件无法识别内容

**原因**：
- 扫描版 PDF（纯图片）无文本层，Poppler 无法提取
- 可将 PDF 转为图片后上传，使用 Vision API 识别

### IDE 中 Qt 报错

**症状**：VS Code / CLion 中 Qt 头文件报错

**解决方案**：

项目已生成 `build/compile_commands.json`。确保：

1. `.clangd` 文件存在于项目根目录
2. IDE 的 C++ 扩展已启用 compile_commands.json 支持
3. Qt 路径正确配置

VS Code 用户可安装 clangd 扩展获得更好的支持。

---

## AI 服务安装（必需）

> **重要**: 本程序只是一个客户端，需要连接 AI 服务才能正常工作。

### 方式一：Ollama（推荐新手）

Ollama 是最简单的本地 AI 服务，一键安装即可使用。

**安装步骤**:

1. 访问 [Ollama 官网](https://ollama.com/download)
2. 下载对应平台的安装包：
   - macOS: 下载 `.dmg` 文件
   - Windows: 下载 `.exe` 安装程序
   - Linux: 按官网命令安装
3. 安装完成后，Ollama 会自动启动

**下载模型**:

```bash
# 在终端运行，下载一个 AI 模型
ollama pull llama3        # 推荐：Meta Llama 3
ollama pull mistral       # 备选：Mistral
ollama pull qwen2         # 备选：阿里 Qwen2
```

**验证服务运行**:

```bash
curl http://127.0.0.1:11434/v1/models
# 应返回 JSON 格式的模型列表
```

### 方式二：LM Studio

LM Studio 提供图形界面，适合不想用命令行的用户。

1. 访问 [LM Studio 官网](https://lmstudio.ai/)
2. 下载安装程序
3. 在 LM Studio 中下载模型
4. 启动本地服务器（默认端口 1234）

### 方式三：使用 OpenAI API

如果你有 OpenAI API Key：

```bash
# CLI 配置
./build/LocalAIAssistant-CLI config --external \
  --api-url "https://api.openai.com/v1" \
  --api-key "sk-your-key" \
  --model "gpt-4"
```

---

## 快速测试

构建和安装 AI 服务后，进行简单测试：

### 测试 CLI 版本

```bash
# macOS/Linux
./build/LocalAIAssistant-CLI config --show-config    # 查看配置
./build/LocalAIAssistant-CLI ask "你好，介绍一下自己"

# Windows
.\build\LocalAIAssistant-CLI.exe config --show-config
.\build\LocalAIAssistant-CLI.exe ask "你好"
```

### 测试 GUI 版本

```bash
# macOS
open build/LocalAIAssistant.app

# Windows
.\build\LocalAIAssistant.exe

# Linux
./build/LocalAIAssistant
```

**GUI 首次运行配置**:

1. 打开设置（菜单栏 → 设置）
2. 勾选 "使用本地模式"
3. API URL 设置为:
   - Ollama: `http://127.0.0.1:11434`
   - LM Studio: `http://127.0.0.1:1234/v1`
4. 模型名称设置为已下载的模型名（如 `llama3`）
5. 点击保存，开始对话

---

## 相关文档

- [README.md](README.md) - 完整使用指南
- [../README.md](../README.md) - 项目根目录快速开始
- [ARCHITECTURE.md](ARCHITECTURE.md) - 架构设计详情
- [CONTRIBUTING.md](CONTRIBUTING.md) - 贡献指南

---

## 新手检查清单

在开始使用之前，请确认以下步骤已完成：

| 步骤 | 检查项 | 命令/方法 |
|------|--------|-----------|
| 1 | ✓ 已安装编译器 | `cmake --version` |
| 2 | ✓ 已安装 Qt 6.x | 检查 Qt 目录存在 |
| 3 | ✓ 构建成功 | `ls build/LocalAIAssistant*` |
| 4 | ✓ 已安装 AI 服务 | Ollama/LM Studio/OpenAI |
| 5 | ✓ 已下载模型 | `ollama list` |
| 6 | ✓ 已配置 API | GUI设置 或 CLI config |

**全部完成？恭喜！现在可以开始使用了。**

```
# GUI 用户
open build/LocalAIAssistant.app       # macOS
.\build\LocalAIAssistant.exe          # Windows

# CLI 用户  
./build/LocalAIAssistant-CLI chat     # macOS/Linux
.\build\LocalAIAssistant-CLI.exe chat # Windows
```

遇到问题？查看 [常见问题与故障排除](#常见问题与故障排除) 或提交 Issue。

---

**构建愉快！**