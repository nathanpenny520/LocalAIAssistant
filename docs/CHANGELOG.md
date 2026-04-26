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

## [1.2.0] - 2026-04-26

### 新增功能

- **文件上传功能** — GUI 和 CLI 版本均支持文件附件
  - GUI: 输入框右侧文件按钮，可视化文件列表，支持删除单个文件
  - CLI: `/file <路径>`、`/listfiles`、`/clearfiles` 命令
- **多模态消息支持** — 支持发送图片附件（需模型支持多模态）
  - 文本文件: 直接读取内容发送
  - 图片文件: Base64 编码发送
  - PDF 文件: Poppler 本地提取文本内容
- **PDF 文本提取** — 使用 Poppler 库解析 PDF 文本层
  - 自动提取多页 PDF 文本
  - 支持中文内容（UTF-8 编码）
- **macOS 本地化应用名称** — 根据系统语言显示不同名称
  - 英文系统: "LocalAI Assistant"
  - 中文系统: "本地AI助手"

### 改进

- 文件对话框使用 macOS 原生样式（暗色模式适配）
- 文件标签主题适配（亮色/暗色模式）
- 删除按钮使用 Qt 标准图标

### 技术改进

- 添加 Poppler (poppler-cpp) 依赖用于 PDF 解析
- FileManager 类统一文件处理逻辑
- NetworkManager 支持 OpenAI 多模态消息格式
- macOS app bundle 本地化文件结构

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
- **[1.2.0]** — 文件上传、PDF 解析、多模态支持版本
- **[1.1.0]** — 流式输出、多语言、主题版本
- **[1.0.0]** — 初始发布版本

---

## 如何贡献

如果您发现 bug 或有功能建议，请参阅 [CONTRIBUTING.md](CONTRIBUTING.md)。