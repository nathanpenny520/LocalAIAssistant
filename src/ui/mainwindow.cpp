#include "mainwindow.h"
#include "stylesheetmanager.h"
#include "translationmanager.h"
#include "markdownrenderer.h"
#include "filemanager.h"
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
#include <QFileDialog>
#include <QMessageBox>

// Parse thinking content from AI response
// Returns a map with "thinking" and "response" keys
QMap<QString, QString> MainWindow::parseThinkingContent(const QString &content)
{
    QMap<QString, QString> result;
    result["thinking"] = "";
    result["response"] = content;

    // Match <think>...</think> or ৻... pracu tags (common thinking tags)
    QRegularExpression thinkingRegex(R"(<think>(.*?)</think>|৻(.*?)pracu)",
                                      QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatch match = thinkingRegex.match(content);
    if (match.hasMatch()) {
        // Extract thinking content
        QString thinkingContent = match.captured(1).isEmpty() ? match.captured(2) : match.captured(1);
        thinkingContent = thinkingContent.trimmed();

        // Remove thinking tags from original content to get pure response
        QString pureResponse = content;
        pureResponse.remove(thinkingRegex);
        pureResponse = pureResponse.trimmed();

        result["thinking"] = thinkingContent;
        result["response"] = pureResponse;
    }

    return result;
}

QString MainWindow::formatMessageWithThinking(const QString &role, const QString &content)
{
    QString locale = TranslationManager::instance()->currentLocale();
    QString thinkingLabel = (locale == "en") ? "Thinking Process" : QStringLiteral("思考过程");
    QString userLabel = (locale == "en") ? "You" : QStringLiteral("用户");
    QString aiLabel = (locale == "en") ? "AI" : QStringLiteral("AI");

    // Get current theme for color-aware rendering
    StyleSheetManager::Theme theme = StyleSheetManager::instance()->currentTheme();
    bool isDarkTheme = (theme == StyleSheetManager::DarkTheme);
    // Also check if SystemTheme actually uses dark colors
    if (theme == StyleSheetManager::SystemTheme) {
        QPalette palette = QApplication::palette();
        QColor windowColor = palette.color(QPalette::Window);
        int brightness = (windowColor.red() * 299 + windowColor.green() * 587 + windowColor.blue() * 114) / 1000;
        isDarkTheme = (brightness < 128);
    }
    MarkdownColors colors = MarkdownRenderer::getColors(isDarkTheme);

    if (role == "user") {
        // User messages are simple text, no need for full markdown rendering
        // Just escape HTML special characters
        QString escapedContent = content;
        escapedContent.replace("&", "&amp;");
        escapedContent.replace("<", "&lt;");
        escapedContent.replace(">", "&gt;");
        return QString("<div style='margin: 12px 0; color: %1;'><b style='color: #007aff;'>%2:</b> %3</div>")
               .arg(colors.text, userLabel, escapedContent);
    }

    // Parse thinking content for AI messages
    QMap<QString, QString> parsed = parseThinkingContent(content);
    QString thinking = parsed["thinking"];
    QString response = parsed["response"];

    QString html;

    // If there's thinking content, show it in a styled blockquote
    if (!thinking.isEmpty()) {
        // Escape thinking content for HTML display
        QString escapedThinking = thinking;
        escapedThinking.replace("&", "&amp;");
        escapedThinking.replace("<", "&lt;");
        escapedThinking.replace(">", "&gt;");
        escapedThinking.replace("\n", "<br>");

        html += QString(
            "<blockquote style='background-color: %1; border-left: 4px solid %2; "
            "padding: 10px 14px; margin: 12px 0; color: %3; font-size: 13px;'>"
            "<b>%4</b><br><br>%5</blockquote>"
        ).arg(colors.quoteBg, colors.quoteBorder, colors.quoteText, thinkingLabel, escapedThinking);
    }

    // Add the AI label
    html += QString("<div style='margin: 12px 0; color: %1;'><b style='color: #007aff;'>%2:</b></div>")
            .arg(colors.text, aiLabel);

    // Add the actual response with full markdown rendering
    if (!response.isEmpty()) {
        html += MarkdownRenderer::toHtml(response, isDarkTheme);
    } else if (thinking.isEmpty()) {
        // No thinking content found, show original content
        html += MarkdownRenderer::toHtml(content, isDarkTheme);
    }

    return html;
}

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
    , m_fileManager(new FileManager(this))
    , m_fileButton(new QPushButton(tr("📁"), this))
    , m_fileListArea(nullptr)
    , m_fileListLayout(nullptr)
{
    setupUI();
    setupMenuBar();

    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_fileButton, &QPushButton::clicked, this, &MainWindow::onFileButtonClicked);
    connect(m_inputLine, &QLineEdit::returnPressed, this, &MainWindow::onSendClicked);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::onSettingsClicked);
    connect(m_newChatButton, &QPushButton::clicked, this, &MainWindow::onNewChatClicked);
    connect(m_historyList, &QListWidget::itemClicked, this, &MainWindow::onSessionItemClicked);
    connect(m_deleteAction, &QAction::triggered, this, &MainWindow::onDeleteSession);
    connect(m_historyList, &QWidget::customContextMenuRequested, this, &MainWindow::onCustomContextMenuRequested);

    connect(m_networkManager, &NetworkManager::responseReceived, this, &MainWindow::onNetworkFinished);
    connect(m_networkManager, &NetworkManager::streamChunkReceived, this, &MainWindow::onStreamChunkReceived);
    connect(m_networkManager, &NetworkManager::streamFinished, this, &MainWindow::onStreamFinished);
    connect(m_networkManager, &NetworkManager::errorOccurred, this, &MainWindow::onNetworkError);
    connect(SessionManager::instance(), &SessionManager::sessionChanged, this, &MainWindow::renderCurrentSession);
    connect(StyleSheetManager::instance(), &StyleSheetManager::themeChanged, this, &MainWindow::onThemeChanged);
    connect(TranslationManager::instance(), &TranslationManager::languageChanged, this, &MainWindow::onLanguageChanged);

    // Load saved sessions from disk
    SessionManager::instance()->loadSessionsFromFile();
    updateSessionList();
    renderCurrentSession();

    StyleSheetManager::instance()->applyTheme(this);
}

MainWindow::~MainWindow()
{
    delete m_markdownDoc;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    SessionManager::instance()->saveSessionsToFile();
    event->accept();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
        setupMenuBar();
        renderCurrentSession();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    QWidget *leftPanel = new QWidget(splitter);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(5, 5, 5, 5);
    leftLayout->addWidget(m_newChatButton);
    leftLayout->addWidget(m_historyList);

    QWidget *rightPanel = new QWidget(splitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(5, 5, 5, 5);
    rightLayout->addWidget(m_chatDisplay, 1);

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

    splitter->setSizes({180, 600});

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(splitter);

    m_contextMenu->addAction(m_deleteAction);
    m_historyList->setContextMenuPolicy(Qt::CustomContextMenu);

    resize(900, 600);
    setWindowTitle(tr("本地AI助手"));

    updateSessionList();
}

void MainWindow::setupMenuBar()
{
    if (menuBar()) {
        menuBar()->clear();
    }

    QString locale = TranslationManager::instance()->currentLocale();

    QMenuBar *bar = menuBar() ? menuBar() : new QMenuBar(this);

#ifdef Q_OS_MACOS
    QString appName = (locale == "en") ? "LocalAI Assistant" : QStringLiteral("本地AI助手");
    QString prefText = (locale == "en") ? "Preferences..." : QStringLiteral("偏好设置...");
    QString quitText = (locale == "en") ? "Quit LocalAI Assistant" : QStringLiteral("退出 本地AI助手");

    bar->setNativeMenuBar(true);

    QMenu *appMenu = bar->addMenu(appName);
    QAction *prefAction = appMenu->addAction(prefText);
    prefAction->setMenuRole(QAction::PreferencesRole);
    prefAction->setShortcut(QKeySequence::StandardKey::Preferences);
    connect(prefAction, &QAction::triggered, this, &MainWindow::onSettingsClicked);

    appMenu->addSeparator();

    QAction *quitAction = appMenu->addAction(quitText);
    quitAction->setMenuRole(QAction::QuitRole);
    quitAction->setShortcut(QKeySequence::StandardKey::Quit);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    QMenu *fileMenu = bar->addMenu((locale == "en") ? "&File" : QStringLiteral("文件"));
    fileMenu->menuAction()->setText((locale == "en") ? "File" : QStringLiteral("文件"));
#else
    QString settingsText = (locale == "en") ? "Settings" : QStringLiteral("设置");
    m_settingsAction->setText(settingsText);
    QMenu *settingsMenu = bar->addMenu(settingsText);
    settingsMenu->addAction(m_settingsAction);
#endif

    if (!menuBar()) {
        setMenuBar(bar);
    }
}

void MainWindow::retranslateUi()
{
    m_sendButton->setText(tr("发送"));
    m_newChatButton->setText(tr("+ 新建对话"));
    m_deleteAction->setText(tr("删除该对话"));
    setWindowTitle(tr("本地AI助手"));

    updateSessionList();
}

void MainWindow::appendChatMessage(const QString &sender, const QString &message)
{
    QString role = (sender == tr("用户")) ? "user" : "assistant";
    QString formatted = formatMessageWithThinking(role, message);

    // Get current HTML content
    QString currentHtml = m_chatDisplay->toHtml();

    // Find body content position
    int bodyStart = currentHtml.indexOf("<body>");
    int bodyEnd = currentHtml.indexOf("</body>");

    if (bodyStart != -1 && bodyEnd != -1) {
        QString bodyContent = currentHtml.mid(bodyStart + 6, bodyEnd - bodyStart - 6);
        // Dynamic separator color based on actual theme (including SystemTheme)
        StyleSheetManager::Theme theme = StyleSheetManager::instance()->currentTheme();
        bool isDarkTheme = (theme == StyleSheetManager::DarkTheme);
        if (theme == StyleSheetManager::SystemTheme) {
            QPalette palette = QApplication::palette();
            QColor windowColor = palette.color(QPalette::Window);
            int brightness = (windowColor.red() * 299 + windowColor.green() * 587 + windowColor.blue() * 114) / 1000;
            isDarkTheme = (brightness < 128);
        }
        MarkdownColors colors = MarkdownRenderer::getColors(isDarkTheme);
        QString separator = QString("<hr style='border: none; border-top: 1px solid %1; margin: 16px 0;'>").arg(colors.tableBorder);
        QString newHtml = currentHtml.left(bodyStart + 6) + bodyContent + separator + formatted + currentHtml.mid(bodyEnd);
        m_chatDisplay->setHtml(newHtml);
    } else {
        m_chatDisplay->setHtml(formatted);
    }

    QScrollBar *scrollBar = m_chatDisplay->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::renderCurrentSession()
{
    qDebug() << "renderCurrentSession called, m_isRendering=" << m_isRendering;

    if (m_isRendering) {
        qDebug() << "renderCurrentSession blocked by m_isRendering";
        return;
    }
    m_isRendering = true;

    const auto &session = SessionManager::instance()->currentSession();
    qDebug() << "Session has" << session.messages.size() << "messages";

    QString fullHtml;

    for (int i = 0; i < session.messages.size(); ++i) {
        const auto &msg = session.messages[i];
        QString rendered = formatMessageWithThinking(msg.role, msg.content);
        qDebug() << "Message" << i << "role:" << msg.role << "rendered length:" << rendered.length();
        fullHtml += rendered;

        if (i < session.messages.size() - 1) {
            // Dynamic separator color based on actual theme (including SystemTheme)
            StyleSheetManager::Theme theme = StyleSheetManager::instance()->currentTheme();
            bool isDarkTheme = (theme == StyleSheetManager::DarkTheme);
            if (theme == StyleSheetManager::SystemTheme) {
                QPalette palette = QApplication::palette();
                QColor windowColor = palette.color(QPalette::Window);
                int brightness = (windowColor.red() * 299 + windowColor.green() * 587 + windowColor.blue() * 114) / 1000;
                isDarkTheme = (brightness < 128);
            }
            MarkdownColors colors = MarkdownRenderer::getColors(isDarkTheme);
            QString separator = QString("<hr style='border: none; border-top: 1px solid %1; margin: 16px 0;'>").arg(colors.tableBorder);
            fullHtml += separator;
        }
    }

    qDebug() << "Final HTML length:" << fullHtml.length();
    qDebug() << "HTML preview (first 200 chars):" << fullHtml.left(200);

    m_chatDisplay->setHtml(fullHtml);

    QScrollBar *scrollBar = m_chatDisplay->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());

    m_isRendering = false;
}

void MainWindow::updateSessionList()
{
    m_sessionItemMap.clear();
    m_historyList->clear();

    const auto &sessions = SessionManager::instance()->allSessions();
    for (const auto &session : sessions) {
        QString displayTitle = session.title.isEmpty() ? tr("新对话") : session.title;
        QListWidgetItem *item = new QListWidgetItem(displayTitle, m_historyList);
        item->setData(Qt::UserRole, session.id);
        m_historyList->addItem(item);
        m_sessionItemMap[session.id] = item;

        if (session.id == SessionManager::instance()->currentSessionId()) {
            m_historyList->setCurrentItem(item);
            m_historyList->scrollToItem(item);
        }
    }
}

void MainWindow::setInputEnabled(bool enabled)
{
    m_sendButton->setEnabled(enabled);
    m_inputLine->setEnabled(enabled);

    if (enabled) {
        m_inputLine->setPlaceholderText("");
        m_sendButton->setText(tr("发送"));
    } else {
        m_inputLine->setPlaceholderText(tr("正在思考..."));
        m_sendButton->setText(tr("思考中..."));
    }
}

void MainWindow::onSendClicked()
{
    QString userInput = m_inputLine->text().trimmed();
    if (userInput.isEmpty()) {
        return;
    }

    SessionManager::instance()->addMessageToCurrentSession("user", userInput);
    m_inputLine->clear();

    m_isStreaming = true;
    m_streamingContent.clear();
    setInputEnabled(false);
    m_networkManager->sendChatRequestWithContext(SessionManager::instance()->currentSession().messages);
}

void MainWindow::onNetworkFinished(const QString &response)
{
    m_isStreaming = false;
    m_streamingContent.clear();
    SessionManager::instance()->addMessageToCurrentSession("assistant", response);
    setInputEnabled(true);
}

void MainWindow::onNetworkError(const QString &error)
{
    m_isStreaming = false;
    m_streamingContent.clear();
    SessionManager::instance()->addMessageToCurrentSession("assistant", tr("错误: ") + error);
    setInputEnabled(true);
}

void MainWindow::onStreamChunkReceived(const QString &chunk)
{
    if (!m_isStreaming) {
        return;
    }

    m_streamingContent += chunk;

    // During streaming, just append text to show progress (minimal updates)
    QTextCursor cursor = m_chatDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);

    // First chunk - add AI label
    if (m_streamingContent == chunk) {
        QString locale = TranslationManager::instance()->currentLocale();
        QString aiLabel = (locale == "en") ? "AI" : QStringLiteral("AI");
        cursor.insertHtml(QString("<p style='margin:12px 0;'><b style='color:#007aff;'>%1:</b></p>").arg(aiLabel));
    }

    // Append chunk text (raw, will be re-rendered on completion)
    cursor.insertText(chunk);

    // Force scroll to bottom (ensureCursorVisible may not work reliably with insertHtml)
    QScrollBar *scrollBar = m_chatDisplay->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::onStreamFinished(const QString &fullContent)
{
    if (!m_isStreaming) {
        return;
    }

    m_isStreaming = false;

    // Save the complete message
    SessionManager::instance()->addMessageToCurrentSession("assistant", fullContent);
    SessionManager::instance()->saveSessionsToFile();

    // Clear streaming buffer
    m_streamingContent.clear();

    // Force render (reset m_isRendering to allow it)
    m_isRendering = false;
    renderCurrentSession();

    setInputEnabled(true);
}

void MainWindow::onSettingsClicked()
{
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        m_networkManager->updateSettings(
            dialog.getApiBaseUrl(),
            dialog.getApiKey(),
            dialog.getModelName(),
            dialog.isLocalMode()
        );
        m_networkManager->setStreamingEnabled(dialog.isStreamingEnabled());
    }
}

void MainWindow::onNewChatClicked()
{
    SessionManager::instance()->createNewSession();
    m_chatDisplay->clear();
    m_markdownDoc->clear();
    m_inputLine->clear();
    m_inputLine->setFocus();
    updateSessionList();
}

void MainWindow::onSessionItemClicked(QListWidgetItem *item)
{
    if (!item) {
        return;
    }

    QString sessionId = item->data(Qt::UserRole).toString();
    SessionManager::instance()->switchToSession(sessionId);
}

void MainWindow::onDeleteSession()
{
    QListWidgetItem *item = m_historyList->currentItem();
    if (!item) {
        return;
    }

    QString sessionId = item->data(Qt::UserRole).toString();
    SessionManager::instance()->removeSession(sessionId);
    delete item;
    m_sessionItemMap.remove(sessionId);
}

void MainWindow::onCustomContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem *item = m_historyList->itemAt(pos);
    if (item) {
        m_contextMenu->exec(m_historyList->mapToGlobal(pos));
    }
}

void MainWindow::onThemeChanged(int theme)
{
    Q_UNUSED(theme);
    this->setStyleSheet(StyleSheetManager::instance()->currentStyleSheet());
    // Re-render chat content with new theme colors
    renderCurrentSession();
}

void MainWindow::onLanguageChanged()
{
    retranslateUi();
    setupMenuBar();
    renderCurrentSession();
}

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

        // Determine theme-appropriate colors
        StyleSheetManager::Theme theme = StyleSheetManager::instance()->currentTheme();
        bool isDarkTheme = (theme == StyleSheetManager::DarkTheme);
        if (theme == StyleSheetManager::SystemTheme) {
            QPalette palette = QApplication::palette();
            QColor windowColor = palette.color(QPalette::Window);
            int brightness = (windowColor.red() * 299 + windowColor.green() * 587 + windowColor.blue() * 114) / 1000;
            isDarkTheme = (brightness < 128);
        }

        QString textColor = isDarkTheme ? "#e0e0e0" : "#333";
        QString bgColor = isDarkTheme ? "#3a3a3a" : "#f5f5f5";

        nameLabel->setStyleSheet(QString(
            "QLabel { color: %1; font-size: 12px; padding: 2px 6px; "
            "border: 1px solid %2; border-radius: 4px; background: %3; }"
        ).arg(textColor, borderColor, bgColor));
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
