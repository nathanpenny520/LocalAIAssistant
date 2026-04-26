---
title: 文件提交功能设计
date: 2026-04-26
status: approved
---

# 文件提交功能设计

## 概述

为 CLI 应用添加文件提交功能，允许用户在对话中上传文件供 AI 分析。

## 需求

- 支持文本文件直接读取
- 支持图片 base64 编码（多模态模型）
- 其他文件只传元信息（文件名、大小、类型）
- 纯命令行交互：`/file <路径>`
- 支持累积多个文件，手动清除

## 架构

新增 `FileManager` 类独立处理文件：

```
CLIApplication  --->  FileManager  --->  NetworkManager
    (/file)           (读取/编码)       (发送多模态消息)
```

## 数据结构

### FileAttachment（新增）

```cpp
struct FileAttachment
{
    QString path;           // 文件路径
    QString type;           // 类型: "text", "image", "binary"
    QString mimeType;       // MIME 类型
    QString content;        // 文本内容或 base64 编码
    qint64 size;            // 文件大小（字节）
};
```

### ChatMessage（修改）

添加 `attachments` 字段：

```cpp
struct ChatMessage
{
    QString role;
    QString content;
    QVector<FileAttachment> attachments;
};
```

## FileManager 类

**职责**：
- 文件类型判断（文本/图片/其他）
- 文本文件读取
- 图片 base64 编码
- 管理待发送文件列表

**公共方法**：
- `addFile(path)` - 添加文件
- `pendingFiles()` - 获取待发送列表
- `clearPendingFiles()` - 清空列表
- `fileListSummary()` - 获取列表摘要

**文件类型判断**：
- 文本：.txt, .md, .cpp, .h, .py, .js, .json, .xml, .yaml, .csv, .html, .css, .sql, .sh, .bat 等
- 图片：.png, .jpg, .jpeg, .gif, .bmp, .webp
- 其他：标记为 binary

## NetworkManager 多模态格式

当消息包含 attachments，生成 OpenAI 多模态 JSON：

```json
{
  "role": "user",
  "content": [
    { "type": "text", "text": "用户文字" },
    { "type": "image_url", "image_url": { "url": "data:image/png;base64,..." } },
    { "type": "text", "text": "[文件: filename]\n内容..." }
  ]
}
```

**消息块规则**：
- 用户文字：`{ "type": "text", "text": "..." }`
- 图片：`{ "type": "image_url", "image_url": { "url": "data:<mime>;base64,<data>" } }`
- 文本文件：`{ "type": "text", "text": "[文件: filename]\n<内容>" }`
- 二进制文件：`{ "type": "text", "text": "[文件: filename (<size> bytes, <mime>)]" }`

## CLI 命令

| 命令 | 功能 |
|------|------|
| `/file <路径>` | 添加文件 |
| `/listfiles` | 显示待发送文件列表 |
| `/clearfiles` | 清空文件列表 |

## 错误处理

- 文件不存在：提示错误，不添加
- 文件过大（>10MB）：警告或拒绝
- 读取失败：提示错误原因
- 支持路径含空格（引号包裹）

## 文件列表

1. `src/core/filemanager.h` - 新建
2. `src/core/filemanager.cpp` - 新建
3. `src/core/datamodels.h` - 修改，添加 FileAttachment
4. `src/core/networkmanager.h` - 修改，声明新方法
5. `src/core/networkmanager.cpp` - 修改，支持多模态格式
6. `src/cli/cli_application.h` - 修改，添加 FileManager
7. `src/cli/cli_application.cpp` - 修改，添加新命令
8. `CMakeLists.txt` - 修改，添加新源文件