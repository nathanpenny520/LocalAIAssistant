# 本地AI助手 (Local AI Assistant)

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Qt](https://img.shields.io/badge/Qt-6.x-green.svg)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C++-17-orange.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Windows%20%7C%20Linux-lightgrey.svg)](https://github.com)

> 一个功能完整的本地AI助手桌面应用，支持 GUI 和 CLI 双模式，可与 Ollama、LM Studio、OpenAI 等服务交互。

## 功能亮点

| 功能 | 说明 |
|------|------|
| 双模式 | GUI 图形界面 + CLI 命令行 |
| 流式输出 | SSE 实时显示 AI 回复 |
| 会话管理 | 多会话切换、历史持久化 |
| 多语言 | 中文 / English 即时切换 |
| 主题 | 亮色 / 暗色 / 跟随系统 |

## 快速开始

```bash
# 1. 构建
./scripts/build.sh        # macOS/Linux
scripts\build.bat         # Windows

# 2. 安装 AI 服务（必需！）
# 下载 Ollama: https://ollama.com/download
ollama pull llama3

# 3. 运行
./build/LocalAIAssistant-CLI chat    # CLI
open build/LocalAIAssistant.app      # GUI (macOS)
```

## 文档

| 文档 | 说明 |
|------|------|
| [docs/README.md](docs/README.md) | 完整使用指南 |
| [docs/BUILD.md](docs/BUILD.md) | 构建与安装指南 |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | 架构设计详情 |
| [docs/CHANGELOG.md](docs/CHANGELOG.md) | 版本更新日志 |
| [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) | 贡献指南 |

## 许可证

[MIT License](LICENSE)

---

**详细说明请参阅 [docs/README.md](docs/README.md)**