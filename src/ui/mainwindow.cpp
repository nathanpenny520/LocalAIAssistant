#include "mainwindow.h"
#include "stylesheetmanager.h"
#include "translationmanager.h"
#include "markdownrenderer.h"
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
{
    setupUI();
    setupMenuBar();

    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
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

    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(m_inputLine);
    inputLayout->addWidget(m_sendButton);
    rightLayout->addLayout(inputLayout);

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
