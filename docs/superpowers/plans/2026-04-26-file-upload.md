# 文件提交功能实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 CLI 应用添加文件提交功能，支持文本文件读取、图片 base64 编码、其他文件元信息传递。

**Architecture:** 新增 FileManager 类独立处理文件类型判断和内容读取，修改 NetworkManager 支持多模态 JSON 格式，CLI 添加 `/file`, `/listfiles`, `/clearfiles` 命令。

**Tech Stack:** Qt6 C++ (QFile, QFileInfo, QMimeDatabase, base64 编码)

---

## 文件结构

| 文件 | 操作 | 职责 |
|------|------|------|
| `src/core/datamodels.h` | 修改 | 添加 FileAttachment 结构 |
| `src/core/filemanager.h` | 新建 | FileManager 类声明 |
| `src/core/filemanager.cpp` | 新建 | 文件处理实现 |
| `src/core/networkmanager.h` | 修改 | 添加多模态消息构建方法声明 |
| `src/core/networkmanager.cpp` | 修改 | 实现多模态 JSON 格式 |
| `src/cli/cli_application.h` | 修改 | 添加 FileManager 成员 |
| `src/cli/cli_application.cpp` | 修改 | 实现新命令处理 |
| `CMakeLists.txt` | 修改 | 添加 filemanager 到编译目标 |

---

### Task 1: 添加 FileAttachment 数据结构

**Files:**
- Modify: `src/core/datamodels.h`

- [ ] **Step 1: 在 datamodels.h 中添加 FileAttachment 结构**

在 `#ifndef DATAMODELS_H` 后、`struct ChatMessage` 前添加：

```cpp
struct FileAttachment
{
    QString path;           // 文件路径
    QString type;           // 类型标识: "text", "image", "binary"
    QString mimeType;       // MIME 类型: "text/plain", "image/png" 等
    QString content;        // 文本内容或 base64 编码数据
    qint64 size;            // 文件大小（字节）

    FileAttachment() : size(0) {}
    explicit FileAttachment(const QString &p) : path(p), size(0) {}
};
```

修改 `ChatMessage` 结构，添加 attachments 字段：

```cpp
struct ChatMessage
{
    QString role;
    QString content;
    QVector<FileAttachment> attachments;  // 新增

    ChatMessage() = default;
    ChatMessage(const QString &r, const QString &c) : role(r), content(c) {}
};
```

完整的修改后文件内容：

```cpp
#ifndef DATAMODELS_H
#define DATAMODELS_H

#include <QString>
#include <QVector>
#include <QUuid>

struct FileAttachment
{
    QString path;           // 文件路径
    QString type;           // 类型标识: "text", "image", "binary"
    QString mimeType;       // MIME 类型: "text/plain", "image/png" 等
    QString content;        // 文本内容或 base64 编码数据
    qint64 size;            // 文件大小（字节）

    FileAttachment() : size(0) {}
    explicit FileAttachment(const QString &p) : path(p), size(0) {}
};

struct ChatMessage
{
    QString role;
    QString content;
    QVector<FileAttachment> attachments;

    ChatMessage() = default;
    ChatMessage(const QString &r, const QString &c) : role(r), content(c) {}
};

struct ChatSession
{
    QString id;
    QString title;
    QVector<ChatMessage> messages;

    ChatSession() : id(QUuid::createUuid().toString(QUuid::WithoutBraces)) {}
    explicit ChatSession(const QString &sessionTitle) : id(QUuid::createUuid().toString(QUuid::WithoutBraces)), title(sessionTitle) {}
};

#endif
```

- [ ] **Step 2: 验证编译**

```bash
cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/build
cmake .. && make LocalAIAssistantCore -j4
```

Expected: 编译成功，无错误

- [ ] **Step 3: Commit**

```bash
git add src/core/datamodels.h
git commit -m "feat: add FileAttachment structure to datamodels

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 2: 创建 FileManager 类声明

**Files:**
- Create: `src/core/filemanager.h`

- [ ] **Step 1: 创建 filemanager.h**

```cpp
#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include <QVector>
#include "datamodels.h"

class FileManager : public QObject
{
    Q_OBJECT

public:
    explicit FileManager(QObject *parent = nullptr);

    // 添加文件到待发送列表，返回是否成功
    bool addFile(const QString &path);

    // 获取待发送文件列表（用于发送时取走）
    QVector<FileAttachment> pendingFiles() const;

    // 清空待发送列表
    void clearPendingFiles();

    // 获取文件列表摘要（用于 /listfiles 显示）
    QString fileListSummary() const;

    // 待发送文件数量
    int pendingFileCount() const;

    // 判断文件是否为文本类型
    static bool isTextFile(const QString &path);

    // 判断文件是否为图片类型
    static bool isImageFile(const QString &path);

private:
    FileAttachment processFile(const QString &path);
    QString getMimeType(const QString &path);
    QString readTextFile(const QString &path);
    QString encodeImageToBase64(const QString &path);

    QVector<FileAttachment> m_pendingFiles;
};

#endif
```

- [ ] **Step 2: 验证头文件存在**

```bash
ls -la /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/src/core/filemanager.h
```

Expected: 文件存在

- [ ] **Step 3: Commit**

```bash
git add src/core/filemanager.h
git commit -m "feat: add FileManager class declaration

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 3: 实现 FileManager 类

**Files:**
- Create: `src/core/filemanager.cpp`

- [ ] **Step 1: 创建 filemanager.cpp 完整实现**

```cpp
#include "filemanager.h"
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDebug>

namespace {
    // 文本文件扩展名列表
    const QStringList TEXT_EXTENSIONS = {
        "txt", "md", "rst", "cpp", "h", "hpp", "c", "cc", "cxx",
        "py", "js", "ts", "jsx", "tsx", "java", "kt", "kts",
        "go", "rs", "rb", "php", "cs", "swift", "scala",
        "json", "xml", "yaml", "yml", "toml", "ini", "cfg", "conf",
        "html", "htm", "css", "scss", "sass", "less",
        "sql", "sh", "bash", "zsh", "bat", "cmd", "ps1",
        "dockerfile", "makefile", "cmake", "gradle", "maven",
        "log", "csv", "tsv", "env", "gitignore", "dockerignore"
    };

    // 图片文件扩展名列表
    const QStringList IMAGE_EXTENSIONS = {
        "png", "jpg", "jpeg", "gif", "bmp", "webp", "ico", "svg"
    };

    // 最大文件大小限制 (10MB)
    const qint64 MAX_FILE_SIZE = 10 * 1024 * 1024;
}

FileManager::FileManager(QObject *parent)
    : QObject(parent)
    , m_pendingFiles()
{
}

bool FileManager::isTextFile(const QString &path)
{
    QFileInfo info(path);
    QString ext = info.suffix().toLower();

    // 检查扩展名
    if (TEXT_EXTENSIONS.contains(ext)) {
        return true;
    }

    // 特殊文件名（无扩展名）
    QString fileName = info.fileName().toLower();
    if (fileName == "dockerfile" || fileName == "makefile" ||
        fileName == "cmakelists.txt" || fileName.startsWith(".")) {
        return true;
    }

    return false;
}

bool FileManager::isImageFile(const QString &path)
{
    QFileInfo info(path);
    QString ext = info.suffix().toLower();
    return IMAGE_EXTENSIONS.contains(ext);
}

bool FileManager::addFile(const QString &path)
{
    // 检查文件是否存在
    QFileInfo info(path);
    if (!info.exists()) {
        qDebug() << "File does not exist:" << path;
        return false;
    }

    // 检查文件大小
    if (info.size() > MAX_FILE_SIZE) {
        qDebug() << "File too large:" << path << info.size() << "bytes";
        return false;
    }

    // 处理文件
    FileAttachment attachment = processFile(path);
    m_pendingFiles.append(attachment);

    return true;
}

QVector<FileAttachment> FileManager::pendingFiles() const
{
    return m_pendingFiles;
}

void FileManager::clearPendingFiles()
{
    m_pendingFiles.clear();
}

QString FileManager::fileListSummary() const
{
    if (m_pendingFiles.isEmpty()) {
        return "待发送文件列表为空";
    }

    QString summary = "待发送文件列表:\n";
    summary += "----------------------------------------\n";

    for (int i = 0; i < m_pendingFiles.size(); ++i) {
        const FileAttachment &file = m_pendingFiles[i];
        QString typeStr;
        if (file.type == "text") {
            typeStr = "文本";
        } else if (file.type == "image") {
            typeStr = "图片";
        } else {
            typeStr = "二进制";
        }

        summary += QString("  %1. %2 (%3, %4 bytes)\n")
            .arg(i + 1)
            .arg(file.path)
            .arg(typeStr)
            .arg(file.size);
    }

    summary += "----------------------------------------\n";
    summary += QString("共 %1 个文件\n").arg(m_pendingFiles.size());

    return summary;
}

int FileManager::pendingFileCount() const
{
    return m_pendingFiles.size();
}

FileAttachment FileManager::processFile(const QString &path)
{
    FileAttachment attachment;
    attachment.path = path;

    QFileInfo info(path);
    attachment.size = info.size();
    attachment.mimeType = getMimeType(path);

    if (isTextFile(path)) {
        attachment.type = "text";
        attachment.content = readTextFile(path);
    } else if (isImageFile(path)) {
        attachment.type = "image";
        attachment.content = encodeImageToBase64(path);
    } else {
        attachment.type = "binary";
        // 二进制文件不读取内容，只保留元信息
        attachment.content = QString("[二进制文件: %1 (%2 bytes, %3)]")
            .arg(info.fileName())
            .arg(info.size())
            .arg(attachment.mimeType);
    }

    return attachment;
}

QString FileManager::getMimeType(const QString &path)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(path);
    return mime.name();
}

QString FileManager::readTextFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open text file:" << path << file.errorString();
        return QString("[无法读取文件: %1]").arg(file.errorString());
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    // 添加文件头标识
    QFileInfo info(path);
    return QString("[文件: %1]\n\n%2").arg(info.fileName()).arg(content);
}

QString FileManager::encodeImageToBase64(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open image file:" << path << file.errorString();
        return QString();
    }

    QByteArray imageData = file.readAll();
    file.close();

    // 获取 MIME 类型用于 data URL
    QString mime = getMimeType(path);

    // Base64 编码
    QString base64 = imageData.toBase64();

    // 返回 data URL 格式
    return QString("data:%1;base64,%2").arg(mime).arg(base64);
}
```

- [ ] **Step 2: 验证编译**

```bash
cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/build
cmake .. && make LocalAIAssistantCore -j4
```

Expected: 编译成功

- [ ] **Step 3: Commit**

```bash
git add src/core/filemanager.cpp
git commit -m "feat: implement FileManager class

- Support text file reading
- Support image base64 encoding
- Support binary file metadata
- Add file size limit (10MB)

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 4: 更新 CMakeLists.txt

**Files:**
- Modify: `CMakeLists.txt:34-40`

- [ ] **Step 1: 添加 filemanager 到 LocalAIAssistantCore**

修改 `add_library(LocalAIAssistantCore` 部分，添加 filemanager 源文件：

```cmake
add_library(LocalAIAssistantCore
    src/core/networkmanager.cpp
    src/core/networkmanager.h
    src/core/sessionmanager.cpp
    src/core/sessionmanager.h
    src/core/datamodels.h
    src/core/filemanager.cpp
    src/core/filemanager.h
)
```

- [ ] **Step 2: 验证编译**

```bash
cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/build
cmake .. && make LocalAIAssistantCore -j4
```

Expected: 编译成功

- [ ] **Step 3: Commit**

```bash
git add CMakeLists.txt
git commit -m "build: add FileManager to LocalAIAssistantCore

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 5: 修改 NetworkManager 支持多模态消息

**Files:**
- Modify: `src/core/networkmanager.h:49`
- Modify: `src/core/networkmanager.cpp:228-248`

- [ ] **Step 1: 在 networkmanager.h 添加私有方法声明**

在 `buildMessagesArray` 声明后添加：

```cpp
    QJsonArray buildMessagesArray(const QVector<ChatMessage> &messages, int maxMessages);
    QJsonObject buildTextContentBlock(const QString &text);
    QJsonObject buildImageContentBlock(const QString &base64Data, const QString &mime);
    QJsonObject buildFileContentBlock(const FileAttachment &file);
```

完整的私有方法部分：

```cpp
private:
    void loadSettings();
    void saveSettings();
    QString loadSystemPrompt();
    QString extractContentFromResponse(const QByteArray &data);
    QString extractDeltaFromSSE(const QByteArray &data, bool isOllama = false);
    QJsonArray buildMessagesArray(const QVector<ChatMessage> &messages, int maxMessages);
    QJsonObject buildTextContentBlock(const QString &text);
    QJsonObject buildImageContentBlock(const QString &base64Data, const QString &mime);
    QJsonObject buildFileContentBlock(const FileAttachment &file);
```

- [ ] **Step 2: 在 networkmanager.cpp 实现多模态消息构建**

在文件开头添加 `#include "filemanager.h"`：

```cpp
#include "networkmanager.h"
#include "filemanager.h"
#include <QDebug>
#include <QBuffer>
```

修改 `buildMessagesArray` 方法，处理 attachments：

```cpp
QJsonArray NetworkManager::buildMessagesArray(const QVector<ChatMessage> &messages, int maxMessages)
{
    QJsonArray jsonMessages;

    // System message
    QJsonObject systemObj;
    systemObj["role"] = "system";
    systemObj["content"] = m_systemPrompt;
    jsonMessages.append(systemObj);

    int totalCount = messages.size();
    int startIndex = qMax(0, totalCount - maxMessages);

    for (int i = startIndex; i < totalCount; ++i) {
        const ChatMessage &msg = messages[i];
        QJsonObject msgObj;
        msgObj["role"] = msg.role;

        // 检查是否有附件
        if (msg.attachments.isEmpty()) {
            // 无附件，使用简单字符串格式
            msgObj["content"] = msg.content;
        } else {
            // 有附件，使用多模态数组格式
            QJsonArray contentArray;

            // 先添加用户文本（如果有）
            if (!msg.content.isEmpty()) {
                contentArray.append(buildTextContentBlock(msg.content));
            }

            // 添加所有附件
            for (const FileAttachment &file : msg.attachments) {
                if (file.type == "image") {
                    contentArray.append(buildImageContentBlock(file.content, file.mimeType));
                } else {
                    contentArray.append(buildFileContentBlock(file));
                }
            }

            msgObj["content"] = contentArray;
        }

        jsonMessages.append(msgObj);
    }

    return jsonMessages;
}

QJsonObject NetworkManager::buildTextContentBlock(const QString &text)
{
    QJsonObject block;
    block["type"] = "text";
    block["text"] = text;
    return block;
}

QJsonObject NetworkManager::buildImageContentBlock(const QString &base64Data, const QString &mime)
{
    QJsonObject block;
    block["type"] = "image_url";

    QJsonObject imageUrl;
    imageUrl["url"] = base64Data;  // 已经是 data:mime;base64,... 格式
    block["image_url"] = imageUrl;

    return block;
}

QJsonObject NetworkManager::buildFileContentBlock(const FileAttachment &file)
{
    QJsonObject block;
    block["type"] = "text";

    // 文件内容已经在 FileManager 中格式化
    block["text"] = file.content;

    return block;
}
```

- [ ] **Step 3: 验证编译**

```bash
cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/build
cmake .. && make LocalAIAssistantCore -j4
```

Expected: 编译成功

- [ ] **Step 4: Commit**

```bash
git add src/core/networkmanager.h src/core/networkmanager.cpp
git commit -m "feat: add multimodal message support to NetworkManager

- Support content array format for attachments
- Add image_url block for images
- Add text block for text/binary files

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 6: 修改 CLIApplication 添加 FileManager

**Files:**
- Modify: `src/cli/cli_application.h:38-43`

- [ ] **Step 1: 在 cli_application.h 添加 FileManager 成员**

修改头部 include 和私有成员：

```cpp
#ifndef CLI_APPLICATION_H
#define CLI_APPLICATION_H

#include <QObject>
#include <QCommandLineParser>

class NetworkManager;
class FileManager;  // 新增

class CLIApplication : public QObject
{
    Q_OBJECT

public:
    explicit CLIApplication(QObject *parent = nullptr);
    int run(int argc, char *argv[]);

private:
    void printUsage();
    int runInteractiveMode(QCoreApplication &app, const QCommandLineParser &parser);
    void readInput();
    void handleCommand(const QString &command);
    void showHelp();
    void listSessions();
    void showConfig();
    int runSingleQuery(QCoreApplication &app, const QString &query, const QCommandLineParser &parser);
    int handleSessionsCommand(const QCommandLineParser &parser);
    void showSessionContent(const QString &sessionId);
    int handleConfigCommand(const QCommandLineParser &parser);
    void quit();

    // 文件相关命令
    void handleFileCommand(const QString &command);   // 新增
    void listFiles();                                  // 新增

private slots:
    void onResponseReceived(const QString &response);
    void onErrorOccurred(const QString &error);
    void onStreamChunkReceived(const QString &chunk);
    void onStreamFinished(const QString &fullContent);

private:
    NetworkManager *m_networkManager;
    FileManager *m_fileManager;  // 新增
    bool m_running;
    bool m_interactiveMode;
    bool m_isStreaming;
    QString m_streamingContent;
};

#endif
```

- [ ] **Step 2: 验证头文件语法**

```bash
cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/build
cmake .. && make LocalAIAssistant-CLI -j4 2>&1 | head -20
```

Expected: 可能有未实现的错误，但头文件语法正确

- [ ] **Step 3: Commit**

```bash
git add src/cli/cli_application.h
git commit -m "feat: add FileManager member to CLIApplication

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 7: 实现 CLI 文件命令

**Files:**
- Modify: `src/cli/cli_application.cpp`

- [ ] **Step 1: 在 cli_application.cpp 添加 include 和初始化**

在文件开头添加：

```cpp
#include "cli_application.h"
#include "filemanager.h"  // 新增
#include <QCoreApplication>
...
```

在构造函数中初始化 FileManager：

```cpp
CLIApplication::CLIApplication(QObject *parent)
    : QObject(parent)
    , m_networkManager(nullptr)
    , m_fileManager(nullptr)  // 新增
    , m_running(false)
    , m_interactiveMode(false)
    , m_isStreaming(false)
{
}
```

在 `run` 方法中创建 FileManager（在 NetworkManager 创建后）：

```cpp
    m_networkManager = new NetworkManager(this);
    m_fileManager = new FileManager(this);  // 新增
    connect(m_networkManager, &NetworkManager::responseReceived,
...
```

- [ ] **Step 2: 更新帮助信息**

在 `showHelp()` 方法中添加文件命令说明：

```cpp
void CLIApplication::showHelp()
{
    std::cout << "\n可用命令:\n";
    std::cout << "  /help     - 显示此帮助信息\n";
    std::cout << "  /new      - 创建新会话\n";
    std::cout << "  /list     - 列出所有会话\n";
    std::cout << "  /switch <id> - 切换到指定会话\n";
    std::cout << "  /delete <id> - 删除指定会话\n";
    std::cout << "  /config   - 显示当前配置\n";
    std::cout << "  /stream   - 切换流式输出开/关\n";
    std::cout << "  /clear    - 清屏\n";
    std::cout << "  /file <路径> - 添加文件到待发送列表\n";     // 新增
    std::cout << "  /listfiles   - 显示待发送文件列表\n";        // 新增
    std::cout << "  /clearfiles  - 清空待发送文件列表\n";        // 新增
    std::cout << "  /exit     - 退出程序\n";
    std::cout << "\n直接输入文本即可与AI对话\n";
}
```

- [ ] **Step 3: 更新交互模式启动提示**

在 `runInteractiveMode()` 中添加文件命令提示：

```cpp
    std::cout << "命令:\n";
    std::cout << "  /help     - 显示帮助信息\n";
    std::cout << "  /new      - 创建新会话\n";
    std::cout << "  /list     - 列出所有会话\n";
    std::cout << "  /switch <id> - 切换会话\n";
    std::cout << "  /delete <id> - 删除会话\n";
    std::cout << "  /config   - 显示配置\n";
    std::cout << "  /stream   - 切换流式输出\n";
    std::cout << "  /clear    - 清屏\n";
    std::cout << "  /file <路径> - 添加文件\n";      // 新增
    std::cout << "  /listfiles   - 显示文件列表\n";  // 新增
    std::cout << "  /clearfiles  - 清空文件列表\n";  // 新增
    std::cout << "  /exit     - 退出程序\n";
```

- [ ] **Step 4: 实现 handleFileCommand 和 listFiles 方法**

在 `handleCommand()` 中添加文件命令处理（在 `/clear` 后、`/exit` 前）：

```cpp
    } else if (cmd == "/clear") {
        #ifdef Q_OS_WIN
            system("cls");
        #else
            system("clear");
        #endif
    } else if (cmd.startsWith("/file ")) {
        handleFileCommand(command);
        return;
    } else if (cmd == "/listfiles") {
        listFiles();
    } else if (cmd == "/clearfiles") {
        m_fileManager->clearPendingFiles();
        std::cout << "已清空待发送文件列表" << std::endl;
    } else if (cmd == "/exit" || cmd == "/quit") {
```

在文件末尾（`quit()` 方法前）添加实现：

```cpp
void CLIApplication::handleFileCommand(const QString &command)
{
    // 提取文件路径（支持空格路径：用引号包裹）
    QString arg = command.mid(6).trimmed();

    if (arg.isEmpty()) {
        std::cout << "用法: /file <文件路径>" << std::endl;
        std::cout << "提示: 路径含空格时请用引号包裹，如 /file \"path with space/file.txt\"" << std::endl;
        QTimer::singleShot(0, this, &CLIApplication::readInput);
        return;
    }

    // 解析路径（处理引号）
    QString filePath = arg;
    if (filePath.startsWith('"') && filePath.endsWith('"')) {
        filePath = filePath.mid(1, filePath.length() - 2);
    }

    // 添加文件
    QFileInfo info(filePath);
    if (!info.exists()) {
        std::cout << "错误: 文件不存在: " << filePath.toStdString() << std::endl;
        QTimer::singleShot(0, this, &CLIApplication::readInput);
        return;
    }

    if (info.size() > 10 * 1024 * 1024) {
        std::cout << "错误: 文件过大 (>10MB): " << filePath.toStdString() << std::endl;
        QTimer::singleShot(0, this, &CLIApplication::readInput);
        return;
    }

    if (m_fileManager->addFile(filePath)) {
        QString typeStr;
        if (FileManager::isTextFile(filePath)) {
            typeStr = "文本";
        } else if (FileManager::isImageFile(filePath)) {
            typeStr = "图片";
        } else {
            typeStr = "二进制";
        }
        std::cout << "已添加文件: " << filePath.toStdString()
                  << " (" << typeStr.toStdString() << ", "
                  << info.size() << " bytes)" << std::endl;
        std::cout << "当前待发送文件数: " << m_fileManager->pendingFileCount() << std::endl;
    } else {
        std::cout << "错误: 无法添加文件" << std::endl;
    }

    QTimer::singleShot(0, this, &CLIApplication::readInput);
}

void CLIApplication::listFiles()
{
    QString summary = m_fileManager->fileListSummary();
    std::cout << summary.toStdString() << std::endl;
    QTimer::singleShot(0, this, &CLIApplication::readInput);
}
```

- [ ] **Step 5: 修改 readInput 发送消息时包含文件**

修改发送消息逻辑，添加文件附件：

```cpp
void CLIApplication::readInput()
{
    std::cout << "\n你: ";
    std::cout.flush();

    std::string input;
    if (!std::getline(std::cin, input)) {
        quit();
        return;
    }

    QString qInput = QString::fromStdString(input).trimmed();

    if (qInput.isEmpty()) {
        QTimer::singleShot(0, this, &CLIApplication::readInput);
        return;
    }

    if (qInput.startsWith("/")) {
        handleCommand(qInput);
        return;
    }

    m_running = true;
    m_isStreaming = false;
    m_streamingContent.clear();

    // 创建消息并添加附件
    ChatMessage userMsg("user", qInput);
    if (m_fileManager->pendingFileCount() > 0) {
        userMsg.attachments = m_fileManager->pendingFiles();
        std::cout << "发送消息时携带 " << userMsg.attachments.size() << " 个文件" << std::endl;
    }
    SessionManager::instance()->addMessageToCurrentSession("user", qInput);

    // 更新会话中最后一条消息的附件
    // SessionManager 需要支持带附件的消息，暂时直接设置
    // 这里我们需要修改 SessionManager 或使用其他方式
    // 简化方案：直接修改刚添加的消息的 attachments

    if (m_networkManager->isStreamingEnabled()) {
        std::cout << "\nAI: " << std::flush;
    } else {
        std::cout << "\nAI正在思考..." << std::endl;
    }

    // 构建带附件的消息列表发送
    QVector<ChatMessage> messages = SessionManager::instance()->currentSession().messages;
    if (!userMsg.attachments.isEmpty()) {
        // 修改最后一条消息添加附件
        if (!messages.isEmpty()) {
            messages.last().attachments = userMsg.attachments;
        }
        // 发送后清空待发送文件列表
        m_fileManager->clearPendingFiles();
    }

    m_networkManager->sendChatRequestWithContext(messages);
}
```

- [ ] **Step 6: 验证编译**

```bash
cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/build
cmake .. && make LocalAIAssistant-CLI -j4
```

Expected: 编译成功

- [ ] **Step 7: Commit**

```bash
git add src/cli/cli_application.cpp
git commit -m "feat: add file commands to CLI

- /file <path> - add file to pending list
- /listfiles - show pending files
- /clearfiles - clear pending files
- Attach files to message on send

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 8: 更新 printUsage 添加文件命令说明

**Files:**
- Modify: `src/cli/cli_application.cpp:138-177`

- [ ] **Step 1: 在 printUsage 中添加文件命令**

在 "输出选项" 后添加 "文件选项" 部分：

```cpp
void CLIApplication::printUsage()
{
    std::cout << "\n本地AI助手 - 命令行版本 v1.0.0\n\n";
    std::cout << "用法(ai 是 ./LocalAIAssistant-CLI 缩写，可以自己取alias):\n";
    std::cout << "  ai chat                    进入交互式对话模式\n";
    std::cout << "  ai ask <问题>              单次查询\n";
    std::cout << "  ai sessions [选项]         会话管理\n";
    std::cout << "  ai config [选项]           配置管理\n\n";
    std::cout << "会话管理选项:\n";
    std::cout << "  -l, --list                 列出所有会话\n";
    std::cout << "  -n, --new                  创建新会话\n";
    std::cout << "  -d, --delete <id>          删除指定会话\n";
    std::cout << "  --show <id>                显示会话内容\n";
    std::cout << "  -s, --session <id>         切换到指定会话\n\n";
    std::cout << "配置管理选项:\n";
    std::cout << "  --api-url <url>            设置API基础URL\n";
    std::cout << "  --api-key <key>            设置API密钥\n";
    std::cout << "  --model <name>             设置模型名称\n";
    std::cout << "  --local                    使用本地模式\n";
    std::cout << "  --external                 使用外部API模式\n";
    std::cout << "  --show-config              显示当前配置\n\n";
    std::cout << "模型参数选项:\n";
    std::cout << "  --temperature <value>      设置温度 (0.0-2.0)\n";
    std::cout << "  --top-p <value>            设置 top_p (0.0-1.0)\n";
    std::cout << "  --max-tokens <value>       设置最大输出 tokens (1-128000)\n";
    std::cout << "  --presence-penalty <value> 设置存在惩罚 (-2.0-2.0)\n";
    std::cout << "  --frequency-penalty <value> 设置频率惩罚 (-2.0-2.0)\n";
    std::cout << "  --seed <value>             设置随机种子 (整数，-1清除)\n";
    std::cout << "  --max-context <value>      设置最大上下文消息数 (1-100)\n\n";
    std::cout << "输出选项:\n";
    std::cout << "  --no-stream                禁用流式输出，等待完整响应\n\n";
    std::cout << "文件选项 (交互模式):\n";                    // 新增
    std::cout << "  /file <路径>               添加文件到待发送列表\n";  // 新增
    std::cout << "  /listfiles                 显示待发送文件列表\n";   // 新增
    std::cout << "  /clearfiles                清空待发送文件列表\n\n"; // 新增
    std::cout << "示例:\n";
    std::cout << "  ai chat                    # 启动交互式对话\n";
    std::cout << "  ai ask \"什么是量子计算?\"   # 单次查询\n";
    std::cout << "  ai ask \"你好\" --no-stream  # 禁用流式输出\n";
    std::cout << "  ai sessions -l             # 列出所有会话\n";
    std::cout << "  ai config --show-config    # 显示当前配置\n";
    std::cout << "  ai config --temperature 0.7 --top-p 0.9  # 设置模型参数\n";
    std::cout << std::endl;
}
```

- [ ] **Step 2: 验证编译**

```bash
cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/build
cmake .. && make LocalAIAssistant-CLI -j4
```

Expected: 编译成功

- [ ] **Step 3: Commit**

```bash
git add src/cli/cli_application.cpp
git commit -m "docs: add file commands to printUsage

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 9: 集成测试

- [ ] **Step 1: 编译完整项目**

```bash
cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/build
cmake .. && make -j4
```

Expected: 全部编译成功

- [ ] **Step 2: 启动 CLI 测试基本功能**

```bash
./LocalAIAssistant-CLI chat
```

输入测试：
1. `/help` - 查看是否显示文件命令
2. `/file /path/to/test.txt` - 测试添加文本文件
3. `/listfiles` - 查看文件列表
4. `/clearfiles` - 清空列表
5. `/exit` - 退出

- [ ] **Step 3: 测试文件不存在情况**

```bash
./LocalAIAssistant-CLI chat
/file /nonexistent/file.txt
```

Expected: 显示 "错误: 文件不存在"

- [ ] **Step 4: 测试发送带文件的消息**

准备一个测试文本文件，然后：

```bash
./LocalAIAssistant-CLI chat
/file /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/src/core/datamodels.h
请解释这个文件的作用
```

Expected: AI 收到文件内容并给出解释

---

### Task 10: 最终提交

- [ ] **Step 1: 确认所有修改已提交**

```bash
git status
```

Expected: 无未提交的修改

- [ ] **Step 2: 查看提交历史**

```bash
git log --oneline -10
```

- [ ] **Step 3: 确认功能完整性**

手动验证：
- `/file` 命令工作
- `/listfiles` 显示正确
- `/clearfiles` 清空有效
- 发送消息时文件被携带
- 文件不存在时有错误提示
- 文件过大时有错误提示

---

## Spec 自检

| Spec 要求 | Task |
|-----------|------|
| FileAttachment 结构 | Task 1 |
| FileManager 类 | Task 2, 3 |
| 文本文件读取 | Task 3 |
| 图片 base64 | Task 3 |
| 二进制元信息 | Task 3 |
| NetworkManager 多模态 | Task 5 |
| `/file` 命令 | Task 7 |
| `/listfiles` 命令 | Task 7 |
| `/clearfiles` 命令 | Task 7 |
| 文件累积模式 | Task 7 (readInput) |
| 错误处理（不存在、过大） | Task 7 |
| CMakeLists 更新 | Task 4 |