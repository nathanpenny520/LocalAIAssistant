# LocalAIAssistant WSL 完整使用教程

## 一、项目概述

LocalAIAssistant 是一款基于 Qt6 的跨平台桌面 AI 应用，支持 GUI 图形界面和 CLI 命令行两种模式，内置 AI 女友语音交互模块。

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

## 二、环境准备

### 2.1 确认 WSL 已安装

```powershell
# 在 Windows PowerShell 中运行
wsl -l -v
```

输出示例：
```
  NAME              STATE           VERSION
* Ubuntu            Stopped         2
  docker-desktop    Stopped         2
```

如果没有安装 WSL，请先安装：
```powershell
wsl --install -d Ubuntu
```

### 2.2 启动 WSL

```powershell
wsl -d Ubuntu
```

---

## 三、将项目复制到 Linux 文件系统（推荐）

虽然可以直接从 `/mnt/d/` 运行，但**强烈建议**将项目复制到 WSL 的 Linux 文件系统中：

- **性能更好**：Linux 原生文件系统比跨系统访问 `/mnt/d/` 快得多
- **路径更简洁**：不用处理空格和转义字符
- **兼容性更好**：避免 Windows/Linux 文件权限差异问题

### 复制命令

```bash
# 从 Windows D 盘复制到 Linux 家目录
cp -r /mnt/d/LocalAIAssistant-Linux/LocalAIAssistant-1.1.0-Linux-x86_64/LocalAIAssistant-1.1.0 ~/LocalAIAssistant

# 进入 Linux 目录
cd ~/LocalAIAssistant
```

> 后续教程默认使用 `~/LocalAIAssistant` 路径。

---

## 四、命令行版本（CLI）运行教程

### 4.1 进入项目目录

```bash
cd ~/LocalAIAssistant
```

### 4.2 安装方式对比

你可以选择**直接运行**（不安装）或**运行 install.sh 安装**，两者的主要区别如下：

| 对比项 | 安装前（直接运行） | 安装后（运行 install.sh） |
|--------|-------------------|--------------------------|
| **运行命令** | `./LocalAIAssistant-CLI` | `LocalAIAssistant-CLI`（无需 `./`） |
| **运行位置** | 必须在项目目录 `~/LocalAIAssistant` 中 | 任意目录均可运行 |
| **文件位置** | 可执行文件和资源都在当前目录 | 二进制在 `~/.local/bin/`，资源在 `~/.local/share/localaiassistant/` |
| **PATH 环境变量** | 需要指定完整路径或进入目录 | 已自动添加到用户 PATH |
| **依赖安装** | 需手动安装 Qt6 等依赖 | 自动检测并安装缺失的依赖 |
| **配置文件** | 项目目录中的 `.env` | `~/.local/share/localaiassistant/.env` |
| **删除原目录** | ❌ 不可删除（程序依赖它） | ✅ 可以删除原始项目目录 |
| **适用场景** | 临时测试、便携使用 | 长期使用、系统集成 |

> **推荐**：如果你是长期使用，建议运行 `./install.sh` 进行安装，这样可以在终端任何位置直接输入命令启动。

### 4.3 运行 install.sh 安装

```bash
./install.sh
```

安装脚本会：
- 检测并安装 Qt6 运行时依赖
- 将二进制文件复制到 `~/.local/bin/`
- 将资源文件复制到 `~/.local/share/localaiassistant/`
- 自动创建 `.env` 配置文件

### 4.4 直接运行 CLI（不安装）

```bash
./LocalAIAssistant-CLI --help
```

输出帮助信息：
```
Usage: ./LocalAIAssistant-CLI [options] command

LocalAIAssistant - CLI Version
Supports interactive chat and single query modes

Options:
  -h, --help                   Displays help on commandline options.
  -v, --version                Displays version information.
  -s, --session <session-id>   Specify session ID for chat
  -n, --new                    Create new session
  -l, --list                   List all sessions
  -d, --delete <session-id>    Delete specified session
  --show <session-id>          Show session content
  --api-url <url>              Set API base URL
  --api-key <key>              Set API key
  --model <name>               Set model name
  --show-config                Show current config
  --no-stream                  Disable streaming output
  --temperature <value>        Set temperature (0.0-2.0)
  --top-p <value>              Set top_p (0.0-1.0)
  --max-tokens <value>         Set max output tokens (1-128000)
  --api-type <type>            Set API type (openai, ollama, llamacpp, anthropic)
  -y, --yes                    Auto-confirm task plans

Arguments:
  command                      Command: chat | ask <question> | sessions | config
```

### 4.5 启动交互式聊天

```bash
./LocalAIAssistant-CLI chat
```

> 如果已安装，可直接使用：`LocalAIAssistant-CLI chat`

### 4.6 单次提问模式

```bash
./LocalAIAssistant-CLI ask "你好，请介绍一下自己"
```

> 如果已安装，可直接使用：`LocalAIAssistant-CLI ask "你好，请介绍一下自己"`

### 4.7 常用 CLI 交互命令

进入交互模式后，可使用以下命令：

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

### 4.8 配置 AI 服务

编辑 `.env` 文件：

```bash
# 使用本地 Ollama
cp .env.example .env
nano .env
```

示例配置（Ollama）：
```env
AI_API_TYPE=ollama
AI_API_URL=http://127.0.0.1:11434
AI_MODEL_NAME=llama3
```

示例配置（OpenAI）：
```env
AI_API_TYPE=openai
AI_API_URL=https://api.openai.com
AI_API_KEY=your-api-key-here
AI_MODEL_NAME=gpt-4o
```

---

## 五、桌面版本（GUI）运行教程

### 5.1 安装依赖

GUI 版本需要 Qt6 运行库，在 WSL Ubuntu 中安装：

```bash
sudo apt-get update
sudo apt-get install -y qt6-base-dev qt6-multimedia-dev qt6-websockets-dev \
    libpoppler-cpp-dev libzip-dev libpugixml-dev
```

### 5.2 配置图形显示

WSL 运行 GUI 应用需要图形显示支持，有以下两种方式：

#### 方式一：WSLg（推荐，Win11 自带）

Windows 11 已内置 WSLg，通常无需额外配置：

```bash
export DISPLAY=:0
```

#### 方式二：安装 X 服务器（Win10 或旧版 Win11）

1. 在 Windows 上安装 VcXsrv 或 X410
2. 启动 XLaunch，选择 "Multiple windows"，Display number 设为 0
3. 在 WSL 中设置：

```bash
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0
```

### 5.3 启动 GUI 版本

```bash
cd ~/LocalAIAssistant
export DISPLAY=:0
./LocalAIAssistant
```

如果看到类似以下输出，说明程序正在启动：
```
qt.multimedia.ffmpeg: Using Qt multimedia with FFmpeg version 8.0.1
```

### 5.4 验证进程

```bash
ps aux | grep LocalAIAssistant | grep -v grep
```

输出示例：
```
nathanp+  783  2.2  0.8 374504 68740 pts/0  Sl+  17:08  0:00 ./LocalAIAssistant
```

### 5.5 停止 GUI 程序

```bash
pkill -f LocalAIAssistant
```

---

## 六、AI 服务配置详解

### 6.1 方式一：云端 API（推荐）

| 服务商               | API 地址                      | 说明          |
| -------------------- | ----------------------------- | ------------- |
| OpenAI               | `https://api.openai.com`      | 需要 API Key  |
| 并行科技             | `https://llmapi.paratera.com` | 国内 API 代理 |
| 其他 OpenAI 兼容服务 | 按服务商文档                  | —             |

### 6.2 方式二：本地部署（Ollama）

1. 安装 Ollama：
```bash
curl -fsSL https://ollama.com/install.sh | sh
```

2. 下载模型：
```bash
ollama pull llama3
```

3. 在 `.env` 中配置：
```env
AI_API_TYPE=ollama
AI_API_URL=http://127.0.0.1:11434
AI_MODEL_NAME=llama3
```

### 6.3 方式三：本地部署（llama.cpp）

1. 下载并编译 llama.cpp
2. 下载 GGUF 模型文件
3. 启动 server：
```bash
llama-server -m model.gguf --port 8080
```

4. 在 `.env` 中配置：
```env
AI_API_TYPE=llamacpp
AI_API_URL=http://127.0.0.1:8080
AI_MODEL_NAME=local-model
```

---

## 七、AI 女友模块

### 7.1 打开 AI 女友窗口

在 GUI 中按 `Ctrl + G` 或从菜单打开。

### 7.2 头像等级

| 等级             | 类型     | 说明               |
| ---------------- | -------- | ------------------ |
| Level 1 (Belle)  | 静态 PNG | 经典风格           |
| Level 2 (Hot)    | 静态 PNG | 更加火辣           |
| Level 3 (Hotter) | MP4 视频 | 动态视频，跃然屏上 |

### 7.3 语音交互配置

编辑 `.env` 填入讯飞语音服务凭证：

```env
XFYUN_APP_ID=your_app_id_here
XFYUN_API_KEY=your_api_key_here
XFYUN_API_SECRET=your_api_secret_here
XFYUN_ASR_URL=wss://iat-api.xfyun.cn/v2/iat
XFYUN_TTS_URL=wss://cbm01.cn-huabei-1.xf-yun.com/v1/private/mcd9m97e6
XFYUN_VOICE_TYPE=x6_lingxiaoxuan_pro
```

获取凭证步骤：
1. 注册讯飞开放平台：https://www.xfyun.cn
2. 创建应用并开通「语音听写」和「超拟人语音合成」服务
3. 获取 APP ID、API Key、API Secret

---

## 八、常见问题

### 8.1 GUI 启动报错 `libQt6WebSockets.so.6: cannot open`

**原因**：缺少 Qt6 WebSockets 库  
**解决**：
```bash
sudo apt-get install -y qt6-websockets-dev libqt6websockets6
```

### 8.2 GUI 启动后无窗口显示

**原因**：DISPLAY 环境变量未设置  
**解决**：
```bash
export DISPLAY=:0
```

### 8.3 AI 无响应 / Connection refused

**原因**：AI 服务未启动或配置错误  
**解决**：
1. 检查 `.env` 中的 API 地址和密钥
2. 确认 Ollama 或其他 AI 服务已启动
3. 测试连接：`curl http://127.0.0.1:11434`

### 8.4 语音功能不可用

**原因**：未配置讯飞凭证或服务未开通  
**解决**：
1. 在 `.env` 中配置讯飞凭证
2. 确认讯飞账号中已开通相应服务
3. 检查网络是否能访问讯飞服务器

### 8.5 CLI 输出缓慢

**原因**：非流式模式下 AI 响应一次性到达  
**解决**：启用流式输出（默认已开启），或使用 `--no-stream` 以外的模式

### 8.6 安装后能在 Windows 中直接运行吗？

**答案**：不能。

`install.sh` 只是把文件复制到 WSL 内部的 Linux 目录（`~/.local/bin/` 和 `~/.local/share/localaiassistant/`），这些路径只在 WSL 环境中有效。

**原因**：
- 可执行文件是 ELF 格式的 Linux 程序，Windows 无法直接运行
- 依赖 Qt6 等 Linux 动态链接库（`.so` 文件）

**解决方案**：
1. **继续使用 WSL**（推荐）：在 Windows 终端或 PowerShell 中通过 WSL 运行：
   ```powershell
   wsl LocalAIAssistant-CLI chat
   wsl LocalAIAssistant  # GUI版本（需配置WSLg）
   ```
2. **使用 Windows 原生版本**：检查是否有专门的 Windows 版本（`.exe` 文件）

### 8.7 GUI 界面显示方块/乱码

**现象**：中文显示为方块（□□□）或乱码

**原因**：WSL 缺少中文字体

**解决方案**：

1. **安装中文字体**（关键步骤）：
   ```bash
   sudo apt-get update
   sudo apt-get install -y fonts-wqy-zenhei fonts-wqy-microhei fonts-noto-cjk
   fc-cache -fv
   ```

2. **设置 UTF-8 编码**（无需改为中文 locale）：
   ```bash
   export LANG=en_US.UTF-8
   export LC_ALL=en_US.UTF-8
   export DISPLAY=:0
   ./LocalAIAssistant
   ```

> **注意**：只需安装字体 + 使用 UTF-8 编码即可，无需将系统 locale 改为中文，保持英文 locale 也能正常显示中文。

**可选：创建启动脚本**（避免每次手动设置）：
```bash
# 创建启动脚本
cat > ~/start-localai.sh << 'EOF'
#!/bin/bash
export DISPLAY=:0
export LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8
cd ~/LocalAIAssistant
./LocalAIAssistant
EOF

# 赋予执行权限
chmod +x ~/start-localai.sh

# 以后直接运行
~/start-localai.sh
```

---

## 九、目录结构

```
LocalAIAssistant-1.1.0/
├── LocalAIAssistant           # GUI 可执行文件
├── LocalAIAssistant-CLI       # CLI 可执行文件
├── install.sh                 # 安装脚本
├── localaiassistant.desktop   # Linux 桌面快捷方式
├── .env.example               # 环境变量模板
├── core/                      # 核心配置
│   └── soul.md
├── docs/                      # 文档
│   ├── USAGE.md
│   └── USAGE_zh_CN.md
├── girlfriend/                # AI 女友配置
│   ├── memory.md
│   └── personality.md
├── models/                    # ONNX 嵌入模型
├── prompts/                   # 提示词模板
├── translations/              # 界面翻译文件
└── AIGirlfriend/              # AI 女友素材
    ├── level-1-belle/         # 等级1 静态图片
    ├── level-2-hot/           # 等级2 静态图片
    └── level-3-hotter/        # 等级3 视频
```

---

## 十、快捷命令参考

### 安装前命令（需在项目目录中运行）

```bash
# ========== CLI 命令 ==========
# 查看帮助
./LocalAIAssistant-CLI --help

# 交互式聊天
./LocalAIAssistant-CLI chat

# 单次提问
./LocalAIAssistant-CLI ask "问题内容"

# 自动确认任务计划
./LocalAIAssistant-CLI ask --yes "创建文件"

# 查看配置
./LocalAIAssistant-CLI config --show-config

# 设置 API
./LocalAIAssistant-CLI config --api-url "http://127.0.0.1:11434"

# ========== GUI 命令 ==========
# 启动桌面版（WSLg）
export DISPLAY=:0
./LocalAIAssistant

# 启动桌面版（X410/VcXsrv）
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0
./LocalAIAssistant

# ========== 会话管理 ==========
# 列出会话
./LocalAIAssistant-CLI sessions -l

# 新建会话
./LocalAIAssistant-CLI sessions -n

# 删除会话
./LocalAIAssistant-CLI sessions -d <session-id>
```

### 安装后命令（可在任意目录运行）

```bash
# ========== CLI 命令 ==========
# 查看帮助
LocalAIAssistant-CLI --help

# 交互式聊天
LocalAIAssistant-CLI chat

# 单次提问
LocalAIAssistant-CLI ask "问题内容"

# 自动确认任务计划
LocalAIAssistant-CLI ask --yes "创建文件"

# 查看配置
LocalAIAssistant-CLI config --show-config

# 设置 API
LocalAIAssistant-CLI config --api-url "http://127.0.0.1:11434"

# ========== GUI 命令 ==========
# 启动桌面版（WSLg）
export DISPLAY=:0
LocalAIAssistant

# 启动桌面版（X410/VcXsrv）
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0
LocalAIAssistant

# ========== 会话管理 ==========
# 列出会话
LocalAIAssistant-CLI sessions -l

# 新建会话
LocalAIAssistant-CLI sessions -n

# 删除会话
LocalAIAssistant-CLI sessions -d <session-id>
```

---

## 十一、安全说明

SafetyChecker 在执行前对每个文件操作和 Shell 命令进行三级安全校验：

| 等级 | 名称 | 行为 | 示例 |
|------|------|------|------|
| **Tier 1** | 永久拦截 | 立即拒绝 | `sudo`、`rm -rf`、`eval` |
| **Tier 2** | 需用户确认 | 用户明确批准 | 系统路径、白名单外路径 |
| **Tier 3** | 自动批准 | 无需提示 | `~/`、`/tmp`、当前目录 |

使用 `--yes` 可跳过 Tier 2 确认，但 **Tier 1 永不被绕过**。

---

**教程完成！** 现在你可以在 WSL 中愉快地使用 LocalAIAssistant 的命令行和桌面版本了。
