# GUI 文件提交功能实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 GUI 版本添加文件提交功能，复用 CLI 版本的 FileManager 类。

**Architecture:** 在 MainWindow 中添加 FileManager 成员、文件按钮、文件列表显示区域，修改发送逻辑携带文件附件。

**Tech Stack:** Qt6 C++ (QPushButton, QFileDialog, QLabel, QHBoxLayout, FileManager)

---

## 文件结构

| 文件 | 操作 | 职责 |
|------|------|------|
| `src/ui/mainwindow.h` | 修改 | 添加 FileManager 成员和新方法声明 |
| `src/ui/mainwindow.cpp` | 修改 | 实现文件按钮、文件列表、发送逻辑 |

---

### Task 1: 修改 mainwindow.h 添加声明

**Files:**
- Modify: `src/ui/mainwindow.h`

- [ ] **Step 1: 添加 include 和前向声明**

在文件开头，`#include "sessionmanager.h"` 后添加：

```cpp
#include "sessionmanager.h"
#include "filemanager.h"  // 新增
```

- [ ] **Step 2: 添加私有成员和方法声明**

在 `private slots:` 部分，`onLanguageChanged()` 后添加：

```cpp
    void onLanguageChanged();

    // 文件相关
    void onFileButtonClicked();           // 新增
    void onRemoveFileClicked();           // 新增
```

在 `private:` 成员部分，`m_isStreaming` 后添加：

```cpp
    bool m_isStreaming;
    QString m_streamingContent;
    bool m_isRendering = false;

    // 文件相关成员
    FileManager *m_fileManager;           // 新增
    QPushButton *m_fileButton;            // 新增
    QWidget *m_fileListArea;              // 新增
    QHBoxLayout *m_fileListLayout;        // 新增
    void updateFileListDisplay();         // 新增
    void clearFileListDisplay();          // 新增
```

完整的修改后 mainwindow.h：

```cpp
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTextBrowser>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QAction>
#include <QCloseEvent>
#include <QMenu>
#include <QTextDocument>
#include <QMap>
#include "networkmanager.h"
#include "settingsdialog.h"
#include "sessionmanager.h"
#include "filemanager.h"  // 新增

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onSendClicked();
    void onNetworkFinished(const QString &response);
    void onNetworkError(const QString &error);
    void onStreamChunkReceived(const QString &chunk);
    void onStreamFinished(const QString &fullContent);
    void onSettingsClicked();
    void onNewChatClicked();
    void onSessionItemClicked(QListWidgetItem *item);
    void onDeleteSession();
    void onCustomContextMenuRequested(const QPoint &pos);
    void onThemeChanged(int theme);
    void onLanguageChanged();

    // 文件相关
    void onFileButtonClicked();           // 新增
    void onRemoveFileClicked();           // 新增

private:
    void setupUI();
    void setupMenuBar();
    void retranslateUi();
    void appendChatMessage(const QString &sender, const QString &message);
    void renderCurrentSession();
    void updateSessionList();
    void setInputEnabled(bool enabled);
    QMap<QString, QString> parseThinkingContent(const QString &content);
    QString formatMessageWithThinking(const QString &role, const QString &content);

    QListWidget *m_historyList;
    QTextBrowser *m_chatDisplay;
    QLineEdit *m_inputLine;
    QPushButton *m_sendButton;
    QPushButton *m_newChatButton;
    QAction *m_settingsAction;
    QMenu *m_contextMenu;
    QAction *m_deleteAction;
    NetworkManager *m_networkManager;
    QMap<QString, QListWidgetItem*> m_sessionItemMap;
    QTextDocument *m_markdownDoc;

    bool m_isStreaming;
    QString m_streamingContent;
    bool m_isRendering = false;

    // 文件相关成员
    FileManager *m_fileManager;           // 新增
    QPushButton *m_fileButton;            // 新增
    QWidget *m_fileListArea;              // 新增
    QHBoxLayout *m_fileListLayout;        // 新增
    void updateFileListDisplay();         // 新增
    void clearFileListDisplay();          // 新增
};

#endif
```

- [ ] **Step 2: 验证编译**

```bash
cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/build
cmake .. && make LocalAIAssistant -j4 2>&1 | tail -20
```

Expected: 有链接错误（方法未实现），但头文件语法正确

- [ ] **Step 3: Commit**

```bash
git add src/ui/mainwindow.h
git commit -m "feat: add FileManager and file button declarations to MainWindow

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 2: 实现 FileManager 初始化和文件按钮

**Files:**
- Modify: `src/ui/mainwindow.cpp`

- [ ] **Step 1: 在构造函数初始化列表添加 FileManager 和文件按钮**

修改构造函数（大约第 115-128 行）：

```cpp
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_historyList(new QListWidget(this))
    , m_chatDisplay(new QTextBrowser(this))
    , m_inputLine(new QLineEdit(this))
    , m_sendButton(new QPushButton(tr("发送"), this))
    , m_newChatButton(new QPushButton(tr("+ 新建对话"), this))
    , m_settingsAction(new QAction(tr("设置"), this))
    , m_contextMenu(new QMenu(this))
    , m_deleteAction(new QAction(tr("删除该对话"), this))
    , m_networkManager(new NetworkManager(this))
    , m_markdownDoc(new QTextDocument(this))
    , m_isStreaming(false)
    , m_fileManager(new FileManager(this))           // 新增
    , m_fileButton(new QPushButton(tr("📁"), this))  // 新增
    , m_fileListArea(nullptr)                        // 新增
    , m_fileListLayout(nullptr)                      // 新增
{
```

- [ ] **Step 2: 在构造函数中添加文件按钮连接**

在构造函数的 connect 部分（大约第 132-146 行），在 `connect(m_sendButton...` 前添加：

```cpp
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_fileButton, &QPushButton::clicked, this, &MainWindow::onFileButtonClicked);  // 新增
    connect(m_inputLine, &QLineEdit::returnPressed, this, &MainWindow::onSendClicked);
```

- [ ] **Step 3: 在 setupUI 中添加文件按钮和文件列表区域**

修改 setupUI 方法中的输入布局部分（大约第 195-198 行）：

找到：
```cpp
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(m_inputLine);
    inputLayout->addWidget(m_sendButton);
    rightLayout->addLayout(inputLayout);
```

修改为：
```cpp
    // 文件列表区域（输入框上方）
    m_fileListArea = new QWidget(rightPanel);
    m_fileListLayout = new QHBoxLayout(m_fileListArea);
    m_fileListLayout->setContentsMargins(0, 0, 0, 5);
    m_fileListLayout->addStretch();  // 左侧留空，文件标签靠左排列
    m_fileListArea->setVisible(false);  // 默认隐藏，有文件时显示
    rightLayout->addWidget(m_fileListArea);

    // 输入区域（输入框 + 文件按钮 + 发送按钮）
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(m_inputLine, 1);  // 输入框占主要空间
    inputLayout->addWidget(m_fileButton);    // 新增：文件按钮
    inputLayout->addWidget(m_sendButton);
    rightLayout->addLayout(inputLayout);

    // 设置文件按钮样式
    m_fileButton->setFixedSize(40, 30);
    m_fileButton->setToolTip(tr("添加文件"));
```

- [ ] **Step 4: 验证编译**

```bash
cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/build
cmake .. && make LocalAIAssistant -j4 2>&1 | tail -20
```

Expected: 有链接错误（onFileButtonClicked 未实现）

- [ ] **Step 5: Commit**

```bash
git add src/ui/mainwindow.cpp
git commit -m "feat: add FileManager initialization and file button to MainWindow

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 3: 实现文件选择和列表显示

**Files:**
- Modify: `src/ui/mainwindow.cpp`

- [ ] **Step 1: 添加 QFileDialog 和 QMessageBox include**

在文件开头（大约第 1-15 行），添加：

```cpp
#include "mainwindow.h"
#include "stylesheetmanager.h"
#include "translationmanager.h"
#include "markdownrenderer.h"
#include "filemanager.h"  // 新增
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QScrollBar>
#include <QTextCursor>
#include <QEvent>
#include <QRegularExpression>
#include <QFileDialog>    // 新增
#include <QMessageBox>    // 新增
```

- [ ] **Step 2: 实现 onFileButtonClicked 方法**

在文件末尾（`onLanguageChanged()` 方法后）添加：

```cpp
void MainWindow::onFileButtonClicked()
{
    QString locale = TranslationManager::instance()->currentLocale();
    QString title = (locale == "en") ? "Select Files" : QStringLiteral("选择文件");

    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        title,
        QString(),
        tr("All Files (*.*)")
    );

    if (filePaths.isEmpty()) {
        return;
    }

    for (const QString &path : filePaths) {
        QFileInfo info(path);
        if (!info.exists()) {
            QString msg = (locale == "en")
                ? QString("File does not exist: %1").arg(path)
                : QString(QStringLiteral("文件不存在: %1")).arg(path);
            QMessageBox::warning(this, title, msg);
            continue;
        }

        if (info.size() > 10 * 1024 * 1024) {
            QString msg = (locale == "en")
                ? QString("File too large (>10MB): %1").arg(path)
                : QString(QStringLiteral("文件过大 (>10MB): %1")).arg(path);
            QMessageBox::warning(this, title, msg);
            continue;
        }

        if (m_fileManager->addFile(path)) {
            qDebug() << "File added:" << path;
        }
    }

    updateFileListDisplay();
}
```

- [ ] **Step 3: 实现 updateFileListDisplay 方法**

在 `onFileButtonClicked()` 后添加：

```cpp
void MainWindow::updateFileListDisplay()
{
    // 清除现有文件标签
    clearFileListDisplay();

    QVector<FileAttachment> files = m_fileManager->pendingFiles();
    if (files.isEmpty()) {
        m_fileListArea->setVisible(false);
        return;
    }

    m_fileListArea->setVisible(true);

    QString locale = TranslationManager::instance()->currentLocale();

    for (const FileAttachment &file : files) {
        // 创建文件标签 widget
        QWidget *fileTag = new QWidget(m_fileListArea);
        QHBoxLayout *tagLayout = new QHBoxLayout(fileTag);
        tagLayout->setContentsMargins(4, 2, 4, 2);
        tagLayout->setSpacing(4);

        // 文件类型颜色
        QString borderColor;
        if (file.type == "text") {
            borderColor = "#007aff";  // 蓝色
        } else if (file.type == "image") {
            borderColor = "#34c759";  // 绿色
        } else {
            borderColor = "#8e8e93";  // 灰色
        }

        // 文件名标签
        QString displayName = QFileInfo(file.path).fileName();
        if (displayName.length() > 20) {
            displayName = displayName.left(17) + "...";
        }

        QLabel *nameLabel = new QLabel(displayName, fileTag);
        nameLabel->setStyleSheet(QString(
            "QLabel { color: #333; font-size: 12px; padding: 2px 6px; "
            "border: 1px solid %1; border-radius: 4px; background: #f5f5f5; }"
        ).arg(borderColor));
        tagLayout->addWidget(nameLabel);

        // 删除按钮
        QPushButton *removeBtn = new QPushButton("×", fileTag);
        removeBtn->setFixedSize(20, 20);
        removeBtn->setStyleSheet(
            "QPushButton { color: #ff3b30; font-size: 14px; font-weight: bold; "
            "border: none; background: transparent; }"
            "QPushButton:hover { background: #ffebeb; border-radius: 10px; }"
        );
        removeBtn->setProperty("filePath", file.path);  // 存储文件路径用于删除
        connect(removeBtn, &QPushButton::clicked, this, &MainWindow::onRemoveFileClicked);
        tagLayout->addWidget(removeBtn);

        // 添加到文件列表布局（在 stretch 之前插入）
        m_fileListLayout->insertWidget(m_fileListLayout->count() - 1, fileTag);
    }

    // 更新文件按钮 tooltip 显示文件数量
    QString tooltip = (locale == "en")
        ? QString("Add Files (%1 pending)").arg(files.size())
        : QString(QStringLiteral("添加文件 (%1 个待发送)")).arg(files.size());
    m_fileButton->setToolTip(tooltip);
}
```

- [ ] **Step 4: 实现 clearFileListDisplay 和 onRemoveFileClicked 方法**

在 `updateFileListDisplay()` 后添加：

```cpp
void MainWindow::clearFileListDisplay()
{
    // 删除所有文件标签 widget（保留 stretch）
    while (m_fileListLayout->count() > 1) {
        QLayoutItem *item = m_fileListLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void MainWindow::onRemoveFileClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) {
        return;
    }

    QString filePath = btn->property("filePath").toString();

    // 从 FileManager 中移除文件
    QVector<FileAttachment> files = m_fileManager->pendingFiles();
    QVector<FileAttachment> newFiles;
    for (const FileAttachment &file : files) {
        if (file.path != filePath) {
            newFiles.append(file);
        }
    }

    // 重建 pendingFiles（FileManager 没有 removeSingleFile 方法，需要清空再添加）
    m_fileManager->clearPendingFiles();
    for (const FileAttachment &file : newFiles) {
        m_fileManager->addFile(file.path);
    }

    updateFileListDisplay();
}
```

- [ ] **Step 5: 验证编译**

```bash
cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/build
cmake .. && make LocalAIAssistant -j4
```

Expected: 编译成功

- [ ] **Step 6: Commit**

```bash
git add src/ui/mainwindow.cpp
git commit -m "feat: implement file selection and file list display

- Add file button click handler
- Add file list display with type-colored tags
- Add remove file functionality

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 4: 修改发送逻辑携带文件附件

**Files:**
- Modify: `src/ui/mainwindow.cpp`

- [ ] **Step 1: 修改 onSendClicked 方法**

找到 `onSendClicked()` 方法（大约第 386-400 行），修改为：

```cpp
void MainWindow::onSendClicked()
{
    QString userInput = m_inputLine->text().trimmed();
    if (userInput.isEmpty() && m_fileManager->pendingFileCount() == 0) {
        return;  // 无输入且无文件时不发送
    }

    // 如果有文件，显示提示
    if (m_fileManager->pendingFileCount() > 0) {
        QString locale = TranslationManager::instance()->currentLocale();
        QString tip = (locale == "en")
            ? QString("Sending with %1 file(s)").arg(m_fileManager->pendingFileCount())
            : QString(QStringLiteral("发送消息时携带 %1 个文件")).arg(m_fileManager->pendingFileCount());
        qDebug() << tip;
    }

    // 创建消息并添加附件
    ChatMessage userMsg("user", userInput);
    if (m_fileManager->pendingFileCount() > 0) {
        userMsg.attachments = m_fileManager->pendingFiles();
    }

    SessionManager::instance()->addMessageToCurrentSession("user", userInput);
    m_inputLine->clear();

    // 构建带附件的消息列表发送
    QVector<ChatMessage> messages = SessionManager::instance()->currentSession().messages;
    if (!userMsg.attachments.isEmpty()) {
        // 修改最后一条消息添加附件
        if (!messages.isEmpty()) {
            messages.last().attachments = userMsg.attachments;
        }
        // 发送后清空待发送文件列表
        m_fileManager->clearPendingFiles();
        clearFileListDisplay();
        m_fileButton->setToolTip(tr("添加文件"));
    }

    m_isStreaming = true;
    m_streamingContent.clear();
    setInputEnabled(false);
    m_networkManager->sendChatRequestWithContext(messages);
}
```

- [ ] **Step 2: 验证编译**

```bash
cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/build
cmake .. && make LocalAIAssistant -j4
```

Expected: 编译成功

- [ ] **Step 3: Commit**

```bash
git add src/ui/mainwindow.cpp
git commit -m "feat: attach files to message on send in GUI

- Include pending files in ChatMessage
- Clear file list after sending

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 5: 更新 retranslateUi 添加文件按钮翻译

**Files:**
- Modify: `src/ui/mainwindow.cpp`

- [ ] **Step 1: 在 retranslateUi 中添加文件按钮文本**

找到 `retranslateUi()` 方法（大约第 259-267 行），添加文件按钮更新：

```cpp
void MainWindow::retranslateUi()
{
    m_sendButton->setText(tr("发送"));
    m_newChatButton->setText(tr("+ 新建对话"));
    m_deleteAction->setText(tr("删除该对话"));
    setWindowTitle(tr("本地AI助手"));

    // 文件按钮
    m_fileButton->setText(tr("📁"));
    m_fileButton->setToolTip(tr("添加文件"));

    updateSessionList();
}
```

- [ ] **Step 2: 验证编译**

```bash
cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/build
cmake .. && make LocalAIAssistant -j4
```

Expected: 编译成功

- [ ] **Step 3: Commit**

```bash
git add src/ui/mainwindow.cpp
git commit -m "feat: add file button translation in retranslateUi

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>"
```

---

### Task 6: 集成测试

- [ ] **Step 1: 编译完整项目**

```bash
cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant/build
cmake .. && make -j4
```

Expected: 全部编译成功

- [ ] **Step 2: 启动 GUI 测试基本功能**

```bash
open build/LocalAIAssistant.app
```

手动测试：
1. 检查文件按钮是否在输入框右侧
2. 点击文件按钮，选择文件
3. 检查文件列表是否显示在输入框上方
4. 点击 × 删除文件
5. 发送消息，检查文件是否被携带

---

### Task 7: 最终提交和推送

- [ ] **Step 1: 确认所有修改已提交**

```bash
git status
```

Expected: 无未提交的修改

- [ ] **Step 2: 推送到 GitHub**

```bash
git push
```

---

## Spec 自检

| Spec 要求 | Task |
|-----------|------|
| FileManager 成员 | Task 1, 2 |
| 文件按钮（输入框右侧） | Task 2 |
| 文件列表区域（输入框上方） | Task 2 |
| 文件选择对话框 | Task 3 |
| 文件标签显示（颜色区分） | Task 3 |
| 点击 × 删除文件 | Task 3 |
| 发送消息携带文件 | Task 4 |
| 发送后清空文件列表 | Task 4 |
| 国际化翻译 | Task 5 |