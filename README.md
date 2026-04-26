# 本地AI助手 (Local AI Assistant)

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Qt](https://img.shields.io/badge/Qt-6.x-green.svg)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C++-17-orange.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Windows%20%7C%20Linux-lightgrey.svg)](https://github.com)

> 一个功能完整的本地AI助手桌面应用，支持图形界面(GUI)和命令行界面(CLI)，可与 Ollama、LM Studio、OpenAI 等服务交互。

---

## 功能亮点

- **双模式支持** — GUI 图形界面 + CLI 命令行
- **文件上传** — 支持文本、图片、PDF 文件附件
- **多模态识别** — Vision API 图片识别，PDF 文本提取
- **流式输出** — SSE 实时显示，AI 回复逐字呈现
- **会话管理** — 多会话切换、历史持久化
- **多语言** — 简体中文 / English 即时切换
- **主题切换** — 亮色 / 暗色 / 跟随系统
- **隐私保护** — 数据完全本地化（本地模式）

---

## 界面预览

<!-- 截图占位 - 请添加实际截图后替换 -->
```
┌───────────────────────────────────────────────┐
│  设置                                         │
├──────────┬────────────────────────────────────┤
│          │                                    │
│ [+新对话] │         聊天内容区                  │
│ 对话列表  │      支持 Markdown 渲染             │
│          │                                    │
│          ├────────────────────────────────────┤
│          │  [file.txt ×] [image.png ×]        │  ← 文件列表
│          ├────────────────────────────────────┤
│          │  输入框: [________] [📁] [发送]      │  ← 文件按钮
└──────────┴────────────────────────────────────┘
```

> **提示**: 实际截图请参见项目 Wiki 或 Release 页面。

---

## 快速开始

### 30 秒体验

```bash
# 1. 构建（自动检测 Qt）
./scripts/build.sh          # macOS/Linux
scripts\build.bat           # Windows CMD
.\scripts\build.ps1         # Windows PowerShell

# 2. 安装 AI 服务（必需）
# 下载 Ollama: https://ollama.com/download
ollama pull llama3          # 下载模型

# 3. 运行
./build/LocalAIAssistant-CLI chat    # CLI 版本
open build/LocalAIAssistant.app      # GUI 版本 (macOS)
```

> 详细步骤请参阅 [BUILD.md](BUILD.md)。

---

## 前置要求

| 软件 | 版本 | 说明 |
|------|------|------|
| C++编译器 | C++17 | macOS: Xcode CLT / Windows: VS2019+ / Linux: GCC 9+ |
| Qt | 6.x | 跨平台 GUI 框架 |
| CMake | 3.16+ | 构建工具 |
| Poppler | 26.x | PDF 解析库（macOS: `brew install poppler`） |
| AI服务 | - | Ollama（推荐）/ LM Studio / OpenAI API |

---

## 使用方法

### GUI 版本

```bash
# macOS
open build/LocalAIAssistant.app

# Windows
.\build\LocalAIAssistant.exe

# Linux
./build/LocalAIAssistant
```

首次运行请在 **设置** 中配置：
- 勾选"使用本地模式"
- API URL: `http://127.0.0.1:11434`（Ollama）
- 模型名: `llama3`

### CLI 版本

```bash
# 交互式对话
./build/LocalAIAssistant-CLI chat

# 单次查询
./build/LocalAIAssistant-CLI ask "什么是人工智能？"

# 会话管理
./build/LocalAIAssistant-CLI sessions -l    # 列出会话
./build/LocalAIAssistant-CLI sessions -n    # 新建会话

# 设置
./build/LocalAIAssistant-CLI config --local --api-url "http://127.0.0.1:11434"
```

#### 模型参数设置

```bash
# 设置单个参数
./build/LocalAIAssistant-CLI config --temperature 0.7

# 设置多个参数
./build/LocalAIAssistant-CLI config --temperature 0.7 --top-p 0.9 --seed 42

# 查看所有配置
./build/LocalAIAssistant-CLI config --show-config
```

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

---

## 文件上传

### GUI 版本

点击输入框右侧的 📁 按钮，选择文件添加为附件。文件列表显示在输入框上方，点击 × 可删除单个文件。

### CLI 版本

```bash
/file /path/to/document.pdf    # 添加文件
/listfiles                      # 查看待发送文件
/clearfiles                     # 清空文件列表
```

### 支持的文件类型

| 类型 | 扩展名 | 处理方式 |
|------|--------|---------|
| 文本文件 | txt, md, py, cpp, json, yaml, ... | 直接读取内容 |
| 图片文件 | png, jpg, jpeg, gif, bmp, webp | Base64 编码，Vision API 识别 |
| PDF 文件 | pdf | Poppler 提取文本层 |

> **注意**：
> - 图片识别需使用支持 Vision 的模型（如 `gpt-4o`、`gpt-4-vision-preview`）
> - 扫描版 PDF（无文本层）暂不支持，可转为图片后上传
> - 文件大小限制：10MB

---

## 文档

| 文档 | 说明 |
|------|------|
| [BUILD.md](BUILD.md) | 构建与安装详细指南 |
| [ARCHITECTURE.md](ARCHITECTURE.md) | 项目架构设计 |
| [CHANGELOG.md](CHANGELOG.md) | 版本更新日志 |
| [CONTRIBUTING.md](CONTRIBUTING.md) | 贡献指南 |
| [../README.md](../README.md) | 项目根目录快速入口 |

---

## 常见问题

<details>
<summary><b>Q: GUI 和 CLI 有什么区别？</b></summary>

- **GUI** — 日常使用，界面友好，支持 Markdown 渲染、主题切换
- **CLI** — 脚本集成、自动化、远程服务器使用
</details>

<details>
<summary><b>Q: 支持哪些 AI 服务？</b></summary>

- **Ollama**（推荐）— 本地部署，一键安装
- **LM Studio** — 图形界面管理模型
- **LocalAI** — OpenAI API 兼容
- **OpenAI API** — 云端服务，需 API Key
</details>

<details>
<summary><b>Q: 图片/PDF 为什么无法识别？</b></summary>

- **图片识别**：需使用支持 Vision 的模型（`gpt-4o`、`gpt-4-vision-preview`），`gpt-3.5-turbo` 不支持
- **PDF 文本**：正常 PDF（有文本层）可提取；扫描版 PDF 需转为图片后上传
</details>

<details>
<summary><b>Q: 数据存储在哪里？</b></summary>

| 平台 | 会话数据 | 配置 |
|------|----------|------|
| macOS | `~/Library/Application Support/LocalAIAssistant/` | QSettings |
| Windows | `%APPDATA%/LocalAIAssistant/` | 注册表 |
| Linux | `~/.local/share/LocalAIAssistant/` | `~/.config/` |
</details>

<details>
<summary><b>Q: 构建失败怎么办？</b></summary>

请查看 [BUILD.md - 常见问题与故障排除](BUILD.md#常见问题与故障排除)。
</details>

---

## 技术栈

| 层级 | 技术 |
|------|------|
| 语言 | C++17 |
| 框架 | Qt 6.x (Widgets, Network, LinguistTools) |
| 构建 | CMake 3.16+ |
| 架构 | 分层架构（表示层 / 业务逻辑层 / 数据层） |

---

## 项目结构

```
sourcecode-ai-assistant/
├── src/
│   ├── core/           # 核心业务逻辑
│   ├── ui/             # GUI 界面
│   └── cli/            # CLI 命令行
├── scripts/            # 构建/安装/演示脚本
├── translations/       # 国际化翻译文件
├── docs/               # 项目文档
├── cmake/              # CMake 配置
```

---

## 贡献

欢迎贡献代码、报告问题或提出建议！

请参阅 [CONTRIBUTING.md](CONTRIBUTING.md) 了解贡献指南。

---

## 许可证

本项目采用 **MIT License** 开源许可证。

详见 [LICENSE](../LICENSE) 文件。

---

## 致谢

感谢以下开源项目：

- [Qt](https://www.qt.io/) — 跨平台应用框架
- [Ollama](https://ollama.com/) — 本地 AI 服务

---

**祝您使用愉快！如有问题请提交 Issue。**