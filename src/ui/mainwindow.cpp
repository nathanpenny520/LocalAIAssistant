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
    , m_toggleHistoryAction(new QAction(tr("显示历史面板"), this))
    , m_contextMenu(new QMenu(this))
    , m_deleteAction(new QAction(tr("删除该对话"), this))
    , m_networkManager(new NetworkManager(this))
    , m_markdownDoc(new QTextDocument(this))
    , m_splitter(nullptr)
    , m_leftPanel(nullptr)
    , m_isStreaming(false)
    , m_fileManager(new FileManager(this))
    , m_fileButton(new QPushButton(this))
    , m_fileListArea(nullptr)
    , m_fileListLayout(nullptr)
    , m_searchBar(nullptr)
    , m_searchInput(nullptr)
    , m_searchPrevBtn(nullptr)
    , m_searchNextBtn(nullptr)
    , m_searchCloseBtn(nullptr)
    , m_searchResultLabel(nullptr)
    , m_searchAction(new QAction(tr("搜索"), this))
    , m_girlfriendAction(new QAction(tr("AI女友"), this))
    , m_currentMatchIndex(0)
    , m_totalMatches(0)
{
    setupUI();
    setupMenuBar();

    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_fileButton, &QPushButton::clicked, this, &MainWindow::onFileButtonClicked);
    connect(m_inputLine, &QLineEdit::returnPressed, this, &MainWindow::onSendClicked);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::onSettingsClicked);
    connect(m_toggleHistoryAction, &QAction::triggered, this, &MainWindow::onToggleHistoryPanel);
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

    // 搜索功能连接
    m_searchAction->setShortcut(QKeySequence::Find);  // Ctrl+F / Cmd+F
    connect(m_searchAction, &QAction::triggered, this, &MainWindow::onSearchTriggered);

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

    m_splitter = new QSplitter(Qt::Horizontal, this);

    m_leftPanel = new QWidget(m_splitter);
    QVBoxLayout *leftLayout = new QVBoxLayout(m_leftPanel);
    leftLayout->setContentsMargins(5, 5, 5, 5);
    leftLayout->addWidget(m_newChatButton);
    leftLayout->addWidget(m_historyList);

    // 设置左侧面板最小宽度，防止完全关闭
    m_leftPanel->setMinimumWidth(120);

    QWidget *rightPanel = new QWidget(m_splitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(5, 5, 5, 5);

    // 搜索栏（默认隐藏，Ctrl+F时显示）
    setupSearchBar();
    rightLayout->addWidget(m_searchBar);

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

    // 设置文件按钮样式 - 使用 Qt 标准图标
    QIcon fileIcon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
    m_fileButton->setIcon(fileIcon);
    m_fileButton->setIconSize(QSize(20, 20));
    m_fileButton->setFixedSize(40, 30);
    m_fileButton->setToolTip(tr("添加文件"));

    m_splitter->setSizes({180, 600});

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_splitter);

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
    // Windows/Linux: 标准 File 菜单
    QString fileText = (locale == "en") ? "&File" : QStringLiteral("文件");
    QMenu *fileMenu = bar->addMenu(fileText);

    QString settingsText = (locale == "en") ? "Settings..." : QStringLiteral("设置...");
    m_settingsAction->setText(settingsText);
    fileMenu->addAction(m_settingsAction);

    fileMenu->addSeparator();

    QString exitText = (locale == "en") ? "E&xit" : QStringLiteral("退出");
    QAction *exitAction = fileMenu->addAction(exitText);
    exitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
#endif

    // 视图菜单 - 显示/隐藏历史面板
    QString viewText = (locale == "en") ? "View" : QStringLiteral("视图");
    QMenu *viewMenu = bar->addMenu(viewText);

    QString toggleHistoryText = (locale == "en") ? "Show History Panel" : QStringLiteral("显示历史面板");
    m_toggleHistoryAction->setText(toggleHistoryText);
    m_toggleHistoryAction->setCheckable(true);
    m_toggleHistoryAction->setChecked(m_leftPanel && m_leftPanel->width() > 0);
    m_toggleHistoryAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_H));
    viewMenu->addAction(m_toggleHistoryAction);

    // 编辑菜单 - 搜索功能
    QString editText = (locale == "en") ? "Edit" : QStringLiteral("编辑");
    QMenu *editMenu = bar->addMenu(editText);

    QString searchText = (locale == "en") ? "Find..." : QStringLiteral("查找...");
    m_searchAction->setText(searchText);
    editMenu->addAction(m_searchAction);

    // AI女友菜单入口
    QString girlfriendText = (locale == "en") ? "AI Girlfriend" : QStringLiteral("AI女友");
    m_girlfriendAction->setText(girlfriendText);
    m_girlfriendAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
    viewMenu->addAction(m_girlfriendAction);
    connect(m_girlfriendAction, &QAction::triggered, this, &MainWindow::onGirlfriendClicked);

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

    // 文件按钮 tooltip
    m_fileButton->setToolTip(tr("添加文件"));

    // 更新显示历史面板菜单项文本
    QString locale = TranslationManager::instance()->currentLocale();
    QString toggleHistoryText = (locale == "en") ? "Show History Panel" : QStringLiteral("显示历史面板");
    m_toggleHistoryAction->setText(toggleHistoryText);

    // 更新搜索相关文本
    QString searchText = (locale == "en") ? "Find..." : QStringLiteral("查找...");
    m_searchAction->setText(searchText);
    if (m_searchInput) {
        m_searchInput->setPlaceholderText(tr("搜索历史消息..."));
    }
    if (m_searchPrevBtn) {
        m_searchPrevBtn->setToolTip(tr("上一个"));
    }
    if (m_searchNextBtn) {
        m_searchNextBtn->setToolTip(tr("下一个"));
    }
    if (m_searchCloseBtn) {
        m_searchCloseBtn->setToolTip(tr("关闭"));
    }

    // AI女友菜单项文本
    QString girlfriendText = (locale == "en") ? "AI Girlfriend" : QStringLiteral("AI女友");
    m_girlfriendAction->setText(girlfriendText);

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
    if (m_isRendering) {
        return;
    }
    m_isRendering = true;

    const auto &session = SessionManager::instance()->currentSession();

    QString fullHtml;

    for (int i = 0; i < session.messages.size(); ++i) {
        const auto &msg = session.messages[i];
        QString rendered = formatMessageWithThinking(msg.role, msg.content);

        // 渲染附件信息（仅用户消息）
        if (msg.role == "user" && !msg.attachments.isEmpty()) {
            QString attachmentHtml;
            for (const auto &attachment : msg.attachments) {
                if (attachment.type == "image") {
                    // 图片：显示缩略图
                    attachmentHtml += QString(
                        "<div style='margin:8px 0;'>"
                        "<img src='%1' style='max-width:300px; max-height:200px; border-radius:8px; border:1px solid #ccc;' />"
                        "</div>"
                    ).arg(attachment.content);
                } else {
                    // 其他文件：显示文件名和类型
                    QString iconColor = (attachment.type == "text") ? "#4CAF50" : "#FF9800";
                    QString typeLabel = (attachment.type == "text") ? "文本" : "二进制";
                    QFileInfo info(attachment.path);
                    attachmentHtml += QString(
                        "<div style='margin:8px 0; padding:8px 12px; background:#f5f5f5; border-radius:6px; display:inline-block;'>"
                        "<span style='color:%1; font-weight:bold;'>[%2]</span> %3 (%4 KB)"
                        "</div>"
                    ).arg(iconColor).arg(typeLabel).arg(info.fileName()).arg(attachment.size / 1024);
                }
            }
            // 用户消息格式: <div ...><b>用户:</b> 内容</div>
            // 在 </div> 之前插入附件
            rendered = rendered.replace("</div>", attachmentHtml + "</div>");
        }

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
    QVector<FileAttachment> attachments;
    if (m_fileManager->pendingFileCount() > 0) {
        attachments = m_fileManager->pendingFiles();
    }

    // 使用带附件参数的函数保存消息
    if (attachments.isEmpty()) {
        SessionManager::instance()->addMessageToCurrentSession("user", userInput);
    } else {
        SessionManager::instance()->addMessageToCurrentSession("user", userInput, attachments);
        m_fileManager->clearPendingFiles();
        clearFileListDisplay();
        m_fileButton->setToolTip(tr("添加文件"));
    }

    m_inputLine->clear();

    // 构建消息列表发送
    QVector<ChatMessage> messages = SessionManager::instance()->currentSession().messages;

    m_isStreaming = true;
    m_streamingContent.clear();
    m_requestSessionId = SessionManager::instance()->currentSessionId();  // 记录发起请求时的会话ID
    setInputEnabled(false);
    m_networkManager->sendChatRequestWithContext(messages);
}

void MainWindow::onNetworkFinished(const QString &response)
{
    // 验证响应是否属于发起请求时的会话
    if (m_requestSessionId.isEmpty()) {
        return;
    }

    m_isStreaming = false;
    m_streamingContent.clear();

    // 直接添加消息到原会话
    SessionManager::instance()->addMessageToSession(m_requestSessionId, "assistant", response);

    // 只有当前显示的是原会话才渲染
    if (m_requestSessionId == SessionManager::instance()->currentSessionId()) {
        m_isRendering = false;
        renderCurrentSession();
    }

    m_requestSessionId.clear();
    setInputEnabled(true);
}

void MainWindow::onNetworkError(const QString &error)
{
    // 验证响应是否属于发起请求时的会话
    if (m_requestSessionId.isEmpty()) {
        return;
    }

    m_isStreaming = false;
    m_streamingContent.clear();

    // 直接添加错误消息到原会话
    SessionManager::instance()->addMessageToSession(m_requestSessionId, "assistant", tr("错误: ") + error);

    // 只有当前显示的是原会话才渲染
    if (m_requestSessionId == SessionManager::instance()->currentSessionId()) {
        m_isRendering = false;
        renderCurrentSession();
    }

    m_requestSessionId.clear();
    setInputEnabled(true);
}

void MainWindow::onStreamChunkReceived(const QString &chunk)
{
    // 验证响应是否属于当前显示的会话
    if (!m_isStreaming || m_requestSessionId != SessionManager::instance()->currentSessionId()) {
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
    // 验证响应是否属于发起请求时的会话
    if (!m_isStreaming || m_requestSessionId.isEmpty()) {
        return;
    }

    m_isStreaming = false;

    // 直接添加消息到原会话，不需要切换
    SessionManager::instance()->addMessageToSession(m_requestSessionId, "assistant", fullContent);

    // 自动命名：如果是新对话的第一条回复，根据用户第一条消息生成标题
    const auto &sessions = SessionManager::instance()->allSessions();
    if (sessions.contains(m_requestSessionId)) {
        const auto &session = sessions[m_requestSessionId];
        if (session.title == "新对话" && session.messages.size() == 2) {
            QString firstUserMsg;
            for (const auto &msg : session.messages) {
                if (msg.role == "user") {
                    firstUserMsg = msg.content;
                    break;
                }
            }
            if (!firstUserMsg.isEmpty()) {
                QString title = firstUserMsg.split('\n')[0].left(30);
                if (title.length() < firstUserMsg.split('\n')[0].length()) {
                    title += "...";
                }
                SessionManager::instance()->updateSessionTitle(m_requestSessionId, title);
                updateSessionList();
            }
        }
    }

    SessionManager::instance()->saveSessionsToFile();

    // 只有当前显示的是原会话才渲染
    if (m_requestSessionId == SessionManager::instance()->currentSessionId()) {
        m_isRendering = false;
        renderCurrentSession();
    }

    m_streamingContent.clear();
    m_requestSessionId.clear();
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
    QString defaultTitle = TranslationManager::instance()->currentLocale() == "en"
        ? "New Chat" : tr("新对话");
    SessionManager::instance()->createNewSession(defaultTitle);
    m_chatDisplay->clear();
    m_markdownDoc->clear();
    m_inputLine->clear();
    m_inputLine->setFocus();
    updateSessionList();
}

void MainWindow::onToggleHistoryPanel()
{
    if (!m_splitter || !m_leftPanel) {
        return;
    }

    // 如果面板当前隐藏（宽度为最小宽度），则恢复到正常宽度
    if (m_leftPanel->width() <= m_leftPanel->minimumWidth()) {
        m_splitter->setSizes({180, m_splitter->width() - 180});
        m_toggleHistoryAction->setChecked(true);
    } else {
        // 隐藏面板（设置为最小宽度）
        m_splitter->setSizes({0, m_splitter->width()});
        m_toggleHistoryAction->setChecked(false);
    }
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
    // Clear search highlights first to avoid display issues
    if (m_searchBar && m_searchBar->isVisible()) {
        clearHighlights();
    }

    this->setStyleSheet(StyleSheetManager::instance()->currentStyleSheet());
    // Re-render chat content with new theme colors
    renderCurrentSession();

    // Re-apply highlights with new theme colors if there's a search term
    // This must be done after renderCurrentSession() completes
    if (m_searchBar && m_searchBar->isVisible() && m_searchInput) {
        QString keyword = m_searchInput->text().trimmed();
        if (!keyword.isEmpty()) {
            // Recalculate match count after re-render
            QTextDocument *doc = m_chatDisplay->document();
            QTextCursor cursor(doc);
            m_totalMatches = 0;
            while (!cursor.isNull()) {
                cursor = doc->find(keyword, cursor);
                if (!cursor.isNull()) {
                    m_totalMatches++;
                }
            }
            // Reset to first match
            m_currentMatchIndex = 1;
            highlightAllMatches();
            onSearchNext();
            updateSearchResultLabel();
        }
    }
}

void MainWindow::onLanguageChanged()
{
    retranslateUi();
    setupMenuBar();
    renderCurrentSession();
}

void MainWindow::onFileButtonClicked()
{
    // macOS 原生文件对话框的语言由系统语言偏好顺序决定
    // 无法通过 Qt 应用代码直接控制
    // 如需英文对话框，请在 macOS 系统设置中将英文设为首选语言

    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        QString(),
        QString(),
        QString()
    );

    if (filePaths.isEmpty()) {
        return;
    }

    QString locale = TranslationManager::instance()->currentLocale();
    QString errorTitle = (locale == "en") ? "Error" : QStringLiteral("错误");

    for (const QString &path : filePaths) {
        QFileInfo info(path);
        if (!info.exists()) {
            QString msg = (locale == "en")
                ? QString("File does not exist: %1").arg(path)
                : QString(QStringLiteral("文件不存在: %1")).arg(path);
            QMessageBox::warning(this, errorTitle, msg);
            continue;
        }

        if (info.size() > 10 * 1024 * 1024) {
            QString msg = (locale == "en")
                ? QString("File too large (>10MB): %1").arg(path)
                : QString(QStringLiteral("文件过大 (>10MB): %1")).arg(path);
            QMessageBox::warning(this, errorTitle, msg);
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

        // 删除按钮 - 使用主题适配的关闭图标
        QPushButton *removeBtn = new QPushButton(fileTag);
        // SP_TitleBarCloseButton 通常有更好的颜色适配
        QIcon closeIcon = QApplication::style()->standardIcon(QStyle::SP_TitleBarCloseButton);
        removeBtn->setIcon(closeIcon);
        removeBtn->setIconSize(QSize(16, 16));
        removeBtn->setFixedSize(24, 24);

        // 根据主题设置按钮样式
        QString removeBtnStyle;
        if (isDarkTheme) {
            removeBtnStyle = "QPushButton { border: none; background: transparent; padding: 2px; }"
                             "QPushButton:hover { background: rgba(255, 59, 48, 0.2); border-radius: 12px; }";
        } else {
            removeBtnStyle = "QPushButton { border: none; background: transparent; padding: 2px; }"
                             "QPushButton:hover { background: #ffebeb; border-radius: 12px; }";
        }
        removeBtn->setStyleSheet(removeBtnStyle);
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

// ==================== 搜索功能实现 ====================

void MainWindow::setupSearchBar()
{
    m_searchBar = new QFrame(this);
    m_searchBar->setObjectName("searchBar");
    m_searchBar->setVisible(false);  // 默认隐藏

    QHBoxLayout *searchLayout = new QHBoxLayout(m_searchBar);
    searchLayout->setContentsMargins(10, 6, 10, 6);
    searchLayout->setSpacing(8);

    // 搜索输入框
    m_searchInput = new QLineEdit(m_searchBar);
    m_searchInput->setObjectName("searchInput");
    m_searchInput->setPlaceholderText(tr("搜索历史消息..."));
    m_searchInput->setClearButtonEnabled(true);
    m_searchInput->setMinimumHeight(32);
    searchLayout->addWidget(m_searchInput, 1);

    // 搜索结果计数
    m_searchResultLabel = new QLabel(m_searchBar);
    m_searchResultLabel->setObjectName("searchResultLabel");
    m_searchResultLabel->setText("");
    m_searchResultLabel->setMinimumWidth(70);
    searchLayout->addWidget(m_searchResultLabel);

    // 上一个按钮
    m_searchPrevBtn = new QPushButton(m_searchBar);
    m_searchPrevBtn->setObjectName("searchPrevBtn");
    m_searchPrevBtn->setText("◀");
    m_searchPrevBtn->setToolTip(tr("上一个"));
    searchLayout->addWidget(m_searchPrevBtn);

    // 下一个按钮
    m_searchNextBtn = new QPushButton(m_searchBar);
    m_searchNextBtn->setObjectName("searchNextBtn");
    m_searchNextBtn->setText("▶");
    m_searchNextBtn->setToolTip(tr("下一个"));
    searchLayout->addWidget(m_searchNextBtn);

    // 关闭按钮
    m_searchCloseBtn = new QPushButton(m_searchBar);
    m_searchCloseBtn->setObjectName("searchCloseBtn");
    m_searchCloseBtn->setText("✖");
    m_searchCloseBtn->setToolTip(tr("关闭"));
    searchLayout->addWidget(m_searchCloseBtn);

    // 连接信号
    connect(m_searchInput, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_searchPrevBtn, &QPushButton::clicked, this, &MainWindow::onSearchPrevious);
    connect(m_searchNextBtn, &QPushButton::clicked, this, &MainWindow::onSearchNext);
    connect(m_searchCloseBtn, &QPushButton::clicked, this, &MainWindow::onSearchClose);
}

void MainWindow::updateSearchBarStyle()
{
    // 样式由全局样式表管理，主题切换时自动更新
    // 此方法保留以备将来需要额外样式调整时使用
}

void MainWindow::onSearchTriggered()
{
    if (!m_searchBar) {
        setupSearchBar();
    }

    m_searchBar->setVisible(true);
    m_searchInput->clear();
    m_searchInput->setFocus();
    m_currentMatchIndex = 0;
    m_totalMatches = 0;
    m_searchResultLabel->setText("");
}

void MainWindow::onSearchTextChanged()
{
    QString keyword = m_searchInput->text().trimmed();

    if (keyword.isEmpty()) {
        clearHighlights();
        m_totalMatches = 0;
        m_currentMatchIndex = 0;
        m_searchResultLabel->setText("");
        return;
    }

    // 计算匹配数量
    QTextDocument *doc = m_chatDisplay->document();
    QTextCursor cursor(doc);
    m_totalMatches = 0;

    while (!cursor.isNull()) {
        cursor = doc->find(keyword, cursor);
        if (!cursor.isNull()) {
            m_totalMatches++;
        }
    }

    m_currentMatchIndex = 0;

    // 高亮第一个匹配
    highlightAllMatches();
    if (m_totalMatches > 0) {
        onSearchNext();
    }

    updateSearchResultLabel();
}

void MainWindow::onSearchNext()
{
    QString keyword = m_searchInput->text().trimmed();
    if (keyword.isEmpty()) {
        return;
    }

    QTextDocument *doc = m_chatDisplay->document();
    QTextCursor cursor = m_chatDisplay->textCursor();

    // 从当前位置向后搜索
    QTextCursor found = doc->find(keyword, cursor);

    if (found.isNull()) {
        // 没找到，从头开始搜索
        cursor.movePosition(QTextCursor::Start);
        found = doc->find(keyword, cursor);
    }

    if (!found.isNull()) {
        m_chatDisplay->setTextCursor(found);
        m_chatDisplay->ensureCursorVisible();
        // 更新当前索引和高亮
        updateCurrentMatchIndex();
        highlightAllMatches();  // 更新高亮显示
    }
}

void MainWindow::onSearchPrevious()
{
    QString keyword = m_searchInput->text().trimmed();
    if (keyword.isEmpty()) {
        return;
    }

    QTextDocument *doc = m_chatDisplay->document();
    QTextCursor cursor = m_chatDisplay->textCursor();

    // 从当前位置向前搜索
    QTextCursor found = doc->find(keyword, cursor, QTextDocument::FindBackward);

    if (found.isNull()) {
        // 没找到，从末尾开始搜索
        cursor.movePosition(QTextCursor::End);
        found = doc->find(keyword, cursor, QTextDocument::FindBackward);
    }

    if (!found.isNull()) {
        m_chatDisplay->setTextCursor(found);
        m_chatDisplay->ensureCursorVisible();
        updateCurrentMatchIndex();
        highlightAllMatches();  // 更新高亮显示
    }
}

void MainWindow::onSearchClose()
{
    clearHighlights();
    m_searchBar->setVisible(false);
    m_inputLine->setFocus();
}

void MainWindow::highlightAllMatches()
{
    QString keyword = m_searchInput->text().trimmed();
    if (keyword.isEmpty()) {
        return;
    }

    // 使用 extraSelection 实现高亮效果
    QList<QTextEdit::ExtraSelection> extraSelections;

    // 判断当前主题，选择合适的高亮颜色
    StyleSheetManager::Theme theme = StyleSheetManager::instance()->currentTheme();
    bool isDarkTheme = (theme == StyleSheetManager::DarkTheme);
    if (theme == StyleSheetManager::SystemTheme) {
        QPalette palette = QApplication::palette();
        QColor windowColor = palette.color(QPalette::Window);
        int brightness = (windowColor.red() * 299 + windowColor.green() * 587 + windowColor.blue() * 114) / 1000;
        isDarkTheme = (brightness < 128);
    }

    // 匹配项高亮颜色（黄色背景）
    QColor matchColor = isDarkTheme ? QColor(255, 200, 50, 150) : QColor(255, 235, 130);
    // 当前匹配高亮颜色（橙色背景，更醒目）
    QColor currentMatchColor = isDarkTheme ? QColor(255, 165, 0, 200) : QColor(255, 180, 60);

    QTextDocument *doc = m_chatDisplay->document();
    QTextCursor cursor(doc);
    int matchIndex = 0;

    // 找到所有匹配并添加高亮
    while (!cursor.isNull()) {
        cursor = doc->find(keyword, cursor);
        if (!cursor.isNull()) {
            QTextEdit::ExtraSelection selection;
            selection.cursor = cursor;
            selection.format.setBackground(matchIndex == m_currentMatchIndex - 1 ? currentMatchColor : matchColor);
            // 不使用 FullWidthSelection，只高亮匹配的文字
            extraSelections.append(selection);
            matchIndex++;
        }
    }

    m_chatDisplay->setExtraSelections(extraSelections);
}

void MainWindow::clearHighlights()
{
    // 清除所有高亮
    m_chatDisplay->setExtraSelections(QList<QTextEdit::ExtraSelection>());
    // 清除选中状态
    QTextCursor cursor = m_chatDisplay->textCursor();
    cursor.clearSelection();
    m_chatDisplay->setTextCursor(cursor);
}

void MainWindow::updateCurrentMatchIndex()
{
    QString keyword = m_searchInput->text().trimmed();
    if (keyword.isEmpty() || m_totalMatches == 0) {
        return;
    }

    QTextDocument *doc = m_chatDisplay->document();
    QTextCursor currentCursor = m_chatDisplay->textCursor();
    QTextCursor cursor(doc);
    int index = 0;

    while (!cursor.isNull() && cursor.position() <= currentCursor.position()) {
        cursor = doc->find(keyword, cursor);
        if (!cursor.isNull()) {
            if (cursor.position() == currentCursor.position()) {
                m_currentMatchIndex = index + 1;
                break;
            }
            index++;
        }
    }

    updateSearchResultLabel();
}

void MainWindow::updateSearchResultLabel()
{
    if (m_totalMatches == 0) {
        m_searchResultLabel->setText(tr("无结果"));
    } else {
        m_searchResultLabel->setText(QString("%1/%2").arg(m_currentMatchIndex).arg(m_totalMatches));
    }
}

void MainWindow::onGirlfriendClicked()
{
    static GirlfriendWindow *girlfriendWindow = nullptr;

    // 如果窗口已存在且可见，则关闭它
    if (girlfriendWindow && girlfriendWindow->isVisible()) {
        girlfriendWindow->close();
        return;
    }

    // 否则创建或显示窗口
    if (!girlfriendWindow) {
        girlfriendWindow = new GirlfriendWindow(this);
    }
    girlfriendWindow->show();
    girlfriendWindow->raise();
    girlfriendWindow->activateWindow();
}
