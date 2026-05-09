#include "mainwindow.h"

#include <algorithm>

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QWidget>

#include "filemanager.h"
#include "markdownrenderer.h"
#include "operationconfirmdialog.h"
#include "stylesheetmanager.h"
#include "translationmanager.h"

namespace {
constexpr int kSidebarWidth = 200;
}

// Parse thinking content from AI response
// Returns a map with "thinking" and "response" keys
QMap<QString, QString> MainWindow::parseThinkingContent(const QString& content) {
    QMap<QString, QString> result;
    result["thinking"] = "";
    result["response"] = content;

    // Match <think>...</think> tags
    QRegularExpression thinkingRegex(R"(<think>(.*?)</think>)",
                                     QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatch match = thinkingRegex.match(content);
    if (match.hasMatch()) {
        QString thinkingContent = match.captured(1).trimmed();

        // Remove thinking tags from original content to get pure response
        QString pureResponse = content;
        pureResponse.remove(thinkingRegex);
        pureResponse = pureResponse.trimmed();

        result["thinking"] = thinkingContent;
        result["response"] = pureResponse;
    }

    return result;
}

QString MainWindow::formatMessageWithThinking(const QString& role, const QString& content) {
    QString thinkingLabel = tr("思考过程");
    QString userLabel = tr("用户");
    QString aiLabel = tr("AI");

    const AppTheme& t = AppTheme::current();

    if (role == "user") {
        // User message in a <table> — Qt rich-text handles tables more reliably than <div>
        QString escapedContent = content;
        escapedContent.replace("&", "&amp;");
        escapedContent.replace("<", "&lt;");
        escapedContent.replace(">", "&gt;");
        escapedContent.replace("\n", "<br>");

        bool isDark = t.windowBg.lightness() < 128;
        QString boxBg = isDark ? "#1e2a3a" : "#f6f7fa";
        QString boxBorder = isDark ? "#334" : "#d8dce6";

        return QString("<table width='100%%' cellpadding='0' cellspacing='0' "
                       "style='background:%1; border:1px solid %2; margin-top:18px; "
                       "margin-bottom:4px;'>"
                       "<tr><td style='padding:10px 14px; border:none; color:%3; line-height:1.6;'>"
                       "<b style='color:%4; font-size:18px;'>%5</b><br>%6"
                       "</td></tr></table>")
                .arg(boxBg, boxBorder, t.textPrimary.name(), t.accent.name(), userLabel,
                     escapedContent);
    }

    // AI message
    QMap<QString, QString> parsed = parseThinkingContent(content);
    QString thinking = parsed["thinking"];
    QString response = parsed["response"];

    QString html;

    // AI label
    html += QString("<p style='margin:0 0 8px 0;'><b style='color:%1; font-size:18px;'>%2</b></p>")
                    .arg(t.accent.name(), aiLabel);

    // Thinking content: visually distinguished block with muted colors
    if (!thinking.isEmpty()) {
        QString escapedThinking = thinking;
        escapedThinking.replace("&", "&amp;");
        escapedThinking.replace("<", "&lt;");
        escapedThinking.replace(">", "&gt;");
        escapedThinking.replace("\n", "<br>");

        html += QString("<div style='margin-bottom:14px; padding:10px 14px;"
                        "  background-color:%1; border-left:3px solid %2; border-radius:4px;'>"
                        "<div style='font-weight:bold; font-size:14px; color:%3; "
                        "margin-bottom:6px;'>"
                        "%4</div>"
                        "<div style='font-size:13px; color:%5; line-height:1.6;'>"
                        "%6</div>"
                        "</div>")
                        .arg(t.surfaceBg.name(), t.quoteBorder.name(), t.accent.name(),
                             thinkingLabel, t.textSecondary.name(), escapedThinking);
    }

    // AI response: pure markdown
    if (!response.isEmpty()) {
        html += MarkdownRenderer::toHtml(response);
    } else if (thinking.isEmpty()) {
        html += MarkdownRenderer::toHtml(content);
    }

    return html;
}

MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent)
        , m_historyList(new QListWidget(this))
        , m_chatDisplay(new QTextBrowser(this))
        , m_inputLine(new QPlainTextEdit(this))
        , m_sendButton(new QPushButton(tr("发送"), this))
        , m_newChatButton(new QPushButton(tr("+ 新建对话"), this))
        , m_settingsAction(new QAction(tr("设置"), this))
        , m_toggleHistoryAction(new QAction(tr("显示历史面板"), this))
        , m_contextMenu(new QMenu(this))
        , m_deleteAction(new QAction(tr("删除该对话"), this))
        , m_renameAction(new QAction(tr("重命名"), this))
        , m_pinAction(new QAction(tr("置顶"), this))
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
        , m_totalMatches(0) {
    setupUI();
    setupMenuBar();

    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_fileButton, &QPushButton::clicked, this, &MainWindow::onFileButtonClicked);
    // Install event filter so Enter sends and Shift+Enter inserts newline
    m_inputLine->installEventFilter(this);
    m_inputPlaceholder = tr("输入消息... (Enter发送, Shift+Enter换行)");
    m_inputLine->setPlaceholderText(m_inputPlaceholder);
    m_inputLine->setMaximumHeight(m_maxInputHeight);
    m_inputLine->setTabChangesFocus(true);
    m_inputLine->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    // Dynamic height: grow from 1 line, cap at m_maxInputHeight
    connect(m_inputLine, &QPlainTextEdit::textChanged, this, [this]() {
        // Hide placeholder when user has typed anything; restore when empty
        if (m_inputLine->toPlainText().isEmpty()) {
            m_inputLine->setPlaceholderText(m_inputPlaceholder);
        } else {
            m_inputLine->setPlaceholderText(QString());
        }
        adjustInputHeight();
    });
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::onSettingsClicked);
    connect(m_toggleHistoryAction, &QAction::triggered, this, &MainWindow::onToggleHistoryPanel);
    connect(m_newChatButton, &QPushButton::clicked, this, &MainWindow::onNewChatClicked);
    connect(m_historyList, &QListWidget::itemClicked, this, &MainWindow::onSessionItemClicked);
    connect(m_deleteAction, &QAction::triggered, this, &MainWindow::onDeleteSession);
    connect(m_renameAction, &QAction::triggered, this, &MainWindow::onRenameSession);
    connect(m_pinAction, &QAction::triggered, this, &MainWindow::onTogglePinSession);
    connect(m_historyList, &QWidget::customContextMenuRequested, this,
            &MainWindow::onCustomContextMenuRequested);

    connect(m_networkManager, &NetworkManager::responseReceived, this,
            &MainWindow::onNetworkFinished);
    connect(m_networkManager, &NetworkManager::streamChunkReceived, this,
            &MainWindow::onStreamChunkReceived);
    connect(m_networkManager, &NetworkManager::streamFinished, this, &MainWindow::onStreamFinished);
    connect(m_networkManager, &NetworkManager::errorOccurred, this, &MainWindow::onNetworkError);
    connect(SessionManager::instance(), &SessionManager::sessionChanged, this,
            [this](const QString& sessionId) {
                // Only re-render if the changed session is currently being displayed.
                // Background updates (e.g. streaming to another session) should not
                // disrupt the current view.
                if (sessionId == SessionManager::instance()->currentSessionId()) {
                    renderCurrentSession();
                }
            });
    connect(StyleSheetManager::instance(), &StyleSheetManager::themeChanged, this,
            &MainWindow::onThemeChanged);
    connect(TranslationManager::instance(), &TranslationManager::languageChanged, this,
            &MainWindow::onLanguageChanged);

    // Search feature connections
    m_searchAction->setShortcut(QKeySequence::Find);  // Ctrl+F / Cmd+F
    connect(m_searchAction, &QAction::triggered, this, &MainWindow::onSearchTriggered);

    KnowledgeBase::instance()->init();

    // Load user-defined path allowlist from QSettings into SafetyChecker
    {
        QSettings settings("LocalAIAssistant", "Settings");
        QStringList savedWhitelist = settings.value("pathWhitelist").toStringList();
        if (!savedWhitelist.isEmpty()) {
            TaskEngine::instance()->safetyChecker().setAllowedPaths(savedWhitelist);
        }
    }

    // Load saved sessions from disk
    SessionManager::instance()->loadSessionsFromFile();
    renderCurrentSession();

    StyleSheetManager::instance()->applyTheme(this);

    // Install global event filter to catch IME composition events (e.g. pinyin)
    qApp->installEventFilter(this);
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    // Widget geometry is now valid — set correct initial input height
    adjustInputHeight();
    if (m_firstShow) {
        m_firstShow = false;
        updateSessionList();
    }
}

MainWindow::~MainWindow() {
    delete m_markdownDoc;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    SessionManager::instance()->saveSessionsToFile();
    event->accept();
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
        setupMenuBar();
        renderCurrentSession();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    m_splitter = new QSplitter(Qt::Horizontal, this);

    m_leftPanel = new QWidget(m_splitter);
    QVBoxLayout* leftLayout = new QVBoxLayout(m_leftPanel);
    leftLayout->setContentsMargins(5, 5, 5, 5);
    m_newChatButton->setObjectName(QStringLiteral("newChatButton"));
    leftLayout->addWidget(m_newChatButton);
    leftLayout->addWidget(m_historyList);
    m_historyList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_historyList->setToolTip(tr("Right-click a session for more options"));

    // Prevent left panel from being fully collapsed
    m_leftPanel->setMinimumWidth(140);

    QWidget* rightPanel = new QWidget(m_splitter);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(5, 5, 5, 5);

    // Search bar (hidden by default, shown via Ctrl+F)
    setupSearchBar();
    rightLayout->addWidget(m_searchBar);

    rightLayout->addWidget(m_chatDisplay, 1);

    // File attachment tags (above input)
    m_fileListArea = new QWidget(rightPanel);
    m_fileListLayout = new QHBoxLayout(m_fileListArea);
    m_fileListLayout->setContentsMargins(0, 0, 0, 5);
    m_fileListLayout->addStretch();     // keep tags left-aligned
    m_fileListArea->setVisible(false);
    rightLayout->addWidget(m_fileListArea);

    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLayout->addWidget(m_inputLine, 1);
    inputLayout->addWidget(m_fileButton);
    inputLayout->addWidget(m_sendButton);
    rightLayout->addLayout(inputLayout);

    QIcon fileIcon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
    m_fileButton->setIcon(fileIcon);
    m_fileButton->setIconSize(QSize(20, 20));
    m_fileButton->setFixedSize(40, 30);
    m_fileButton->setToolTip(tr("添加文件"));

    m_splitter->setSizes({kSidebarWidth, 580});

    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_splitter);

    m_contextMenu->addAction(m_renameAction);
    m_contextMenu->addAction(m_pinAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_deleteAction);
    m_historyList->setContextMenuPolicy(Qt::CustomContextMenu);

    resize(900, 600);
    setWindowTitle(tr("本地AI助手"));

    // Set window icon for taskbar display
    // On Windows, this ensures correct taskbar icon
    // On macOS, the bundle icon is handled by Info.plist
#ifdef Q_OS_WIN
    // Load icon from executable's directory (copied by CMake)
    QString iconPath = QCoreApplication::applicationDirPath() + "/app.ico";
    if (QFile::exists(iconPath)) {
        setWindowIcon(QIcon(iconPath));
    }
#endif
}

void MainWindow::setupMenuBar() {
    if (menuBar()) {
        menuBar()->clear();
    }

    QMenuBar* bar = menuBar() ? menuBar() : new QMenuBar(this);

#ifdef Q_OS_MACOS
    bar->setNativeMenuBar(true);

    QMenu* appMenu = bar->addMenu(tr("本地AI助手"));
    QAction* prefAction = appMenu->addAction(tr("偏好设置..."));
    prefAction->setMenuRole(QAction::PreferencesRole);
    prefAction->setShortcut(QKeySequence::StandardKey::Preferences);
    connect(prefAction, &QAction::triggered, this, &MainWindow::onSettingsClicked);

    appMenu->addSeparator();

    QAction* quitAction = appMenu->addAction(tr("退出 本地AI助手"));
    quitAction->setMenuRole(QAction::QuitRole);
    quitAction->setShortcut(QKeySequence::StandardKey::Quit);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    QMenu* fileMenu = bar->addMenu(tr("文件"));
#else
    QMenu* fileMenu = bar->addMenu(tr("文件"));

    m_settingsAction->setText(tr("设置..."));
    fileMenu->addAction(m_settingsAction);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction(tr("退出"));
    exitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
#endif

    QMenu* viewMenu = bar->addMenu(tr("视图"));

    m_toggleHistoryAction->setText(tr("显示历史面板"));
    m_toggleHistoryAction->setCheckable(true);
    m_toggleHistoryAction->setChecked(m_leftPanel && m_leftPanel->width() > 0);
    m_toggleHistoryAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_H));
    viewMenu->addAction(m_toggleHistoryAction);

    QMenu* editMenu = bar->addMenu(tr("编辑"));

    m_searchAction->setText(tr("查找..."));
    editMenu->addAction(m_searchAction);

    m_girlfriendAction->setText(tr("AI女友"));
    m_girlfriendAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
    viewMenu->addAction(m_girlfriendAction);
    connect(m_girlfriendAction, &QAction::triggered, this, &MainWindow::onGirlfriendClicked);

    if (!menuBar()) {
        setMenuBar(bar);
    }
}

void MainWindow::retranslateUi() {
    m_sendButton->setText(tr("发送"));
    m_newChatButton->setText(tr("+ 新建对话"));
    m_renameAction->setText(tr("重命名"));
    m_pinAction->setText(tr("置顶"));
    m_deleteAction->setText(tr("删除该对话"));
    setWindowTitle(tr("本地AI助手"));

    m_fileButton->setToolTip(tr("添加文件"));

    m_toggleHistoryAction->setText(tr("显示历史面板"));
    m_searchAction->setText(tr("查找..."));
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

    m_girlfriendAction->setText(tr("AI女友"));

    // Update input placeholder; only show if input is empty
    m_inputPlaceholder = tr("输入消息... (Enter发送, Shift+Enter换行)");
    if (m_inputLine->toPlainText().isEmpty()) {
        m_inputLine->setPlaceholderText(m_inputPlaceholder);
    }

    updateSessionList();
    setInputEnabled(!m_isStreaming);
}

void MainWindow::renderCurrentSession() {
    if (m_isRendering) {
        return;
    }

    // During onSendClicked we append the user message via the cursor API
    // and suppress the setHtml() call that addMessageToCurrentSession would
    // trigger. This flag is cleared right after appendUserMessageToDisplay.
    if (m_suppressRender) {
        return;
    }

    m_isRendering = true;

    const auto& session = SessionManager::instance()->currentSession();

    m_chatDisplay->setUpdatesEnabled(false);
    m_chatDisplay->clear();
    QTextCursor cursor = m_chatDisplay->textCursor();

    for (int i = 0; i < session.messages.size(); ++i) {
        const auto& msg = session.messages[i];
        QString rendered = formatMessageWithThinking(msg.role, msg.content);

        if (msg.role == "user" && !msg.attachments.isEmpty()) {
            QString attachmentHtml;
            for (const auto& attachment : msg.attachments) {
                if (attachment.type == "image") {
                    attachmentHtml += QString("<div style='margin:8px 0;'>"
                                              "<img src='%1' style='max-width:300px; "
                                              "max-height:200px; border-radius:8px; border:1px "
                                              "solid #ccc;' />"
                                              "</div>")
                                              .arg(attachment.content);
                } else {
                    QString iconColor = (attachment.type == "text") ? "#4CAF50" : "#FF9800";
                    QString typeLabel = (attachment.type == "text") ? tr("文本") : tr("二进制");
                    QFileInfo info(attachment.path);
                    attachmentHtml += QString("<div style='margin:8px 0; padding:8px 12px; "
                                              "background:#f5f5f5; border-radius:6px; "
                                              "display:inline-block;'>"
                                              "<span style='color:%1; "
                                              "font-weight:bold;'>[%2]</span> %3 (%4 KB)"
                                              "</div>")
                                              .arg(iconColor)
                                              .arg(typeLabel)
                                              .arg(info.fileName())
                                              .arg(attachment.size / 1024);
                }
            }
            rendered = rendered.replace(QStringLiteral("</table>"),
                                        attachmentHtml + QStringLiteral("</table>"));
        }

        cursor.insertHtml(rendered);

        // Spacer between messages — HTML-based to avoid block-format leakage
        if (i < session.messages.size() - 1) {
            cursor.insertHtml(QStringLiteral("<p style='margin:0; line-height:1px;'>&nbsp;</p>"));
        }
    }

    // Remove trailing empty block so subsequent cursor API appends don't create a gap
    QTextDocument* doc = m_chatDisplay->document();
    if (doc->blockCount() > 1 && doc->lastBlock().text().isEmpty()) {
        QTextCursor cleanup(doc->lastBlock());
        cleanup.deletePreviousChar();
    }

    QScrollBar* scrollBar = m_chatDisplay->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());

    m_chatDisplay->setUpdatesEnabled(true);

    m_isRendering = false;
}

void MainWindow::appendUserMessageToDisplay(const QString& text,
                                            const QVector<FileAttachment>& attachments) {
    QTextCursor cursor = m_chatDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);

    const AppTheme& t = AppTheme::current();

    // Escape HTML in user text
    QString escaped = text;
    escaped.replace(QLatin1String("&"), QLatin1String("&amp;"));
    escaped.replace(QLatin1String("<"), QLatin1String("&lt;"));
    escaped.replace(QLatin1String(">"), QLatin1String("&gt;"));
    escaped.replace(QLatin1String("\n"), QLatin1String("<br>"));

    // User message: table-based box (Qt handles tables reliably)
    bool isDark = t.windowBg.lightness() < 128;
    QString boxBg = isDark ? "#1e2a3a" : "#f6f7fa";
    QString boxBorder = isDark ? "#334" : "#d8dce6";

    cursor.insertHtml(QString("<table width='100%%' cellpadding='0' cellspacing='0' "
                              "style='background:%1; border:1px solid %2; margin-top:18px; "
                              "margin-bottom:4px;'>"
                              "<tr><td style='padding:10px 14px; border:none; color:%3; "
                              "line-height:1.6;'>"
                              "<b style='color:%4; font-size:18px;'>%5</b><br>%6"
                              "</td></tr></table>")
                              .arg(boxBg, boxBorder, t.textPrimary.name(), t.accent.name(),
                                   tr("用户"), escaped));

    // Append attachment info if any
    for (const auto& attachment : attachments) {
        if (attachment.type == QStringLiteral("image")) {
            cursor.insertBlock();
            cursor.insertHtml(QStringLiteral("<div style='margin:8px 0;'>"
                                             "<img src='%1' style='max-width:300px; "
                                             "max-height:200px; border-radius:8px; border:1px "
                                             "solid #ccc;' />"
                                             "</div>")
                                      .arg(attachment.content));
        } else {
            QString iconColor = (attachment.type == QStringLiteral("text"))
                                        ? QStringLiteral("#4CAF50")
                                        : QStringLiteral("#FF9800");
            QString typeLabel = (attachment.type == QStringLiteral("text")) ? tr("文本")
                                                                            : tr("二进制");
            QFileInfo info(attachment.path);
            cursor.insertBlock();
            cursor.insertHtml(QStringLiteral("<div style='margin:8px 0; padding:8px 12px; "
                                             "background:#f5f5f5; border-radius:6px; "
                                             "display:inline-block;'>"
                                             "<span style='color:%1; "
                                             "font-weight:bold;'>[%2]</span> %3 (%4 KB)"
                                             "</div>")
                                      .arg(iconColor, typeLabel, info.fileName())
                                      .arg(attachment.size / 1024));
        }
    }

    // Scroll to show the new message
    QScrollBar* scrollBar = m_chatDisplay->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::updateSessionList() {
    m_sessionItemMap.clear();
    m_historyList->clear();

    const auto& sessions = SessionManager::instance()->allSessions();

    // Sort: pinned first, then by title
    QVector<ChatSession> sorted;
    for (const auto& s : sessions) sorted.append(s);
    std::sort(sorted.begin(), sorted.end(), [](const ChatSession& a, const ChatSession& b) {
        if (a.pinned != b.pinned) return a.pinned > b.pinned;
        return a.title.toLower() < b.title.toLower();
    });

    QString currentId = SessionManager::instance()->currentSessionId();
    const AppTheme& theme = AppTheme::current();

    for (const auto& session : sorted) {
        QListWidgetItem* item = new QListWidgetItem();
        item->setData(Qt::UserRole, session.id);
        item->setSizeHint(QSize(0, 44));

        // Custom widget: [pin icon] title
        QWidget* itemWidget = new QWidget();
        itemWidget->setStyleSheet(QStringLiteral("background: transparent;"));
        itemWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QHBoxLayout* layout = new QHBoxLayout(itemWidget);
        layout->setContentsMargins(6, 1, 4, 1);
        layout->setSpacing(4);

        QString displayTitle = session.title.isEmpty() ? tr("新对话") : session.title;

        QLabel* titleLabel = new QLabel(displayTitle);
        titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        titleLabel->setWordWrap(false);
        titleLabel->setTextFormat(Qt::PlainText);

        if (session.pinned) {
            QFont f = titleLabel->font();
            f.setBold(true);
            titleLabel->setFont(f);
            titleLabel->setStyleSheet(QStringLiteral("color: %1;").arg(theme.accent.name()));
            titleLabel->setText(QStringLiteral("📌 ") + displayTitle);
        }

        layout->addWidget(titleLabel, 1);

        m_historyList->addItem(item);
        m_historyList->setItemWidget(item, itemWidget);
        m_sessionItemMap[session.id] = item;

        if (session.id == currentId) {
            m_historyList->setCurrentItem(item);
            m_historyList->scrollToItem(item);
        }
    }
}

void MainWindow::setInputEnabled(bool enabled) {
    m_inputLine->setReadOnly(!enabled);

    if (enabled) {
        m_inputLine->setPlaceholderText(m_inputPlaceholder);
        m_sendButton->setText(tr("发送"));
        m_sendButton->setEnabled(true);
    } else {
        m_inputLine->setPlaceholderText(tr("正在思考..."));
        m_sendButton->setText(tr("停止"));
        m_sendButton->setEnabled(true);  // keep enabled so user can click to abort
    }
}

void MainWindow::stopCurrentStreamingSession() {
    m_networkManager->abortCurrentRequest();
    if (!m_streamingContent.isEmpty()) {
        SessionManager::instance()->addMessageToSession(m_requestSessionId, "assistant",
                                                        m_streamingContent);
    }
    m_isStreaming = false;
    m_streamingContent.clear();
    m_requestSessionId.clear();
}

void MainWindow::onSendClicked() {
    // If another session is streaming, stop it first, then proceed with normal send
    if (m_isStreaming && !m_requestSessionId.isEmpty() &&
        m_requestSessionId != SessionManager::instance()->currentSessionId()) {
        stopCurrentStreamingSession();
        // Fall through to normal send below
    }

    // If THIS session is streaming, treat click as stop
    if (m_isStreaming) {
        stopCurrentStreamingSession();
        setInputEnabled(true);
        return;
    }

    QString userInput = m_inputLine->toPlainText().trimmed();
    if (userInput.isEmpty() && m_fileManager->pendingFileCount() == 0) {
        return;  // nothing to send
    }

    if (m_fileManager->pendingFileCount() > 0) {
        qDebug() << tr("发送消息时携带 %1 个文件").arg(m_fileManager->pendingFileCount());
    }

    QVector<FileAttachment> attachments;
    if (m_fileManager->pendingFileCount() > 0) {
        attachments = m_fileManager->pendingFiles();
    }

    // Inject knowledge base results into system prompt (not user message)
    // to avoid polluting the conversation context
    KnowledgeBase* kb = KnowledgeBase::instance();
    if (kb->isReady()) {
        QString context = kb->generateContext(userInput);
        if (!context.isEmpty()) m_networkManager->setKnowledgeContext(context);
    }

    // Suppress renderCurrentSession while we add the user message, so the
    // setHtml() call doesn't disrupt existing chat content. We append the
    // user message via the cursor API instead.
    m_suppressRender = true;
    m_isStreaming = true;
    m_streamingContent.clear();
    m_streamEndedWithNewline = false;

    // Save user input to session (renderCurrentSession is suppressed by m_suppressRender)
    if (attachments.isEmpty()) {
        SessionManager::instance()->addMessageToCurrentSession("user", userInput);
    } else {
        SessionManager::instance()->addMessageToCurrentSession("user", userInput, attachments);
        m_fileManager->clearPendingFiles();
        clearFileListDisplay();
        m_fileButton->setToolTip(tr("添加文件"));
    }

    // Manually append user message to chat display
    appendUserMessageToDisplay(userInput, attachments);
    m_suppressRender = false;

    m_inputLine->clear();

    QVector<ChatMessage> messages = SessionManager::instance()->currentSession().messages;

    m_requestSessionId = SessionManager::instance()->currentSessionId();
    setInputEnabled(false);
    m_networkManager->sendChatRequestWithContext(messages);
}

void MainWindow::onNetworkFinished(const QString& response) {
    // Verify response belongs to the session that initiated the request
    if (m_requestSessionId.isEmpty()) {
        return;
    }

    m_isStreaming = false;
    m_streamingContent.clear();

    // Check if response contains a task plan
    if (response.contains(QStringLiteral("[TASK_PLAN]"))) {
        handleTaskResponse(response);
        m_requestSessionId.clear();
        setInputEnabled(true);
        return;
    }

    SessionManager::instance()->addMessageToSession(m_requestSessionId, "assistant", response);

    // renderCurrentSession() is triggered via sessionChanged signal

    m_requestSessionId.clear();
    setInputEnabled(true);
}

void MainWindow::onNetworkError(const QString& error) {
    // Verify response belongs to the session that initiated the request
    if (m_requestSessionId.isEmpty()) {
        return;
    }

    m_isStreaming = false;
    m_streamingContent.clear();

    SessionManager::instance()->addMessageToSession(m_requestSessionId, "assistant",
                                                    tr("错误: ") + error);

    // renderCurrentSession() is triggered via sessionChanged signal

    m_requestSessionId.clear();
    setInputEnabled(true);
}

void MainWindow::onStreamChunkReceived(const QString& chunk) {
    if (!m_isStreaming || m_requestSessionId != SessionManager::instance()->currentSessionId()) {
        return;
    }

    m_streamingContent += chunk;

    QTextCursor cursor = m_chatDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);

    // First chunk — insert AI label then start response text
    if (m_streamingContent == chunk) {
        cursor.insertBlock();
        QTextCharFormat boldFormat;
        boldFormat.setForeground(QColor(QStringLiteral("#007aff")));
        boldFormat.setFontWeight(QFont::Bold);
        boldFormat.setFontPointSize(14);
        cursor.insertText(tr("AI"), boldFormat);
        cursor.insertBlock();
    }

    // Append chunk text, converting \n to paragraph blocks for readable streaming
    QTextCharFormat normalFormat;
    const QStringList lines = chunk.split(QChar::LineFeed);
    bool hadEmptyLine = false;
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].isEmpty()) {
            hadEmptyLine = true;
            continue;
        }
        if (i > 0) {
            // Skip insertBlock if previous chunk ended with \n and this chunk starts with \n
            bool skipBlock = (i == 1 && m_streamEndedWithNewline && lines[0].isEmpty());
            if (!skipBlock) {
                cursor.insertBlock();
                if (hadEmptyLine) {
                    cursor.insertBlock();  // intentional blank line between paragraphs
                }
            }
        }
        cursor.insertText(lines[i], normalFormat);
        hadEmptyLine = false;
    }
    m_streamEndedWithNewline = chunk.endsWith(QLatin1Char('\n'));

    QScrollBar* scrollBar = m_chatDisplay->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::onStreamFinished(const QString& fullContent) {
    // Verify response belongs to the session that initiated the request
    if (!m_isStreaming || m_requestSessionId.isEmpty()) {
        return;
    }

    m_isStreaming = false;

    // Check if response contains a task plan
    if (fullContent.contains(QStringLiteral("[TASK_PLAN]"))) {
        handleTaskResponse(fullContent);
        m_streamingContent.clear();
        m_requestSessionId.clear();
        setInputEnabled(true);
        return;
    }

    SessionManager::instance()->addMessageToSession(m_requestSessionId, "assistant", fullContent);

    // Auto-name: generate title from first user message if session has not yet been auto-named
    const auto& sessions = SessionManager::instance()->allSessions();
    if (sessions.contains(m_requestSessionId)) {
        const auto& session = sessions[m_requestSessionId];
        if (!session.autoNamed && session.messages.size() >= 2) {
            QString firstUserMsg;
            for (const auto& msg : session.messages) {
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

    // renderCurrentSession() is already triggered via sessionChanged signal
    // from addMessageToSession() above — no need for an explicit call

    m_streamingContent.clear();
    m_requestSessionId.clear();
    setInputEnabled(true);
}

void MainWindow::onSettingsClicked() {
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        m_networkManager->updateSettings(dialog.getApiBaseUrl(), dialog.getApiKey(),
                                         dialog.getModelName(), dialog.getApiType());
        m_networkManager->setStreamingEnabled(dialog.isStreamingEnabled());

        // Sync path allowlist to SafetyChecker
        SafetyChecker& checker = TaskEngine::instance()->safetyChecker();
        QStringList customWhitelist = dialog.pathWhitelist();
        if (!customWhitelist.isEmpty()) {
            checker.setAllowedPaths(customWhitelist);
        } else {
            checker.resetToDefaults();
        }
    }
}

void MainWindow::onNewChatClicked() {
    QString defaultTitle = TranslationManager::instance()->currentLocale() == "en" ? "New Chat"
                                                                                   : tr("新对话");
    SessionManager::instance()->createNewSession(defaultTitle);
    m_chatDisplay->clear();
    m_markdownDoc->clear();
    m_inputLine->clear();
    m_inputLine->setFocus();
    updateSessionList();

    // Re-enable input when creating a new session during active streaming
    if (m_isStreaming) {
        setInputEnabled(true);
    }
}

void MainWindow::onToggleHistoryPanel() {
    if (!m_splitter || !m_leftPanel) {
        return;
    }

    // If panel is hidden (at minimum width), restore to normal width
    if (m_leftPanel->width() <= m_leftPanel->minimumWidth()) {
        m_splitter->setSizes({kSidebarWidth, m_splitter->width() - kSidebarWidth});
        m_toggleHistoryAction->setChecked(true);
        updateSessionList();
    } else {
        // Collapse panel to minimum width
        m_splitter->setSizes({0, m_splitter->width()});
        m_toggleHistoryAction->setChecked(false);
    }
}

void MainWindow::onSessionItemClicked(QListWidgetItem* item) {
    if (!item) {
        return;
    }

    QString sessionId = item->data(Qt::UserRole).toString();

    // No-op if already viewing this session (prevents unnecessary re-render
    // that would wipe live streaming display)
    if (sessionId == SessionManager::instance()->currentSessionId()) {
        return;
    }

    SessionManager::instance()->switchToSession(sessionId);
    updateSessionList();

    // Re-enable input when switching to a non-streaming session
    if (m_isStreaming && !m_requestSessionId.isEmpty() && sessionId != m_requestSessionId) {
        setInputEnabled(true);
    }
    // Restore stop button when switching back to the streaming session
    else if (m_isStreaming && !m_requestSessionId.isEmpty() && sessionId == m_requestSessionId) {
        setInputEnabled(false);
    }
}

void MainWindow::onDeleteSession() {
    QString sessionId = m_contextMenuSessionId;
    if (sessionId.isEmpty()) return;

    const auto& sessions = SessionManager::instance()->allSessions();

    // Don't allow deleting the last remaining session
    if (sessions.size() <= 1) return;

    // Don't allow deleting a session that has an active stream in flight
    if (m_isStreaming && sessionId == m_requestSessionId) return;

    // If deleting the current active session, switch to another first
    if (sessionId == SessionManager::instance()->currentSessionId()) {
        QString targetId;
        for (const auto& s : sessions) {
            if (s.id != sessionId) {
                targetId = s.id;
                break;
            }
        }
        if (!targetId.isEmpty()) SessionManager::instance()->switchToSession(targetId);
    }

    SessionManager::instance()->removeSession(sessionId);
    if (m_sessionItemMap.contains(sessionId)) {
        delete m_sessionItemMap[sessionId];
        m_sessionItemMap.remove(sessionId);
    }
    updateSessionList();
}

void MainWindow::onRenameSession() {
    QString sessionId = m_contextMenuSessionId;
    if (sessionId.isEmpty()) return;

    const auto& sessions = SessionManager::instance()->allSessions();
    if (!sessions.contains(sessionId)) return;

    QString oldTitle = sessions[sessionId].title;
    bool ok = false;
    QString newTitle = QInputDialog::getText(this, tr("重命名"), tr("新名称:"), QLineEdit::Normal,
                                             oldTitle, &ok);
    if (ok && !newTitle.trimmed().isEmpty()) {
        SessionManager::instance()->updateSessionTitle(sessionId, newTitle.trimmed());
        updateSessionList();
    }
}

void MainWindow::onTogglePinSession() {
    QString sessionId = m_contextMenuSessionId;
    if (sessionId.isEmpty()) return;

    const auto& sessions = SessionManager::instance()->allSessions();
    if (!sessions.contains(sessionId)) return;

    bool newPinned = !sessions[sessionId].pinned;
    SessionManager::instance()->setSessionPinned(sessionId, newPinned);
    updateSessionList();
}

void MainWindow::onCustomContextMenuRequested(const QPoint& pos) {
    QListWidgetItem* item = m_historyList->itemAt(pos);
    if (item) {
        m_contextMenuSessionId = item->data(Qt::UserRole).toString();

        // Update pin action text
        const auto& sessions = SessionManager::instance()->allSessions();
        if (sessions.contains(m_contextMenuSessionId)) {
            m_pinAction->setText(sessions[m_contextMenuSessionId].pinned ? tr("取消置顶")
                                                                         : tr("置顶"));
        }

        m_contextMenu->exec(m_historyList->mapToGlobal(pos));
    }
}

void MainWindow::onThemeChanged(int theme) {
    Q_UNUSED(theme);
    // Clear search highlights first to avoid display issues
    if (m_searchBar && m_searchBar->isVisible()) {
        clearHighlights();
    }

    this->setStyleSheet(StyleSheetManager::instance()->currentStyleSheet());
    // Re-render chat content with new theme colors
    renderCurrentSession();
    // Rebuild session list so "⋯" buttons pick up new palette colors
    updateSessionList();

    // Re-apply highlights with new theme colors if there's a search term
    // This must be done after renderCurrentSession() completes
    if (m_searchBar && m_searchBar->isVisible() && m_searchInput) {
        QString keyword = m_searchInput->text().trimmed();
        if (!keyword.isEmpty()) {
            // Recalculate match count after re-render
            QTextDocument* doc = m_chatDisplay->document();
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

void MainWindow::onLanguageChanged() {
    retranslateUi();
    setupMenuBar();
    renderCurrentSession();
}

void MainWindow::onFileButtonClicked() {
    // The language of the macOS native file dialog is determined by the
    // system language preference order and cannot be controlled via Qt.
    // To use an English dialog, set English as the preferred language in
    // macOS System Settings.

    QStringList filePaths = QFileDialog::getOpenFileNames(this, QString(), QString(), QString());

    if (filePaths.isEmpty()) {
        return;
    }

    QString errorTitle = tr("错误");

    for (const QString& path : filePaths) {
        QFileInfo info(path);
        if (!info.exists()) {
            QMessageBox::warning(this, errorTitle, tr("文件不存在: %1").arg(path));
            continue;
        }

        if (info.size() > 10 * 1024 * 1024) {
            QMessageBox::warning(this, errorTitle, tr("文件过大 (>10MB): %1").arg(path));
            continue;
        }

        if (m_fileManager->addFile(path)) {
            qDebug() << "File added:" << path;
        }
    }

    updateFileListDisplay();
}

void MainWindow::updateFileListDisplay() {
    clearFileListDisplay();

    QVector<FileAttachment> files = m_fileManager->pendingFiles();
    if (files.isEmpty()) {
        m_fileListArea->setVisible(false);
        return;
    }

    m_fileListArea->setVisible(true);

    for (const FileAttachment& file : files) {
        QWidget* fileTag = new QWidget(m_fileListArea);
        QHBoxLayout* tagLayout = new QHBoxLayout(fileTag);
        tagLayout->setContentsMargins(4, 2, 4, 2);
        tagLayout->setSpacing(4);

        QString borderColor;
        if (file.type == "text") {
            borderColor = "#007aff";
        } else if (file.type == "image") {
            borderColor = "#34c759";
        } else {
            borderColor = "#8e8e93";
        }

        QString displayName = QFileInfo(file.path).fileName();
        if (displayName.length() > 20) {
            displayName = displayName.left(17) + "...";
        }

        QLabel* nameLabel = new QLabel(displayName, fileTag);

        const AppTheme& t = AppTheme::current();

        nameLabel->setStyleSheet(
                QString("QLabel { color: %1; font-size: 12px; padding: 2px 6px; "
                        "border: 1px solid %2; border-radius: 4px; background: %3; }")
                        .arg(t.textPrimary.name(), borderColor, t.surfaceBg.name()));
        tagLayout->addWidget(nameLabel);

        // Remove button with theme-adaptive close icon
        QPushButton* removeBtn = new QPushButton(fileTag);
        QIcon closeIcon = QApplication::style()->standardIcon(QStyle::SP_TitleBarCloseButton);
        removeBtn->setIcon(closeIcon);
        removeBtn->setIconSize(QSize(16, 16));
        removeBtn->setFixedSize(24, 24);

        bool isDark = t.windowBg.lightness() < 128;
        QString removeBtnStyle;
        if (isDark) {
            removeBtnStyle =
                    "QPushButton { border: none; background: transparent; padding: 2px; }"
                    "QPushButton:hover { background: rgba(255, 59, 48, 0.2); border-radius: 12px; "
                    "}";
        } else {
            removeBtnStyle =
                    "QPushButton { border: none; background: transparent; padding: 2px; }"
                    "QPushButton:hover { background: #ffebeb; border-radius: 12px; }";
        }
        removeBtn->setStyleSheet(removeBtnStyle);
        removeBtn->setProperty("filePath", file.path);
        connect(removeBtn, &QPushButton::clicked, this, &MainWindow::onRemoveFileClicked);
        tagLayout->addWidget(removeBtn);

        // Insert before the stretch spacer
        m_fileListLayout->insertWidget(m_fileListLayout->count() - 1, fileTag);
    }

    m_fileButton->setToolTip(tr("添加文件 (%1 个待发送)").arg(files.size()));
}

void MainWindow::clearFileListDisplay() {
    // Remove all file tag widgets, preserving the stretch spacer
    while (m_fileListLayout->count() > 1) {
        QLayoutItem* item = m_fileListLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void MainWindow::onRemoveFileClicked() {
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) {
        return;
    }

    QString filePath = btn->property("filePath").toString();

    QVector<FileAttachment> files = m_fileManager->pendingFiles();
    QVector<FileAttachment> newFiles;
    for (const FileAttachment& file : files) {
        if (file.path != filePath) {
            newFiles.append(file);
        }
    }

    // Rebuild pendingFiles — FileManager lacks removeSingleFile, so clear and re-add
    m_fileManager->clearPendingFiles();
    for (const FileAttachment& file : newFiles) {
        m_fileManager->addFile(file.path);
    }

    updateFileListDisplay();
}

// ==================== Search feature implementation ====================

void MainWindow::setupSearchBar() {
    m_searchBar = new QFrame(this);
    m_searchBar->setObjectName("searchBar");
    m_searchBar->setVisible(false);

    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchBar);
    searchLayout->setContentsMargins(10, 6, 10, 6);
    searchLayout->setSpacing(8);

    m_searchInput = new QLineEdit(m_searchBar);
    m_searchInput->setObjectName("searchInput");
    m_searchInput->setPlaceholderText(tr("搜索历史消息..."));
    m_searchInput->setClearButtonEnabled(true);
    m_searchInput->setMinimumHeight(32);
    searchLayout->addWidget(m_searchInput, 1);

    m_searchResultLabel = new QLabel(m_searchBar);
    m_searchResultLabel->setObjectName("searchResultLabel");
    m_searchResultLabel->setText("");
    m_searchResultLabel->setMinimumWidth(70);
    searchLayout->addWidget(m_searchResultLabel);

    m_searchPrevBtn = new QPushButton(m_searchBar);
    m_searchPrevBtn->setObjectName("searchPrevBtn");
    m_searchPrevBtn->setText("◀");
    m_searchPrevBtn->setToolTip(tr("上一个"));
    searchLayout->addWidget(m_searchPrevBtn);

    m_searchNextBtn = new QPushButton(m_searchBar);
    m_searchNextBtn->setObjectName("searchNextBtn");
    m_searchNextBtn->setText("▶");
    m_searchNextBtn->setToolTip(tr("下一个"));
    searchLayout->addWidget(m_searchNextBtn);

    m_searchCloseBtn = new QPushButton(m_searchBar);
    m_searchCloseBtn->setObjectName("searchCloseBtn");
    m_searchCloseBtn->setText("✖");
    m_searchCloseBtn->setToolTip(tr("关闭"));
    searchLayout->addWidget(m_searchCloseBtn);

    // Connect search signals
    connect(m_searchInput, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_searchPrevBtn, &QPushButton::clicked, this, &MainWindow::onSearchPrevious);
    connect(m_searchNextBtn, &QPushButton::clicked, this, &MainWindow::onSearchNext);
    connect(m_searchCloseBtn, &QPushButton::clicked, this, &MainWindow::onSearchClose);
}

void MainWindow::updateSearchBarStyle() {
    // Style is managed by the global stylesheet; auto-updates on theme change.
    // Method reserved for future custom style adjustments.
}

void MainWindow::onSearchTriggered() {
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

void MainWindow::onSearchTextChanged() {
    QString keyword = m_searchInput->text().trimmed();

    if (keyword.isEmpty()) {
        clearHighlights();
        m_totalMatches = 0;
        m_currentMatchIndex = 0;
        m_searchResultLabel->setText("");
        return;
    }

    QTextDocument* doc = m_chatDisplay->document();
    QTextCursor cursor(doc);
    m_totalMatches = 0;

    while (!cursor.isNull()) {
        cursor = doc->find(keyword, cursor);
        if (!cursor.isNull()) {
            m_totalMatches++;
        }
    }

    m_currentMatchIndex = 0;

    highlightAllMatches();
    if (m_totalMatches > 0) {
        onSearchNext();
    }

    updateSearchResultLabel();
}

void MainWindow::onSearchNext() {
    QString keyword = m_searchInput->text().trimmed();
    if (keyword.isEmpty()) {
        return;
    }

    QTextDocument* doc = m_chatDisplay->document();
    QTextCursor cursor = m_chatDisplay->textCursor();

    // Search forward from current position
    QTextCursor found = doc->find(keyword, cursor);

    if (found.isNull()) {
        // Not found; wrap around from start
        cursor.movePosition(QTextCursor::Start);
        found = doc->find(keyword, cursor);
    }

    if (!found.isNull()) {
        m_chatDisplay->setTextCursor(found);
        m_chatDisplay->ensureCursorVisible();
        updateCurrentMatchIndex();
        highlightAllMatches();
    }
}

void MainWindow::onSearchPrevious() {
    QString keyword = m_searchInput->text().trimmed();
    if (keyword.isEmpty()) {
        return;
    }

    QTextDocument* doc = m_chatDisplay->document();
    QTextCursor cursor = m_chatDisplay->textCursor();

    // Search backward from current position
    QTextCursor found = doc->find(keyword, cursor, QTextDocument::FindBackward);

    if (found.isNull()) {
        // Not found; wrap around from end
        cursor.movePosition(QTextCursor::End);
        found = doc->find(keyword, cursor, QTextDocument::FindBackward);
    }

    if (!found.isNull()) {
        m_chatDisplay->setTextCursor(found);
        m_chatDisplay->ensureCursorVisible();
        updateCurrentMatchIndex();
        highlightAllMatches();
    }
}

void MainWindow::onSearchClose() {
    clearHighlights();
    m_searchBar->setVisible(false);
    m_inputLine->setFocus();
}

void MainWindow::highlightAllMatches() {
    QString keyword = m_searchInput->text().trimmed();
    if (keyword.isEmpty()) {
        return;
    }

    // Use extraSelections for highlight effect
    QList<QTextEdit::ExtraSelection> extraSelections;

    const AppTheme& t = AppTheme::current();
    bool isDark = t.windowBg.lightness() < 128;

    // Match highlight (yellow background)
    QColor matchColor = isDark ? QColor(255, 200, 50, 150) : QColor(255, 235, 130);
    // Current match highlight (orange, more prominent)
    QColor currentMatchColor = isDark ? QColor(255, 165, 0, 200) : QColor(255, 180, 60);

    QTextDocument* doc = m_chatDisplay->document();
    QTextCursor cursor(doc);
    int matchIndex = 0;

    while (!cursor.isNull()) {
        cursor = doc->find(keyword, cursor);
        if (!cursor.isNull()) {
            QTextEdit::ExtraSelection selection;
            selection.cursor = cursor;
            selection.format.setBackground(matchIndex == m_currentMatchIndex - 1 ? currentMatchColor
                                                                                 : matchColor);
            // Don't use FullWidthSelection — highlight only matched text
            extraSelections.append(selection);
            matchIndex++;
        }
    }

    m_chatDisplay->setExtraSelections(extraSelections);
}

void MainWindow::clearHighlights() {
    m_chatDisplay->setExtraSelections(QList<QTextEdit::ExtraSelection>());
    QTextCursor cursor = m_chatDisplay->textCursor();
    cursor.clearSelection();
    m_chatDisplay->setTextCursor(cursor);
}

void MainWindow::updateCurrentMatchIndex() {
    QString keyword = m_searchInput->text().trimmed();
    if (keyword.isEmpty() || m_totalMatches == 0) {
        return;
    }

    QTextDocument* doc = m_chatDisplay->document();
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

void MainWindow::updateSearchResultLabel() {
    if (m_totalMatches == 0) {
        m_searchResultLabel->setText(tr("无结果"));
    } else {
        m_searchResultLabel->setText(QString("%1/%2").arg(m_currentMatchIndex).arg(m_totalMatches));
    }
}

void MainWindow::appendCommandOutput(const QString& line) {
    QTextCursor cursor = m_chatDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);

    QTextCharFormat monoFormat;
    monoFormat.setFontFamilies({QStringLiteral("Menlo")});
    monoFormat.setFontPointSize(11);
    monoFormat.setForeground(QColor(QStringLiteral("#4a4a4a")));

    if (AppTheme::current().windowBg.lightness() < 128)
        monoFormat.setForeground(QColor(QStringLiteral("#a0a0a0")));

    cursor.insertText(QStringLiteral("  "), monoFormat);
    cursor.insertText(line, monoFormat);
    cursor.insertBlock();

    QScrollBar* scrollBar = m_chatDisplay->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::handleTaskResponse(const QString& response) {
    TaskEngine* engine = TaskEngine::instance();
    OperationPlan plan = engine->parsePlanFromAIResponse(response);

    if (plan.isEmpty()) {
        // Cannot parse operation plan; display as normal message
        SessionManager::instance()->addMessageToSession(m_requestSessionId, "assistant", response);
        if (m_requestSessionId == SessionManager::instance()->currentSessionId()) {
            renderCurrentSession();
        }
        return;
    }

    SafetyChecker::Result safetyResult = engine->validatePlan(plan);
    if (safetyResult == SafetyChecker::Blocked) {
        QString errMsg = tr("⚠️ 操作被安全拦截：%1").arg(engine->safetyChecker().lastBlockReason());
        SessionManager::instance()->addMessageToSession(m_requestSessionId, "assistant", errMsg);
        if (m_requestSessionId == SessionManager::instance()->currentSessionId()) {
            renderCurrentSession();
        }
        return;
    }

    OperationConfirmDialog dialog(plan, this);
    dialog.exec();

    if (dialog.isConfirmed()) {
        // User confirmed — connect CommandExecutor signals for live output
        CommandExecutor* executor = engine->executor();
        QString accumulatedOutput;

        QMetaObject::Connection connStart =
                connect(executor, &CommandExecutor::operationStarted, this,
                        [this](int index, const QString& command) {
                            Q_UNUSED(index);
                            appendCommandOutput(QStringLiteral("$ %1").arg(command));
                        });

        QMetaObject::Connection connStdout = connect(executor, &CommandExecutor::stdoutLineReceived,
                                                     this, [this](const QString& line, int index) {
                                                         Q_UNUSED(index);
                                                         appendCommandOutput(line);
                                                     });

        QMetaObject::Connection connStderr = connect(executor, &CommandExecutor::stderrLineReceived,
                                                     this, [this](const QString& line, int index) {
                                                         Q_UNUSED(index);
                                                         appendCommandOutput(line);
                                                     });

        QVector<CommandResult> results = engine->executePlan(plan);

        disconnect(connStart);
        disconnect(connStdout);
        disconnect(connStderr);

        QString resultMsg;
        int successCount = 0;
        int failCount = 0;
        for (const auto& r : results) {
            if (r.success)
                successCount++;
            else {
                failCount++;
                if (!r.errorMessage.isEmpty())
                    appendCommandOutput(QStringLiteral("  ❌ %1").arg(r.errorMessage));
            }
        }

        resultMsg = tr("✅ 命令执行完成：%1 成功").arg(successCount);
        if (failCount > 0) resultMsg += tr("，%1 失败").arg(failCount);

        if (engine->canUndo())
            resultMsg += QLatin1String("\n\n") + tr("💡 提示：可以输入「撤销刚才的操作」来恢复");

        // Append plan summary and results to chat
        QString fullMsg = plan.generateSummary() + QStringLiteral("\n\n") + resultMsg +
                          QStringLiteral("\n\n") + response;

        SessionManager::instance()->addMessageToSession(m_requestSessionId, "assistant", fullMsg);
    } else if (dialog.isModifyRequested()) {
        // User requested modification of the plan
        QString modifyMsg =
                tr("📝 请补充说明需要如何调整计划，例如：\n"
                   "  • 修改目标路径\n"
                   "  • 增加或减少操作\n"
                   "  • 添加筛选条件\n"
                   "我会根据你的反馈重新生成计划。");
        SessionManager::instance()->addMessageToSession(m_requestSessionId, "assistant", modifyMsg);
    } else {
        // User cancelled
        QString cancelMsg = tr("❌ 操作已取消。");
        SessionManager::instance()->addMessageToSession(m_requestSessionId, "assistant", cancelMsg);
    }

    if (m_requestSessionId == SessionManager::instance()->currentSessionId()) {
        renderCurrentSession();
    }
}

void MainWindow::onGirlfriendClicked() {
    static GirlfriendWindow* girlfriendWindow = nullptr;

    // If window exists and is visible, close it
    if (girlfriendWindow && girlfriendWindow->isVisible()) {
        girlfriendWindow->close();
        return;
    }

    // Otherwise create or show the window
    if (!girlfriendWindow) {
        girlfriendWindow = new GirlfriendWindow(this);
    }
    girlfriendWindow->show();
    girlfriendWindow->raise();
    girlfriendWindow->activateWindow();
}

void MainWindow::adjustInputHeight() {
    // Force synchronous layout recalculation so document()->size() is current
    m_inputLine->document()->adjustSize();

    int docHeight = static_cast<int>(m_inputLine->document()->size().height());
    int margins = m_inputLine->contentsMargins().top() + m_inputLine->contentsMargins().bottom();
    int frame = static_cast<int>(2 * m_inputLine->frameWidth());
    int contentHeight = docHeight + margins + frame;
    int singleLine = m_inputLine->fontMetrics().lineSpacing() + margins + frame + 8;

    int newHeight = qBound(singleLine, contentHeight, m_maxInputHeight);
    if (m_inputLine->height() != newHeight) {
        m_inputLine->setFixedHeight(newHeight);
    }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    adjustInputHeight();
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    // ——— IME composition handling (e.g. pinyin) ———
    // Works cross-platform: QEvent::InputMethod on Windows/Linux;
    // QEvent::InputMethodQuery is a reliable macOS fallback (the IME
    // queries cursor position to place the candidate window).
    if (obj == m_inputLine || obj == m_inputLine->viewport()) {
        if (event->type() == QEvent::InputMethod) {
            auto* imeEvent = static_cast<QInputMethodEvent*>(event);
            if (!imeEvent->preeditString().isEmpty()) {
                m_inputLine->setPlaceholderText(QString());
            } else if (m_inputLine->toPlainText().isEmpty()) {
                m_inputLine->setPlaceholderText(m_inputPlaceholder);
            }
        }
        // macOS IME may not populate preeditString; treat input-method
        // queries as evidence that composition is active.
        if (event->type() == QEvent::InputMethodQuery) {
            if (m_inputLine->toPlainText().isEmpty()) {
                m_inputLine->setPlaceholderText(QString());
            }
        }
        // Restore placeholder when leaving an empty input
        if (event->type() == QEvent::FocusOut) {
            if (m_inputLine->toPlainText().isEmpty()) {
                m_inputLine->setPlaceholderText(m_inputPlaceholder);
            }
        }

        if (event->type() == QEvent::KeyPress) {
            auto* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                // Shift+Enter: let QPlainTextEdit handle it (inserts newline)
                if (keyEvent->modifiers() & Qt::ShiftModifier) {
                    return QMainWindow::eventFilter(obj, event);
                }
                // Enter without Shift: send the message
                onSendClicked();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
