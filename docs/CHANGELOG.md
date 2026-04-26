# 更新日志

本项目的所有重要变更都将记录在此文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)。

---

## [Unreleased]

### 计划功能

- 快捷键支持
- 会话导出/导入
- 代码高亮显示

---

## [1.1.0] - 2026-04-26

### 新增功能

- **流式响应 (SSE)** — AI 回复逐字实时显示
- **多语言支持** — 简体中文 / English 动态切换
- **主题切换** — 亮色 / 暗色 / 跟随系统主题
- **TranslationManager** — 统一的翻译管理器

### 改进

- GUI 界面优化
- Markdown 渲染增强
- 消息显示防重入机制

### 技术改进

- 添加 `.clangd` 配置文件
- 生成 `compile_commands.json` 供 IDE 使用

---

## [1.0.0] - 2026-03-xx

### 核心功能

- AI 对话交互（多轮对话，上下文维护）
- GUI 版本 — Qt Widgets 图形界面
- CLI 版本 — 命令行交互模式
- 会话管理 — 创建、切换、删除会话
- 数据持久化 — 自动保存会话和配置
- API 配置 — 支持本地模式和外部 API

### 支持的 AI 服务

- Ollama（推荐）
- LocalAI
- LM Studio
- OpenAI API

### 技术栈

- C++17
- Qt 6.x (Widgets, Network, LinguistTools)
- CMake 3.16+

---

## 版本说明

- **[Unreleased]** — 开发中的功能
- **[1.1.0]** — 流式输出、多语言、主题版本
- **[1.0.0]** — 初始发布版本

---

## 如何贡献

如果您发现 bug 或有功能建议，请参阅 [CONTRIBUTING.md](CONTRIBUTING.md)。