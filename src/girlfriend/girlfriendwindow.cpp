#include "girlfriendwindow.h"
#include "girlfriend_translations.h"
#include "translationmanager.h"
#include <QDebug>
#include <QScrollBar>
#include <QTimer>
#include <QRegularExpression>
#include <QMessageBox>
#include <QMenu>
#include <QEvent>
#include <QShortcut>
#include <QKeySequence>
#include <QWidgetAction>
#include <QComboBox>
#include <QButtonGroup>
#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCoreApplication>
#include <QFormLayout>
#include <QInputDialog>
#include <QApplication>
#include "../ui/stylesheetmanager.h"

// 流式思考过滤器 - 逐字符处理
// 支持三种思考标签格式: <thinking>, <reasoning>, <think>
QString GirlfriendWindow::filterThinkingFromChunk(const QString &chunk)
{
    QString output;

    // 逐字符处理
    for (int i = 0; i < chunk.length(); ++i) {
        QChar ch = chunk[i];
        m_thinkFilterBuffer += ch;

        // 检测开始标签
        // <thinking>
        if (m_thinkFilterBuffer.endsWith("<thinking>")) {
            m_inThinkBlock = true;
            m_currentThinkTag = "thinking";  // 记录当前标签类型
            m_thinkFilterBuffer = m_thinkFilterBuffer.left(m_thinkFilterBuffer.length() - 10);
            continue;
        }
        // <reasoning>
        if (m_thinkFilterBuffer.endsWith("<reasoning>")) {
            m_inThinkBlock = true;
            m_currentThinkTag = "reasoning";
            m_thinkFilterBuffer = m_thinkFilterBuffer.left(m_thinkFilterBuffer.length() - 11);
            continue;
        }
        // <think>
        if (m_thinkFilterBuffer.endsWith("<think>")) {
            m_inThinkBlock = true;
            m_currentThinkTag = "think";
            m_thinkFilterBuffer = m_thinkFilterBuffer.left(m_thinkFilterBuffer.length() - 6);
            continue;
        }

        // 检测结束标签
        // </thinking>
        if (m_currentThinkTag == "thinking" && m_thinkFilterBuffer.endsWith("</thinking>")) {
            m_inThinkBlock = false;
            m_currentThinkTag.clear();
            m_thinkFilterBuffer.clear();
            continue;
        }
        // </reasoning>
        if (m_currentThinkTag == "reasoning" && m_thinkFilterBuffer.endsWith("</reasoning>")) {
            m_inThinkBlock = false;
            m_currentThinkTag.clear();
            m_thinkFilterBuffer.clear();
            continue;
        }
        // </think>
        if (m_currentThinkTag == "think" && m_thinkFilterBuffer.endsWith("</think>")) {
            m_inThinkBlock = false;
            m_currentThinkTag.clear();
            m_thinkFilterBuffer.clear();
            continue;
        }
    }

    // 如果不在思考块内，输出缓冲区内容
    if (!m_inThinkBlock) {
        output = m_thinkFilterBuffer;
        m_thinkFilterBuffer.clear();
    }

    return output;
}

// 解析并移除思考过程标签（用于最终内容的完整过滤）
// 支持三种格式: <thinking>...</thinking>, <reasoning>...</reasoning>, <think>...</think>
static QString parseThinkingContent(const QString &content)
{
    QString pureResponse = content;

    // 匹配三种思考标签格式
    QStringList patterns = {
        R"(<thinking>.*?</thinking>)",
        R"(<reasoning>.*?</reasoning>)",
        R"(<think>.*?</think>)"
    };

    for (const QString &pattern : patterns) {
        QRegularExpression thinkingRegex(pattern, QRegularExpression::DotMatchesEverythingOption);
        pureResponse.remove(thinkingRegex);
    }

    // 移除多余的空行和空格
    pureResponse = pureResponse.trimmed();

    return pureResponse;
}

GirlfriendWindow::GirlfriendWindow(QWidget *parent)
    : QWidget(nullptr)  // 不传入 parent，使其成为独立窗口
    , m_avatarWidget(new AvatarWidget(this))
    , m_personalityEngine(new PersonalityEngine(this))
    , m_memoryManager(new MemoryManager(this))
    , m_networkManager(new NetworkManager(this))
    , m_voiceManager(new VoiceManager(this))
    , m_chatScrollArea(new QScrollArea(this))
    , m_chatContainer(new QWidget())
    , m_chatLayout(new QVBoxLayout(m_chatContainer))
    , m_inputLine(new QLineEdit(this))
    , m_sendButton(new QPushButton(GTr::sendButton(), this))
    , m_voiceButton(new QPushButton("🎤", this))
    , m_settingsButton(new QPushButton(GTr::settingsButton(), this))
    , m_settingsMenu(nullptr)
    , m_isStreaming(false)
    , m_inThinkBlock(false)
    , m_currentThinkTag("")
    , m_thinkFilterBuffer("")
    , m_streamingBubble(nullptr)
    , m_streamingTextLabel(nullptr)
    , m_overlayEmotionLabel(nullptr)
    , m_overlayMoodBarLabel(nullptr)
    , m_overlayMoodPercentLabel(nullptr)
    , m_currentOverlayEmotion("default")
    , m_currentOverlayMood(0.6)
{
    // 设置为独立顶层窗口，有标题栏和关闭按钮
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);

    setupUI();

    // Load all sessions
    GirlfriendSessionManager::instance()->loadAll();

    // === 先连接信号，再加载会话数据 ===

    // 连接情绪变化信号 - overlay标签（必须在 loadSessionMessages 之前）
    connect(m_avatarWidget, &AvatarWidget::emotionChanged,
            this, &GirlfriendWindow::onAvatarEmotionChanged);

    // 连接心情变化信号
    connect(m_personalityEngine, &PersonalityEngine::moodChanged,
            m_avatarWidget, &AvatarWidget::setMood);
    connect(m_personalityEngine, &PersonalityEngine::moodChanged,
            this, &GirlfriendWindow::onAvatarMoodChanged);

    // 心情变化时保存到会话
    connect(m_personalityEngine, &PersonalityEngine::moodChanged, this, [this](double mood) {
        GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
        if (session) {
            session->setMood(mood);
        }
    });

    // 连接设置变化信号
    connect(GirlfriendSettings::instance(), &GirlfriendSettings::avatarLevelChanged,
            this, &GirlfriendWindow::onSettingsAvatarLevelChanged);
    connect(GirlfriendSettings::instance(), &GirlfriendSettings::videoSoundChanged,
            this, &GirlfriendWindow::onSettingsVideoSoundChanged);
    connect(GirlfriendSettings::instance(), &GirlfriendSettings::voiceOutputChanged,
            this, &GirlfriendWindow::onSettingsVoiceOutputChanged);

    // === 加载会话数据（信号已连接，会自动触发 overlay 更新）===

    // Display history messages from current session
    loadSessionMessages();

    // 从会话加载心情值，同步到 PersonalityEngine
    GirlfriendSession* initialSession = GirlfriendSessionManager::instance()->currentSessionData();
    if (initialSession) {
        // 设置心情值（会触发 moodChanged 信号更新 overlay）
        double sessionMood = initialSession->mood();
        m_currentOverlayMood = sessionMood;  // 立即设置 overlay 值
        m_personalityEngine->setMood(sessionMood);

        // 设置情绪值（loadSessionMessages 已经设置了，这里确保 overlay 同步）
        QString sessionEmotion = initialSession->currentEmotion();
        m_currentOverlayEmotion = sessionEmotion;

        // 初始化时调用 updateOverlayLabels 确保 UI 正确
        updateOverlayLabels();

        qDebug() << "GirlfriendWindow: Loaded from session - mood:" << sessionMood
                 << ", emotion:" << sessionEmotion;
    }

    // === 连接其他信号 ===

    connect(m_sendButton, &QPushButton::clicked, this, &GirlfriendWindow::onSendClicked);
    connect(m_voiceButton, &QPushButton::clicked, this, &GirlfriendWindow::onVoiceClicked);
    connect(m_settingsButton, &QPushButton::clicked, this, &GirlfriendWindow::onSettingsClicked);
    connect(m_inputLine, &QLineEdit::returnPressed, this, &GirlfriendWindow::onSendClicked);

    connect(m_networkManager, &NetworkManager::streamChunkReceived, this, &GirlfriendWindow::onStreamChunkReceived);
    connect(m_networkManager, &NetworkManager::streamFinished, this, &GirlfriendWindow::onStreamFinished);
    connect(m_networkManager, &NetworkManager::errorOccurred, this, &GirlfriendWindow::onNetworkError);

    // 连接语音管理器信号
    connect(m_voiceManager, &VoiceManager::asrPartialResult, this, &GirlfriendWindow::onAsrPartialResult);
    connect(m_voiceManager, &VoiceManager::asrFinalResult, this, &GirlfriendWindow::onAsrFinalResult);
    connect(m_voiceManager, &VoiceManager::asrError, this, &GirlfriendWindow::onAsrError);
    connect(m_voiceManager, &VoiceManager::speakingStarted, this, &GirlfriendWindow::onSpeakingStarted);
    connect(m_voiceManager, &VoiceManager::speakingFinished, this, &GirlfriendWindow::onSpeakingFinished);
    connect(m_voiceManager, &VoiceManager::statusChanged, this, &GirlfriendWindow::onVoiceStatusChanged);

    // 连接主题变化信号
    connect(StyleSheetManager::instance(), &StyleSheetManager::themeChanged, this, [this](StyleSheetManager::Theme) {
        applyTheme();
    });

    // 连接语言变化信号
    connect(TranslationManager::instance(), &TranslationManager::languageChanged, this, [this]() {
        retranslateUi();
    });

    // 添加 Ctrl+G 快捷键关闭窗口（与 MainWindow 打开快捷键一致）
    QShortcut *closeShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_G), this);
    closeShortcut->setContext(Qt::WindowShortcut);  // 窗口激活时生效
    connect(closeShortcut, &QShortcut::activated, this, &QWidget::close);

    // 加载人设 Prompt
    QString personalityPrompt = m_personalityEngine->loadPersonalityPrompt();
    m_networkManager->setSystemPrompt(personalityPrompt);

    // 检查语音配置
    if (!m_voiceManager->isConfigured()) {
        qDebug() << "GirlfriendWindow: Voice credentials not configured";
        m_voiceButton->setToolTip(GTr::voiceNotConfiguredTooltip());
    } else {
        m_voiceButton->setToolTip(GTr::voiceInputTooltip());
    }

    // 设置窗口属性
    setWindowTitle(GTr::windowTitle());
    // 设置9:16比例，适合Level 2图片完整显示
    resize(360, 640);  // 9:16比例

    // 初始化主题
    applyTheme();
}

GirlfriendWindow::~GirlfriendWindow()
{
    GirlfriendSessionManager::instance()->saveAll();
}

void GirlfriendWindow::closeEvent(QCloseEvent *event)
{
    GirlfriendSessionManager::instance()->saveAll();
    event->accept();
}

void GirlfriendWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // 更新 AvatarWidget 覆盖整个窗口
    m_avatarWidget->setGeometry(0, 0, width(), height());

    // 关键：确保AvatarWidget在所有overlay之下
    m_avatarWidget->lower();

    // 更新overlay标签位置 - 确保在最上层
    if (m_overlayEmotionLabel) {
        m_overlayEmotionLabel->move(12, 12);
        m_overlayEmotionLabel->raise();
    }
    if (m_overlayMoodBarLabel) {
        int emotionLabelHeight = m_overlayEmotionLabel ? m_overlayEmotionLabel->sizeHint().height() : 24;
        m_overlayMoodBarLabel->move(12, 12 + emotionLabelHeight + 4);
        m_overlayMoodBarLabel->raise();
    }
    if (m_overlayMoodPercentLabel) {
        int emotionLabelHeight = m_overlayEmotionLabel ? m_overlayEmotionLabel->sizeHint().height() : 24;
        m_overlayMoodPercentLabel->move(12 + 54, 12 + emotionLabelHeight + 2);
        m_overlayMoodPercentLabel->raise();
    }

    // 更新设置按钮位置（右上角）- 确保在最上层
    m_settingsButton->move(width() - 40, 12);
    m_settingsButton->raise();

    // 更新底部聊天区域的位置 - 确保在最上层
    QWidget *bottomOverlay = findChild<QWidget *>("bottomOverlay");
    if (bottomOverlay) {
        int overlayHeight = 200;
        bottomOverlay->setGeometry(0, height() - overlayHeight, width(), overlayHeight);
        bottomOverlay->raise();
    }

    // 强制刷新窗口层级
    update();
}

void GirlfriendWindow::applyTheme()
{
    // Detect theme from StyleSheetManager
    StyleSheetManager::Theme theme = StyleSheetManager::instance()->currentTheme();
    bool isDark = (theme == StyleSheetManager::DarkTheme);
    // Also check system theme
    if (theme == StyleSheetManager::SystemTheme) {
        QPalette palette = QApplication::palette();
        QColor windowColor = palette.color(QPalette::Window);
        int brightness = (windowColor.red() * 299 + windowColor.green() * 587 + windowColor.blue() * 114) / 1000;
        isDark = brightness < 128;
    }
    m_isDarkTheme = isDark;

    // Derive theme colors
    QString bg         = isDark ? QStringLiteral("#1e1e1e") : QStringLiteral("#ffffff");
    QString surface    = isDark ? QStringLiteral("#2d2d2d") : QStringLiteral("#f5f5f5");
    QString text       = isDark ? QStringLiteral("#e0e0e0") : QStringLiteral("#333333");
    QString textInv    = isDark ? QStringLiteral("#333333") : QStringLiteral("#ffffff");
    QString secondary  = isDark ? QStringLiteral("#999999") : QStringLiteral("#666666");
    QString hintColor  = isDark ? QStringLiteral("#888888") : QStringLiteral("#999999");
    QString border     = isDark ? QStringLiteral("#3d3d3d") : QStringLiteral("#cccccc");
    QString hoverBg    = isDark ? QStringLiteral("#3d3d3d") : QStringLiteral("#f0f0f0");
    QString inputBg    = isDark ? QStringLiteral("rgba(233, 30, 99, 0.12)") : QStringLiteral("rgba(255, 182, 193, 0.5)");
    QString userBubble = isDark ? QStringLiteral("rgba(255, 255, 255, 0.06)") : QStringLiteral("rgba(100, 100, 100, 0.1)");
    QString gfBubble   = isDark ? QStringLiteral("rgba(233, 30, 99, 0.12)") : QStringLiteral("rgba(233, 30, 99, 0.15)");
    QString pink       = QStringLiteral("#e91e63");
    QString pinkHover  = QStringLiteral("#c2185b");
    QString pinkDarker = QStringLiteral("#d81b60");
    QString red        = QStringLiteral("#f44336");
    QString redHover   = QStringLiteral("#d32f2f");
    QString grayBg     = QStringLiteral("#9e9e9e");
    QString cancelBg   = isDark ? QStringLiteral("#555555") : QStringLiteral("#555555");
    QString cancelHov  = isDark ? QStringLiteral("#777777") : QStringLiteral("#777777");

    // Base stylesheet for the window
    setStyleSheet(QString());

    // Re-apply stylesheets to all child widgets
    // Settings button (top-right)
    if (m_settingsButton) {
        m_settingsButton->setStyleSheet(
            QStringLiteral("QPushButton { background: %1; color: %2; border: none; "
                           "font-size: 14px; border-radius: 14px; }"
                           "QPushButton:hover { background: %3; }")
                .arg(surface, text, hoverBg));
    }

    // Settings menu
    if (m_settingsMenu) {
        m_settingsMenu->setStyleSheet(
            QStringLiteral("QMenu { background: %1; color: %2; border: 1px solid %3; border-radius: 8px; }"
                           "QMenu::item { padding: 8px 20px; color: %2; }"
                           "QMenu::item:selected { background: %4; color: white; }")
                .arg(surface, text, border, pink));
    }

    // Voice button (normal state)
    // Only reset voice button style if in normal state (🎤), not recording (🔴) or waiting (⏳)
    if (m_voiceButton && m_voiceButton->text() == QStringLiteral("🎤")) {
        if (m_voiceButton) {
            m_voiceButton->setStyleSheet(
                QStringLiteral("QPushButton { background: %1; color: white; border: none; "
                               "padding: 10px 18px; border-radius: 10px; font-size: 13px; min-width: 70px; }"
                               "QPushButton:hover { background: %2; }")
                    .arg(pink, pinkHover));
        }
    }

    // Input line
    if (m_inputLine) {
        m_inputLine->setStyleSheet(
            QStringLiteral("QLineEdit { background: %1; border: none; color: %2; "
                           "padding: 10px 14px; border-radius: 10px; font-size: 13px; }")
                .arg(inputBg, text));
    }

    // Send button
    if (m_sendButton) {
        m_sendButton->setStyleSheet(
            QStringLiteral("QPushButton { background: %1; color: white; border: none; "
                           "padding: 10px 18px; border-radius: 10px; font-size: 13px; min-width: 70px; }"
                           "QPushButton:hover { background: %2; }")
                .arg(pink, pinkHover));
    }

    qDebug() << "GirlfriendWindow: Theme applied -" << (isDark ? "dark" : "light");
}

void GirlfriendWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    } else if (event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange) {
        applyTheme();
    }
    QWidget::changeEvent(event);
}

void GirlfriendWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // 窗口每次显示时重新应用主题
    applyTheme();

    // 窗口每次显示时更新 overlay 状态
    updateOverlayVisibility();

    // 确保所有 overlay widgets 都在最上层并可见
    if (m_overlayEmotionLabel) {
        m_overlayEmotionLabel->raise();
    }
    if (m_overlayMoodBarLabel) {
        m_overlayMoodBarLabel->raise();
    }
    if (m_overlayMoodPercentLabel) {
        m_overlayMoodPercentLabel->raise();
    }
    if (m_settingsButton) {
        m_settingsButton->raise();
    }
    QWidget *bottomOverlay = findChild<QWidget *>("bottomOverlay");
    if (bottomOverlay) {
        bottomOverlay->raise();
    }

    // 更新情绪和心情标签显示（从会话恢复）
    GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
    if (session) {
        m_currentOverlayEmotion = session->currentEmotion();
        m_currentOverlayMood = session->mood();
        updateOverlayLabels();
    }

    qDebug() << "GirlfriendWindow: Show event - overlay visibility updated";
}

void GirlfriendWindow::retranslateUi()
{
    // 更新窗口标题
    setWindowTitle(GTr::windowTitle());

    // 更新输入框占位符
    m_inputLine->setPlaceholderText(GTr::inputPlaceholder());

    // 更新发送按钮
    m_sendButton->setText(GTr::sendButton());

    // 更新语音按钮提示
    if (m_voiceManager->isConfigured()) {
        m_voiceButton->setToolTip(GTr::voiceInputTooltip());
    } else {
        m_voiceButton->setToolTip(GTr::voiceNotConfiguredTooltip());
    }

    // 更新左上角情绪标签（AvatarWidget内部）
    m_avatarWidget->retranslateUi();

    // 更新overlay标签（GirlfriendWindow直接子widget）
    updateOverlayLabels();

    // 更新设置菜单项（菜单会在每次点击时重建，所以这里不需要更新）

    qDebug() << "GirlfriendWindow: UI retranslated";
}

void GirlfriendWindow::setupUI()
{
    // 不使用布局，使用绝对定位叠加
    // AvatarWidget 作为整个窗口的背景（覆盖100%）
    m_avatarWidget->setParent(this);
    m_avatarWidget->setGeometry(0, 0, width(), height());

    // === Overlay labels (direct children of GirlfriendWindow, above AvatarWidget) ===
    // 情绪标签 - 粉红色背景白色文字
    m_overlayEmotionLabel = new QLabel(this);
    m_overlayEmotionLabel->setStyleSheet(
        "QLabel { background: rgba(233, 30, 99, 0.85); color: white; "
        "padding: 4px 12px; font-size: 12px; border-radius: 6px; }"
    );
    m_overlayEmotionLabel->setText(GTr::emotionDefault());
    m_overlayEmotionLabel->adjustSize();
    m_overlayEmotionLabel->move(12, 12);

    // Mood bar - 进度条
    m_overlayMoodBarLabel = new QLabel(this);
    m_overlayMoodBarLabel->setStyleSheet("QLabel { background: transparent; }");
    m_overlayMoodBarLabel->setFixedHeight(6);
    m_overlayMoodBarLabel->setFixedWidth(50);

    // Mood percentage label
    m_overlayMoodPercentLabel = new QLabel(this);
    m_overlayMoodPercentLabel->setStyleSheet(
        "QLabel { background: transparent; font-size: 10px; color: white; }"
    );

    // 初始化mood显示
    updateOverlayLabels();

    // 隐藏AvatarWidget内部的情绪/mood标签，使用GirlfriendWindow的overlay标签
    m_avatarWidget->hideInternalLabels(true);

    // 设置按钮 - 右上角
    m_settingsButton->setFixedSize(28, 28);
    m_settingsButton->setStyleSheet(
        "QPushButton { background: white; color: black; border: none; "
        "font-size: 14px; border-radius: 14px; }"
        "QPushButton:hover { background: #f0f0f0; }"
    );
    m_settingsButton->move(width() - 40, 12);
    m_settingsButton->raise();

    // 创建设置菜单 - 白色背景黑色文字
    m_settingsMenu = new QMenu(this);
    m_settingsMenu->setStyleSheet(
        "QMenu { background: white; color: black; border: 1px solid #ccc; border-radius: 8px; }"
        "QMenu::item { padding: 8px 20px; color: black; }"
        "QMenu::item:selected { background: #e91e63; color: white; }"
    );

    // 聊天区域叠加在底部
    QWidget *bottomOverlay = new QWidget(this);
    bottomOverlay->setObjectName("bottomOverlay");
    bottomOverlay->setStyleSheet("background: transparent;");
    bottomOverlay->setGeometry(0, height() - 200, width(), 200);

    QVBoxLayout *overlayLayout = new QVBoxLayout(bottomOverlay);
    overlayLayout->setContentsMargins(12, 8, 12, 8);
    overlayLayout->setSpacing(8);

    // 消息滚动区域 - 全透明背景，只有气泡包裹文字
    m_chatScrollArea->setWidget(m_chatContainer);
    m_chatScrollArea->setWidgetResizable(true);
    m_chatScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_chatScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_chatScrollArea->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { width: 6px; background: transparent; }"
    );
    m_chatContainer->setStyleSheet("background: transparent;");
    m_chatLayout->setAlignment(Qt::AlignTop);
    m_chatLayout->setSpacing(6);
    overlayLayout->addWidget(m_chatScrollArea, 1);

    // 输入区域
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(8);

    m_voiceButton->setStyleSheet(
        "QPushButton { background: #e91e63; color: white; border: none; "
        "padding: 10px 18px; border-radius: 10px; font-size: 13px; min-width: 70px; }"
        "QPushButton:hover { background: #c2185b; }"
    );
    inputLayout->addWidget(m_voiceButton);

    m_inputLine->setStyleSheet(
        "QLineEdit { background: rgba(255, 182, 193, 0.5); border: none; "  // 浅粉红色
        "padding: 10px 14px; border-radius: 10px; font-size: 13px; }"
    );
    m_inputLine->setPlaceholderText(GTr::inputPlaceholder());
    inputLayout->addWidget(m_inputLine, 1);

    m_sendButton->setStyleSheet(
        "QPushButton { background: #e91e63; color: white; border: none; "
        "padding: 10px 18px; border-radius: 10px; font-size: 13px; min-width: 70px; }"
        "QPushButton:hover { background: #c2185b; }"
    );
    inputLayout->addWidget(m_sendButton);

    overlayLayout->addLayout(inputLayout);

    // 确保层级正确：avatar 在底层，overlay 在上层
    m_avatarWidget->lower();
    bottomOverlay->raise();
}

void GirlfriendWindow::addMessageBubble(const QString &role, const QString &content)
{
    QFrame *bubble = new QFrame(m_chatContainer);

    QString style;
    if (role == "girlfriend") {
        style = m_isDarkTheme
            ? QStringLiteral("QFrame { background: rgba(233, 30, 99, 0.12); border-radius: 12px; }")
            : QStringLiteral("QFrame { background: rgba(233, 30, 99, 0.15); border-radius: 12px; }");
        bubble->setLayoutDirection(Qt::LeftToRight);
    } else {
        style = m_isDarkTheme
            ? QStringLiteral("QFrame { background: rgba(255, 255, 255, 0.06); border-radius: 12px; }")
            : QStringLiteral("QFrame { background: rgba(100, 100, 100, 0.1); border-radius: 12px; }");
        bubble->setLayoutDirection(Qt::RightToLeft);
    }
    bubble->setStyleSheet(style);

    QHBoxLayout *bubbleLayout = new QHBoxLayout(bubble);
    bubbleLayout->setContentsMargins(12, 8, 12, 8);  // 增加内边距

    QLabel *textLabel = new QLabel(content, bubble);
    textLabel->setStyleSheet("QLabel { background: transparent; font-size: 12px; }");
    textLabel->setWordWrap(true);
    textLabel->setTextFormat(Qt::PlainText);
    textLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);  // 文本对齐方式
    // 设置最小宽度确保文本可见
    textLabel->setMinimumWidth(100);
    bubbleLayout->addWidget(textLabel);

    m_chatLayout->addWidget(bubble);

    // 滚动到底部
    QTimer::singleShot(10, this, [this]() {
        m_chatScrollArea->verticalScrollBar()->setValue(
            m_chatScrollArea->verticalScrollBar()->maximum()
        );
    });
}

void GirlfriendWindow::updateStreamingBubble(const QString &content)
{
    // 如果还没有创建流式气泡，创建一个
    if (!m_streamingBubble) {
        m_streamingBubble = new QFrame(m_chatContainer);
        m_streamingBubble->setStyleSheet(
            m_isDarkTheme
                ? QStringLiteral("QFrame { background: rgba(233, 30, 99, 0.12); border-radius: 12px; }")
                : QStringLiteral("QFrame { background: rgba(233, 30, 99, 0.15); border-radius: 12px; }")
        );
        m_streamingBubble->setLayoutDirection(Qt::LeftToRight);

        QHBoxLayout *bubbleLayout = new QHBoxLayout(m_streamingBubble);
        bubbleLayout->setContentsMargins(12, 8, 12, 8);

        m_streamingTextLabel = new QLabel("", m_streamingBubble);
        m_streamingTextLabel->setStyleSheet("QLabel { background: transparent; font-size: 12px; }");
        m_streamingTextLabel->setWordWrap(true);
        m_streamingTextLabel->setTextFormat(Qt::PlainText);
        m_streamingTextLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        m_streamingTextLabel->setMinimumWidth(100);
        bubbleLayout->addWidget(m_streamingTextLabel);

        m_chatLayout->addWidget(m_streamingBubble);
    }

    // 直接显示已过滤的内容
    QString displayContent = content.trimmed();
    if (displayContent.isEmpty() && m_inThinkBlock) {
        // 正在思考阶段，显示提示
        m_streamingTextLabel->setText(GTr::thinking());
    } else if (!displayContent.isEmpty()) {
        m_streamingTextLabel->setText(displayContent);
    }

    // 滚动到底部
    QTimer::singleShot(10, this, [this]() {
        m_chatScrollArea->verticalScrollBar()->setValue(
            m_chatScrollArea->verticalScrollBar()->maximum()
        );
    });
}

void GirlfriendWindow::clearInput()
{
    m_inputLine->clear();
    m_inputLine->setFocus();
}

void GirlfriendWindow::setInputEnabled(bool enabled)
{
    m_sendButton->setEnabled(enabled);
    m_inputLine->setEnabled(enabled);
    m_voiceButton->setEnabled(enabled);

    if (enabled) {
        m_inputLine->setPlaceholderText(GTr::inputPlaceholder());
        m_sendButton->setText(GTr::sendButton());
    } else {
        m_inputLine->setPlaceholderText(GTr::replyingPlaceholder());
        m_sendButton->setText(GTr::waitingButton());
    }
}

void GirlfriendWindow::updateAvatarEmotion(const QString &text)
{
    QString emotion = m_personalityEngine->detectEmotion(text);
    m_avatarWidget->setEmotion(emotion);
    GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
    if (session) {
        session->setCurrentEmotion(emotion);
    }
}

void GirlfriendWindow::onSendClicked()
{
    QString userInput = m_inputLine->text().trimmed();
    if (userInput.isEmpty()) {
        return;
    }

    GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
    if (!session) {
        return;
    }

    // 更新心情值
    m_personalityEngine->updateMood(userInput);

    // 重置空闲情绪轮换计时器
    m_avatarWidget->resetIdleTimer();

    // 重新构建系统提示（包含更新后的心情、时间感知和用户记忆）
    QString memoryContent = m_memoryManager->getMemoryContent();
    // 仅当 memory.md 包含实际记录条目（"- xxx"）时才注入, 避免把空白模板发给 AI
    if (!memoryContent.contains(QStringLiteral("- ")))
        memoryContent.clear();
    QString systemPrompt = m_personalityEngine->buildSystemPrompt(memoryContent);
    m_networkManager->setSystemPrompt(systemPrompt);

    // 显示用户消息
    addMessageBubble("user", userInput);
    session->addMessage("user", userInput, "default");

    clearInput();
    setInputEnabled(false);

    // 构建请求消息 - 只包含历史消息，system prompt 由 NetworkManager 处理
    QVector<ChatMessage> messages;

    // 添加历史消息
    for (const GirlfriendMessage &gfMsg : session->messages()) {
        QString role = (gfMsg.role == "user") ? "user" : "assistant";
        messages.append(ChatMessage(role, gfMsg.content));
    }

    m_isStreaming = true;
    m_streamingContent.clear();
    m_inThinkBlock = false;       // 重置思考块状态
    m_currentThinkTag.clear();   // 重置当前标签类型
    m_thinkFilterBuffer.clear();  // 重置过滤器缓冲区
    m_streamingBubble = nullptr;  // 清除旧的流式气泡
    m_streamingTextLabel = nullptr;

    // 锁定状态并设置 speaking 模式 - 从发送到 TTS 播放完成全程保持
    m_avatarWidget->lockState();
    m_avatarWidget->setSpeaking(true);
    m_avatarWidget->setEmotion("speaking");  // 立即显示 speaking 表情

    // 发送请求
    m_networkManager->sendChatRequestWithContext(messages);
}

void GirlfriendWindow::onVoiceClicked()
{
    if (!m_voiceManager->isConfigured()) {
        return;
    }

    if (m_voiceManager->isRecording()) {
        // 停止录音
        m_voiceManager->stopRecording();
        m_voiceButton->setText("⏳");
        m_voiceButton->setStyleSheet(
            "QPushButton { background: #9e9e9e; color: white; border: none; "
            "padding: 10px 18px; border-radius: 10px; font-size: 13px; min-width: 70px; }"
        );
    } else {
        // 开始录音
        m_voiceManager->startRecording();
        m_voiceButton->setText("🔴");
        m_voiceButton->setStyleSheet(
            "QPushButton { background: #f44336; color: white; border: none; "
            "padding: 10px 18px; border-radius: 10px; font-size: 13px; min-width: 70px; }"
            "QPushButton:hover { background: #d32f2f; }"
        );
    }
}

void GirlfriendWindow::onSettingsClicked()
{
    // Clear menu
    m_settingsMenu->clear();

    // === Sessions Section ===
    // Sessions label (disabled action for visual separation)
    QAction *sessionsLabelAction = m_settingsMenu->addAction(GTr::sessionsLabel());
    sessionsLabelAction->setEnabled(false);
    QFont labelFont = sessionsLabelAction->font();
    labelFont.setBold(true);
    sessionsLabelAction->setFont(labelFont);

    // Session dropdown using QWidgetAction
    QWidgetAction *sessionComboAction = new QWidgetAction(m_settingsMenu);
    QComboBox *sessionComboBox = new QComboBox(m_settingsMenu);
    sessionComboBox->setStyleSheet(
        QStringLiteral("QComboBox { background: %1; color: %2; border: 1px solid %3; "
        "padding: 4px 8px; border-radius: 4px; min-width: 150px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox::down-arrow { image: none; border-left: 4px solid transparent; "
        "border-right: 4px solid transparent; border-top: 6px solid %4; margin-right: 8px; }"
        "QComboBox QAbstractItemView { background: %1; color: %2; selection-background-color: #e91e63; }")
            .arg(m_isDarkTheme ? QStringLiteral("#2d2d2d") : QStringLiteral("white"),
                 m_isDarkTheme ? QStringLiteral("#e0e0e0") : QStringLiteral("black"),
                 m_isDarkTheme ? QStringLiteral("#3d3d3d") : QStringLiteral("#cccccc"),
                 m_isDarkTheme ? QStringLiteral("#999999") : QStringLiteral("#666666")));

    // Populate session dropdown
    QVector<SessionMetadata> sessions = GirlfriendSessionManager::instance()->sessions();
    QString currentSessionId = GirlfriendSessionManager::instance()->currentSessionId();
    int currentIndex = 0;
    for (int i = 0; i < sessions.size(); ++i) {
        QString displayName = sessions[i].name;
        if (sessions[i].id == currentSessionId) {
            displayName += " (" + GTr::currentSessionLabel() + ")";
            currentIndex = i;
        }
        sessionComboBox->addItem(displayName, sessions[i].id);
    }
    sessionComboBox->setCurrentIndex(currentIndex);
    connect(sessionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GirlfriendWindow::onSessionChanged);
    sessionComboAction->setDefaultWidget(sessionComboBox);
    m_settingsMenu->addAction(sessionComboAction);

    // New Session action
    QAction *newSessionAction = m_settingsMenu->addAction("+ " + GTr::newSession());
    connect(newSessionAction, &QAction::triggered, this, &GirlfriendWindow::onNewSessionClicked);

    // Manage Conversations button
    {
        QWidgetAction *manageAction = new QWidgetAction(m_settingsMenu);
        QPushButton *manageBtn = new QPushButton(GTr::manageConversations(), m_settingsMenu);
        manageBtn->setStyleSheet(
            "QPushButton { background: #e91e63; color: white; padding: 4px 8px; "
            "border-radius: 4px; font-size: 11px; border: none; }"
            "QPushButton:hover { background: #d81b60; }"
        );
        connect(manageBtn, &QPushButton::clicked, this, &GirlfriendWindow::onManageConversations);
        manageAction->setDefaultWidget(manageBtn);
        m_settingsMenu->addAction(manageAction);
    }

    m_settingsMenu->addSeparator();

    // === Avatar Level Section ===
    QAction *avatarLevelLabel = m_settingsMenu->addAction(GTr::avatarLevelLabel());
    avatarLevelLabel->setEnabled(false);
    avatarLevelLabel->setFont(labelFont);

    // Avatar level buttons using QWidgetAction with exclusive selection
    QWidgetAction *avatarLevelAction = new QWidgetAction(m_settingsMenu);
    QWidget *avatarLevelWidget = new QWidget(m_settingsMenu);
    QHBoxLayout *avatarLevelLayout = new QHBoxLayout(avatarLevelWidget);
    avatarLevelLayout->setContentsMargins(8, 4, 8, 4);
    avatarLevelLayout->setSpacing(4);

    // Use QButtonGroup for exclusive selection
    QButtonGroup *avatarLevelGroup = new QButtonGroup(avatarLevelWidget);
    avatarLevelGroup->setExclusive(true);

    AvatarLevel currentLevel = GirlfriendSettings::instance()->avatarLevel();
    for (int i = 1; i <= 3; ++i) {
        QPushButton *levelBtn = new QPushButton(QString::number(i), avatarLevelWidget);
        levelBtn->setCheckable(true);
        levelBtn->setChecked(static_cast<int>(currentLevel) == i - 1);
        levelBtn->setFixedSize(40, 28);
        levelBtn->setStyleSheet(
            QStringLiteral("QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: 4px; }"
            "QPushButton:checked { background: #e91e63; color: white; border: 1px solid #e91e63; }"
            "QPushButton:hover { background: %4; }")
                .arg(m_isDarkTheme ? QStringLiteral("#2d2d2d") : QStringLiteral("white"),
                     m_isDarkTheme ? QStringLiteral("#e0e0e0") : QStringLiteral("black"),
                     m_isDarkTheme ? QStringLiteral("#3d3d3d") : QStringLiteral("#cccccc"),
                     m_isDarkTheme ? QStringLiteral("#3d3d3d") : QStringLiteral("#f8bbd9")));
        avatarLevelGroup->addButton(levelBtn, i);
        connect(levelBtn, &QPushButton::clicked, this, [this, i, levelBtn]() {
            levelBtn->setChecked(true);  // Ensure this button is checked
            onAvatarLevelChanged(i);
            m_settingsMenu->close();  // Close menu to prevent multi-select
        });
        avatarLevelLayout->addWidget(levelBtn);
    }
    avatarLevelLayout->addStretch();
    avatarLevelAction->setDefaultWidget(avatarLevelWidget);
    m_settingsMenu->addAction(avatarLevelAction);

    m_settingsMenu->addSeparator();

    // === Mood Influence Section ===
    QAction *moodInfluenceLabel = m_settingsMenu->addAction(GTr::moodInfluenceLabel());
    moodInfluenceLabel->setEnabled(false);
    moodInfluenceLabel->setFont(labelFont);

    // Mood influence buttons using QWidgetAction with exclusive selection
    QWidgetAction *moodInfluenceAction = new QWidgetAction(m_settingsMenu);
    QWidget *moodInfluenceWidget = new QWidget(m_settingsMenu);
    QHBoxLayout *moodInfluenceLayout = new QHBoxLayout(moodInfluenceWidget);
    moodInfluenceLayout->setContentsMargins(8, 4, 8, 4);
    moodInfluenceLayout->setSpacing(4);

    // Use QButtonGroup for exclusive selection
    QButtonGroup *moodInfluenceGroup = new QButtonGroup(moodInfluenceWidget);
    moodInfluenceGroup->setExclusive(true);

    MoodInfluenceLevel currentMood = GirlfriendSettings::instance()->moodInfluence();
    QStringList moodLabels = {GTr::moodLow(), GTr::moodMedium(), GTr::moodHigh()};
    for (int i = 0; i < 3; ++i) {
        QPushButton *moodBtn = new QPushButton(moodLabels[i], moodInfluenceWidget);
        moodBtn->setCheckable(true);
        moodBtn->setChecked(static_cast<int>(currentMood) == i);
        moodBtn->setFixedSize(50, 28);
        moodBtn->setStyleSheet(
            QStringLiteral("QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: 4px; }"
            "QPushButton:checked { background: #e91e63; color: white; border: 1px solid #e91e63; }"
            "QPushButton:hover { background: %4; }")
                .arg(m_isDarkTheme ? QStringLiteral("#2d2d2d") : QStringLiteral("white"),
                     m_isDarkTheme ? QStringLiteral("#e0e0e0") : QStringLiteral("black"),
                     m_isDarkTheme ? QStringLiteral("#3d3d3d") : QStringLiteral("#cccccc"),
                     m_isDarkTheme ? QStringLiteral("#3d3d3d") : QStringLiteral("#f8bbd9")));
        moodInfluenceGroup->addButton(moodBtn, i);
        connect(moodBtn, &QPushButton::clicked, this, [this, i, moodBtn]() {
            moodBtn->setChecked(true);  // Ensure this button is checked
            onMoodInfluenceChanged(i);
            m_settingsMenu->close();  // Close menu to prevent multi-select
        });
        moodInfluenceLayout->addWidget(moodBtn);
    }
    moodInfluenceLayout->addStretch();
    moodInfluenceAction->setDefaultWidget(moodInfluenceWidget);
    m_settingsMenu->addAction(moodInfluenceAction);

    m_settingsMenu->addSeparator();

    // === Video Sound Toggle (only for Level 3) ===
    if (GirlfriendSettings::instance()->avatarLevel() == AvatarLevel::Level3_Hotter) {
        QString videoSoundText = GTr::videoSoundLabel() + ": " +
            (GirlfriendSettings::instance()->videoSoundEnabled() ? GTr::videoSoundOn() : GTr::videoSoundOff());
        QAction *videoSoundAction = m_settingsMenu->addAction(videoSoundText);
        connect(videoSoundAction, &QAction::triggered, this, &GirlfriendWindow::onVideoSoundToggled);

        m_settingsMenu->addSeparator();
    }

    // === Voice Output Toggle ===
    QString voiceOutputText = (GirlfriendSettings::instance()->voiceOutputEnabled() ?
        GTr::voiceOutputEnabled() : GTr::voiceOutputDisabled());
    QAction *voiceOutputAction = m_settingsMenu->addAction(voiceOutputText);
    connect(voiceOutputAction, &QAction::triggered, this, &GirlfriendWindow::onToggleVoiceOutput);

    // === Configure Voice ===
    QAction *voiceConfigAction = m_settingsMenu->addAction(GTr::configureVoice());
    connect(voiceConfigAction, &QAction::triggered, this, &GirlfriendWindow::showVoiceConfigDialog);

    m_settingsMenu->addSeparator();

    // === Clear History ===
    QAction *clearAction = m_settingsMenu->addAction(GTr::clearHistory());
    connect(clearAction, &QAction::triggered, this, &GirlfriendWindow::onClearClicked);

    // Show menu
    m_settingsMenu->exec(m_settingsButton->mapToGlobal(QPoint(0, m_settingsButton->height())));
}

void GirlfriendWindow::onToggleVoiceOutput()
{
    bool enabled = !GirlfriendSettings::instance()->voiceOutputEnabled();
    GirlfriendSettings::instance()->setVoiceOutputEnabled(enabled);
    qDebug() << "GirlfriendWindow: Voice output toggled to" << enabled;

    // 如果关闭语音输出且正在播放，立即停止并解锁状态
    if (!enabled && m_voiceManager->isSpeaking()) {
        m_voiceManager->stopSpeaking();
        m_avatarWidget->unlockState();
        qDebug() << "GirlfriendWindow: Stopped voice and unlocked state";
    }
}

void GirlfriendWindow::onClearClicked()
{
    // 弹出确认对话框
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        GTr::clearConfirmTitle(),
        GTr::clearConfirmMessage(),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        // 清空历史前停止语音并解锁状态
        if (m_voiceManager->isSpeaking()) {
            m_voiceManager->stopSpeaking();
        }
        if (m_isStreaming) {
            m_isStreaming = false;
        }
        m_avatarWidget->unlockState();

        GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
        if (session) {
            // 清空会话历史
            session->clearMessages();
            // 重置心情值为默认值 60%
            session->setMood(0.6);
            session->setCurrentEmotion("default");
            GirlfriendSessionManager::instance()->saveAll();
        }

        // 清空界面上的消息气泡
        clearChatUI();

        // 重置情绪和心情状态
        m_avatarWidget->resetIdleTimer();
        m_avatarWidget->setEmotion("default");
        m_personalityEngine->setMood(0.6);
        m_avatarWidget->setMood(0.6);
        m_currentOverlayEmotion = "default";
        m_currentOverlayMood = 0.6;
        updateOverlayLabels();

        qDebug() << "GirlfriendWindow: History cleared, emotion and mood reset to default";
    }
}

void GirlfriendWindow::showVoiceConfigDialog()
{
    GirlfriendSettings *gs = GirlfriendSettings::instance();

    QDialog dialog(this);
    dialog.setWindowTitle(GTr::voiceConfigTitle());
    dialog.setMinimumWidth(420);
    dialog.setStyleSheet(QStringLiteral(
        "QDialog { background: %1; }"
        "QLabel { color: %2; }"
        "QLineEdit { background: %3; color: %2; border: 1px solid %4; padding: 6px; border-radius: 4px; }")
        .arg(m_isDarkTheme ? QStringLiteral("#1e1e1e") : QStringLiteral("#ffffff"),
             m_isDarkTheme ? QStringLiteral("#e0e0e0") : QStringLiteral("#333333"),
             m_isDarkTheme ? QStringLiteral("#2d2d2d") : QStringLiteral("#ffffff"),
             m_isDarkTheme ? QStringLiteral("#3d3d3d") : QStringLiteral("#cccccc")));

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setSpacing(12);

    // Description
    QLabel *descLabel = new QLabel(GTr::voiceConfigDescription(), &dialog);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(QStringLiteral("QLabel { color: %1; font-size: 11px; }")
        .arg(m_isDarkTheme ? QStringLiteral("#999999") : QStringLiteral("#666666")));
    mainLayout->addWidget(descLabel);

    // Form
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(8);
    formLayout->setContentsMargins(0, 8, 0, 8);

    QLineEdit *appIdEdit = new QLineEdit(gs->xfyunAppId(), &dialog);
    formLayout->addRow(GTr::voiceConfigAppId() + ":", appIdEdit);

    QLineEdit *apiKeyEdit = new QLineEdit(gs->xfyunApiKey(), &dialog);
    formLayout->addRow(GTr::voiceConfigApiKey() + ":", apiKeyEdit);

    QLineEdit *apiSecretEdit = new QLineEdit(gs->xfyunApiSecret(), &dialog);
    formLayout->addRow(GTr::voiceConfigApiSecret() + ":", apiSecretEdit);

    QLineEdit *asrUrlEdit = new QLineEdit(gs->xfyunAsrUrl(), &dialog);
    formLayout->addRow(GTr::voiceConfigAsrUrl() + ":", asrUrlEdit);

    QLineEdit *ttsUrlEdit = new QLineEdit(gs->xfyunTtsUrl(), &dialog);
    formLayout->addRow(GTr::voiceConfigTtsUrl() + ":", ttsUrlEdit);

    QLineEdit *voiceTypeEdit = new QLineEdit(gs->xfyunVoiceType(), &dialog);
    formLayout->addRow(GTr::voiceConfigVoiceType() + ":", voiceTypeEdit);

    mainLayout->addLayout(formLayout);

    // Optional fields hint
    QLabel *optionalHint = new QLabel(GTr::voiceConfigOptionalHint(), &dialog);
    optionalHint->setWordWrap(true);
    optionalHint->setStyleSheet(QStringLiteral("QLabel { color: %1; font-size: 10px; font-style: italic; }")
        .arg(m_isDarkTheme ? QStringLiteral("#888888") : QStringLiteral("#999999")));
    mainLayout->addWidget(optionalHint);

    // Restart hint
    QLabel *hintLabel = new QLabel(GTr::voiceConfigTestHint(), &dialog);
    hintLabel->setWordWrap(true);
    hintLabel->setStyleSheet(QStringLiteral("QLabel { color: %1; font-size: 10px; font-style: italic; }")
        .arg(m_isDarkTheme ? QStringLiteral("#888888") : QStringLiteral("#999999")));
    mainLayout->addWidget(hintLabel);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *cancelBtn = new QPushButton(GTr::voiceConfigCancel(), &dialog);
    cancelBtn->setStyleSheet("QPushButton { background: #555; color: white; border: none; "
        "padding: 8px 20px; border-radius: 6px; font-size: 12px; }"
        "QPushButton:hover { background: #777; }");
    cancelBtn->setMinimumWidth(80);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    buttonLayout->addWidget(cancelBtn);

    QPushButton *saveBtn = new QPushButton(GTr::voiceConfigSave(), &dialog);
    saveBtn->setStyleSheet("QPushButton { background: #e91e63; color: white; border: none; "
        "padding: 8px 20px; border-radius: 6px; font-size: 12px; }"
        "QPushButton:hover { background: #c2185b; }");
    saveBtn->setMinimumWidth(80);
    connect(saveBtn, &QPushButton::clicked, &dialog, [&]() {
        QString appId = appIdEdit->text().trimmed();
        QString apiKey = apiKeyEdit->text().trimmed();
        QString apiSecret = apiSecretEdit->text().trimmed();

        if (appId.isEmpty() || apiKey.isEmpty() || apiSecret.isEmpty()) {
            QMessageBox::warning(&dialog, GTr::voiceConfigTitle(), GTr::voiceConfigMissingFields());
            return;
        }

        gs->setXfyunAppId(appId);
        gs->setXfyunApiKey(apiKey);
        gs->setXfyunApiSecret(apiSecret);
        gs->setXfyunAsrUrl(asrUrlEdit->text().trimmed());
        gs->setXfyunTtsUrl(ttsUrlEdit->text().trimmed());
        gs->setXfyunVoiceType(voiceTypeEdit->text().trimmed());

        // Reload voice config
        m_voiceManager->loadConfig();

        QMessageBox::information(&dialog, GTr::voiceConfigTitle(), GTr::voiceConfigSaved());
        dialog.accept();
    });
    buttonLayout->addWidget(saveBtn);

    mainLayout->addLayout(buttonLayout);

    dialog.exec();
}

void GirlfriendWindow::onStreamChunkReceived(const QString &chunk)
{
    if (!m_isStreaming) {
        return;
    }

    // 使用过滤器处理思考标签
    QString filteredChunk = filterThinkingFromChunk(chunk);
    m_streamingContent += filteredChunk;

    // 始终更新显示（包括思考阶段）
    updateStreamingBubble(m_streamingContent);

    // 实时情绪检测（基于已过滤的内容）
    if (!m_streamingContent.isEmpty()) {
        updateAvatarEmotion(m_streamingContent);
    }
}

void GirlfriendWindow::onStreamFinished(const QString &fullContent)
{
    if (!m_isStreaming) {
        return;
    }

    GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
    if (!session) {
        m_isStreaming = false;
        m_avatarWidget->setSpeaking(false);
        setInputEnabled(true);
        return;
    }

    m_isStreaming = false;

    // 过滤思考过程
    QString pureContent = parseThinkingContent(fullContent);

    // 移除流式气泡，添加最终消息气泡
    if (m_streamingBubble) {
        m_chatLayout->removeWidget(m_streamingBubble);
        m_streamingBubble->deleteLater();
        m_streamingBubble = nullptr;
        m_streamingTextLabel = nullptr;
    }

    // 解析情绪标记，获取情绪和清理后的文本
    PersonalityEngine::EmotionResult emotionResult = m_personalityEngine->parseEmotionFromResponse(pureContent);

    // Fallback: 解析记忆更新标记（如果 AI 没用工具调用，用正则解析）
    QList<MemoryManager::MemoryUpdate> memoryUpdates = m_memoryManager->parseMemoryUpdates(emotionResult.cleanText);

    // 从显示文本中移除记忆更新标记
    QString displayText = emotionResult.cleanText;
    QRegularExpression memoryRegex(R"(\[(?:更新记忆|memory):[^\]]+\])");
    displayText.remove(memoryRegex);
    displayText = displayText.trimmed();

    // 显示女友回复
    addMessageBubble("girlfriend", displayText);
    session->addMessage("girlfriend", displayText, emotionResult.emotion);

    // 自动命名：如果会话尚未自动命名，根据用户第一条消息生成标题
    QString currentId = GirlfriendSessionManager::instance()->currentSessionId();
    const auto &sessions = GirlfriendSessionManager::instance()->sessions();
    for (const auto &meta : sessions) {
        if (meta.id == currentId && !meta.autoNamed && session->messages().size() >= 2) {
            QString firstUserMsg;
            for (const auto &msg : session->messages()) {
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
                GirlfriendSessionManager::instance()->renameSession(currentId, title);
                GirlfriendSessionManager::instance()->markSessionAutoNamed(currentId);
            }
            break;
        }
    }

    // 保存回复文本用于TTS
    m_lastReplyText = displayText;

    // Fallback: 应用记忆更新（正则解析方式）
    if (!memoryUpdates.isEmpty()) {
        m_memoryManager->applyMemoryUpdates(memoryUpdates);
        qDebug() << "GirlfriendWindow: Applied memory updates via fallback (regex)";
    }

    // 更新表情（暂存，等 TTS 结束后应用）
    m_avatarWidget->setEmotion(emotionResult.emotion);
    m_avatarWidget->setSpeaking(false);  // 流式输出结束，但 TTS 可能还在播放

    setInputEnabled(true);
    GirlfriendSessionManager::instance()->saveAll();

    // 如果启用语音输出，播报回复
    if (GirlfriendSettings::instance()->voiceOutputEnabled() && m_voiceManager->isConfigured() && !displayText.isEmpty()) {
        // 清理文本：移除所有特殊标记，只保留纯净文本给TTS
        QString ttsText = displayText;

        // 移除情绪标记 [情绪:xxx] 并替换为空格（保持语义分隔）
        QRegularExpression emotionRegex(R"(\[(?:情绪|emotion):[^\]]+\])");
        ttsText.replace(emotionRegex, " ");

        // 移除动作词 (xxx) 和 （xxx） 并替换为空格（支持英文和中文括号，处理嵌套）
        QRegularExpression actionRegex(R"(\([^)]*\)|（[^）]*）)");
        // 循环替换处理嵌套括号，如 (微笑(开心))
        while (actionRegex.match(ttsText).hasMatch()) {
            ttsText.replace(actionRegex, " ");
        }

        // 移除记忆更新标记 [更新记忆:xxx]
        QRegularExpression memoryRegex(R"(\[(?:更新记忆|memory):[^\]]+\])");
        ttsText.replace(memoryRegex, " ");

        // 移除思考标记内容
        QRegularExpression thinkRegex(R"(<thinking>.*?</thinking>|<reasoning>.*?</reasoning>)");
        thinkRegex.setPatternOptions(QRegularExpression::DotMatchesEverythingOption);
        ttsText.replace(thinkRegex, " ");

        // 移除特殊符号，只保留中文、英文、数字、基本标点
        QString cleanText;
        for (const QChar &c : ttsText) {
            ushort unicode = c.unicode();
            bool keep = false;

            // 中文
            if (unicode >= 0x4E00 && unicode <= 0x9FFF) {
                keep = true;
            }
            // 英文
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                keep = true;
            }
            // 数字
            if (c >= '0' && c <= '9') {
                keep = true;
            }
            // 英文标点：逗号、句号、问号、感叹号
            if (c == ',' || c == '.' || c == '?' || c == '!') {
                keep = true;
            }
            // 中文标点：逗号(U+FF0C)、句号(3002)、问号(FF1F)、感叹号(FF01)
            if (unicode == 0xFF0C || unicode == 0x3002 || unicode == 0xFF1F || unicode == 0xFF01) {
                keep = true;
            }
            // 空格（保持分隔）
            if (c == ' ') {
                keep = true;
            }

            if (keep) {
                cleanText += c;
            }
        }

        // 清理多余空格
        cleanText = cleanText.simplified();

        qDebug() << "GirlfriendWindow: TTS clean text:" << cleanText;

        if (!cleanText.isEmpty()) {
            // TTS 播放期间保持 speaking 状态
            m_avatarWidget->setSpeaking(true);
            m_avatarWidget->setEmotion("speaking");
            m_voiceManager->speak(cleanText);
        } else {
            // 无 TTS 内容，直接解锁并恢复
            m_avatarWidget->unlockState();
        }
    } else {
        // 语音输出未启用，直接解锁并恢复
        m_avatarWidget->unlockState();
    }
}

void GirlfriendWindow::onNetworkError(const QString &error)
{
    m_isStreaming = false;
    m_avatarWidget->setSpeaking(false);
    m_avatarWidget->unlockState();  // 解锁状态

    // 移除流式气泡
    if (m_streamingBubble) {
        m_chatLayout->removeWidget(m_streamingBubble);
        m_streamingBubble->deleteLater();
        m_streamingBubble = nullptr;
        m_streamingTextLabel = nullptr;
    }

    addMessageBubble("girlfriend", GTr::errorPrefix() + error);
    setInputEnabled(true);
}

// ==================== 语音相关槽函数 ====================

void GirlfriendWindow::onAsrPartialResult(const QString &text)
{
    // 实时显示识别中的文本
    m_inputLine->setText(text);
}

void GirlfriendWindow::onAsrFinalResult(const QString &text)
{
    // ASR完成，自动发送消息
    m_inputLine->setText(text);

    // 重置语音按钮状态
    m_voiceButton->setText("🎤");
    m_voiceButton->setStyleSheet(
        "QPushButton { background: #e91e63; color: white; border: none; "
        "padding: 10px 18px; border-radius: 10px; font-size: 13px; min-width: 70px; }"
        "QPushButton:hover { background: #c2185b; }"
    );

    // 自动发送识别结果
    if (!text.isEmpty()) {
        QTimer::singleShot(100, this, &GirlfriendWindow::onSendClicked);
    }
}

void GirlfriendWindow::onAsrError(const QString &error)
{
    // 重置语音按钮状态
    m_voiceButton->setText("🎤");
    m_voiceButton->setStyleSheet(
        "QPushButton { background: #e91e63; color: white; border: none; "
        "padding: 10px 18px; border-radius: 10px; font-size: 13px; min-width: 70px; }"
        "QPushButton:hover { background: #c2185b; }"
    );
}

void GirlfriendWindow::onSpeakingStarted()
{
    // TTS 开始播放 - 状态已锁定，speaking 已设置
    // 这里只需要确保 speaking 状态（防止意外解除）
    m_avatarWidget->setSpeaking(true);
    qDebug() << "GirlfriendWindow: TTS speaking started";
}

void GirlfriendWindow::onSpeakingFinished()
{
    // TTS播放结束，解除 speaking 状态
    m_avatarWidget->setSpeaking(false);

    // 只有在当前等级下才解锁并恢复情绪
    // 如果等级已经切换，情绪已在 onAvatarLevelChanged 中恢复
    AvatarLevel currentSettingsLevel = GirlfriendSettings::instance()->avatarLevel();
    AvatarLevel currentWidgetLevel = m_avatarWidget->currentLevel();

    if (currentSettingsLevel == currentWidgetLevel) {
        // 等级未切换，正常解锁并恢复情绪
        m_avatarWidget->unlockState();

        // 获取会话中保存的情绪并恢复
        GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
        if (session) {
            QString sessionEmotion = session->currentEmotion();
            if (!sessionEmotion.isEmpty() && sessionEmotion != "speaking") {
                // 设置回会话中保存的情绪（如果不同于当前）
                m_avatarWidget->setEmotion(sessionEmotion);
                qDebug() << "GirlfriendWindow: TTS finished, emotion restored to:" << sessionEmotion;
            }
        }
    } else {
        // 等级已切换，情绪已在切换时恢复，只解锁
        m_avatarWidget->unlockState();
        qDebug() << "GirlfriendWindow: TTS finished after level change, state unlocked";
    }
}

void GirlfriendWindow::onVoiceStatusChanged(const QString &status)
{
    // 状态变化，不做UI显示
}

// ==================== Settings Menu Slots ====================

void GirlfriendWindow::onSessionChanged(int index)
{
    QComboBox *comboBox = qobject_cast<QComboBox*>(sender());
    if (!comboBox) return;

    QString sessionId = comboBox->itemData(index).toString();
    QString currentId = GirlfriendSessionManager::instance()->currentSessionId();

    if (sessionId != currentId) {
        // 切换会话前停止语音并解锁状态
        if (m_voiceManager->isSpeaking()) {
            m_voiceManager->stopSpeaking();
        }
        if (m_isStreaming) {
            m_isStreaming = false;
        }
        m_avatarWidget->unlockState();

        // Save current session before switching
        GirlfriendSessionManager::instance()->saveAll();

        // Switch to new session
        if (GirlfriendSessionManager::instance()->switchSession(sessionId)) {
            // Clear chat UI and load new session messages
            clearChatUI();
            loadSessionMessages();

            // Update avatar emotion and mood from new session
            GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
            if (session) {
                m_avatarWidget->resetIdleTimer();
                m_avatarWidget->setEmotion(session->currentEmotion());
                // 加载新会话的心情值
                double sessionMood = session->mood();
                m_personalityEngine->setMood(sessionMood);
                m_avatarWidget->setMood(sessionMood);
                m_currentOverlayMood = sessionMood;
                qDebug() << "GirlfriendWindow: Loaded mood from new session:" << sessionMood;
            }

            qDebug() << "GirlfriendWindow: Switched to session" << sessionId;
        }
    }
}

void GirlfriendWindow::onNewSessionClicked()
{
    // 创建新会话前停止语音并解锁状态
    if (m_voiceManager->isSpeaking()) {
        m_voiceManager->stopSpeaking();
    }
    if (m_isStreaming) {
        m_isStreaming = false;
    }
    m_avatarWidget->unlockState();

    // Create new session
    QString newSessionId = GirlfriendSessionManager::instance()->createNewSession();

    // Save current session and switch to new one
    GirlfriendSessionManager::instance()->saveAll();
    GirlfriendSessionManager::instance()->switchSession(newSessionId);

    // Clear chat UI and reset emotion and mood
    clearChatUI();
    m_avatarWidget->setEmotion("default");
    m_currentOverlayEmotion = "default";
    // 新会话使用默认心情值（resetMood 会触发 moodChanged 信号更新 UI）
    m_personalityEngine->resetMood();
    updateOverlayLabels();

    qDebug() << "GirlfriendWindow: Created and switched to new session" << newSessionId;
}

void GirlfriendWindow::onManageConversations()
{
    QVector<SessionMetadata> sessions = GirlfriendSessionManager::instance()->sessions();
    QString currentId = GirlfriendSessionManager::instance()->currentSessionId();

    // Sort: pinned first, then by name
    std::sort(sessions.begin(), sessions.end(), [](const SessionMetadata &a, const SessionMetadata &b) {
        if (a.pinned != b.pinned) return a.pinned > b.pinned;
        return a.name.toLower() < b.name.toLower();
    });

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(GTr::manageConversations());
    dialog->setMinimumWidth(380);
    dialog->setMinimumHeight(300);
    dialog->setStyleSheet(QStringLiteral(
        "QDialog { background: %1; }"
        "QLabel { color: %2; }"
        "QListWidget { background: %3; color: %2; border: 1px solid %4; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: %5; }"
        "QPushButton { color: %2; }")
        .arg(m_isDarkTheme ? QStringLiteral("#1e1e1e") : QStringLiteral("#ffffff"),
             m_isDarkTheme ? QStringLiteral("#e0e0e0") : QStringLiteral("#333333"),
             m_isDarkTheme ? QStringLiteral("#2d2d2d") : QStringLiteral("#f5f5f5"),
             m_isDarkTheme ? QStringLiteral("#3d3d3d") : QStringLiteral("#cccccc"),
             m_isDarkTheme ? QStringLiteral("rgba(233, 30, 99, 0.15)") : QStringLiteral("rgba(233, 30, 99, 0.12)")));

    QVBoxLayout *layout = new QVBoxLayout(dialog);

    QLabel *label = new QLabel(GTr::selectSessionToManage(), dialog);
    layout->addWidget(label);

    QListWidget *sessionList = new QListWidget(dialog);
    for (const auto &meta : sessions) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setData(Qt::UserRole, meta.id);
        item->setSizeHint(QSize(0, 36));

        QWidget *itemWidget = new QWidget();
        itemWidget->setStyleSheet(QStringLiteral("background: transparent;"));
        QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);
        itemLayout->setContentsMargins(8, 2, 8, 2);
        itemLayout->setSpacing(4);

        QString displayName = meta.name;
        if (meta.id == currentId)
            displayName += QStringLiteral("  (") + GTr::currentSessionLabel() + QStringLiteral(")");

        if (meta.pinned) {
            QFont f = QFont();
            f.setBold(true);
            QLabel *nameLabel = new QLabel(QStringLiteral("📌 ") + displayName);
            nameLabel->setFont(f);
            itemLayout->addWidget(nameLabel);
        } else {
            itemLayout->addWidget(new QLabel(displayName));
        }
        itemLayout->addStretch();

        QPushButton *menuBtn = new QPushButton(QStringLiteral("⋯"));
        menuBtn->setFixedSize(28, 28);
        menuBtn->setCursor(Qt::PointingHandCursor);
        menuBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background: transparent; color: %1; border: 1px solid %2; "
            "border-radius: 4px; font-size: 14px; }"
            "QPushButton:hover { background: #e91e63; color: white; border-color: #e91e63; }")
            .arg(m_isDarkTheme ? QStringLiteral("#e0e0e0") : QStringLiteral("#555555"),
                 m_isDarkTheme ? QStringLiteral("#555555") : QStringLiteral("#cccccc")));

        QString sid = meta.id;
        QString sname = meta.name;
        bool sPinned = meta.pinned;
        connect(menuBtn, &QPushButton::clicked, this, [this, dialog, sessionList, sid, sname, sPinned]() {
            QMenu menu(dialog);
            QAction *renameAct = menu.addAction(GTr::renameLabel());
            QAction *pinAct = menu.addAction(sPinned ? GTr::unpinLabel() : GTr::pinLabel());
            menu.addSeparator();
            QAction *deleteAct = menu.addAction(GTr::deleteLabel());
            // Disable delete for current session
            if (sid == GirlfriendSessionManager::instance()->currentSessionId())
                deleteAct->setEnabled(false);

            QAction *chosen = menu.exec(QCursor::pos());
            if (chosen == renameAct) {
                bool ok;
                QString newName = QInputDialog::getText(dialog,
                    GTr::renameLabel(), GTr::renameLabel() + QStringLiteral(":"),
                    QLineEdit::Normal, sname, &ok);
                if (ok && !newName.trimmed().isEmpty()) {
                    GirlfriendSessionManager::instance()->renameSession(sid, newName.trimmed());
                    dialog->accept(); // Close and reopen to refresh
                    QMetaObject::invokeMethod(this, "onManageConversations", Qt::QueuedConnection);
                }
            } else if (chosen == pinAct) {
                GirlfriendSessionManager::instance()->setSessionPinned(sid, !sPinned);
                dialog->accept();
                QMetaObject::invokeMethod(this, "onManageConversations", Qt::QueuedConnection);
            } else if (chosen == deleteAct) {
                auto reallyDelete = QMessageBox::question(dialog,
                    GTr::deleteLabel(),
                    GTr::deleteSessionConfirmMessage(sname),
                    QMessageBox::Yes | QMessageBox::No);
                if (reallyDelete == QMessageBox::Yes) {
                    GirlfriendSessionManager::instance()->deleteSession(sid);
                    dialog->accept();
                    QMetaObject::invokeMethod(this, "onManageConversations", Qt::QueuedConnection);
                }
            }
        });

        itemLayout->addWidget(menuBtn);
        sessionList->addItem(item);
        sessionList->setItemWidget(item, itemWidget);
    }
    layout->addWidget(sessionList);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *closeBtn = new QPushButton(GTr::closeButton(), dialog);
    closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; color: %1; border: 1px solid #e91e63; "
        "padding: 6px 16px; border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background: #e91e63; color: white; }")
        .arg(m_isDarkTheme ? QStringLiteral("#e0e0e0") : QStringLiteral("#333333")));
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    dialog->exec();
    dialog->deleteLater();
}

void GirlfriendWindow::onAvatarLevelChanged(int level)
{
    AvatarLevel newLevel = static_cast<AvatarLevel>(level - 1);
    AvatarLevel oldLevel = GirlfriendSettings::instance()->avatarLevel();

    bool wasSpeaking = m_voiceManager->isSpeaking();
    bool wasStreaming = m_isStreaming;

    // 从 AvatarWidget 获取当前实际显示的情绪（而非 session，session 可能过期）
    QString currentEmotion = m_avatarWidget->currentDisplayEmotion();
    if (currentEmotion.isEmpty() || currentEmotion == "speaking") {
        currentEmotion = "default";
    }

    // === 根据切换方向处理不同的逻辑 ===

    // 切换到视频模式 (Level 3): 立刻停止 TTS，播放视频音频
    if (newLevel == AvatarLevel::Level3_Hotter) {
        // 立刻停止 TTS 并清理音频缓冲
        if (wasSpeaking) {
            m_voiceManager->stopSpeaking();
            // 解锁状态，让视频可以播放
            m_avatarWidget->unlockState();

            // 等待音频清理完成（处理事件队列）
            QCoreApplication::processEvents();

            qDebug() << "GirlfriendWindow: TTS stopped and audio cleared before video mode";
        } else {
            m_avatarWidget->unlockState();
        }

        // 如果正在流式输出，取消请求
        if (wasStreaming) {
            m_isStreaming = false;
            m_networkManager->abortCurrentRequest();
            if (m_streamingBubble) {
                m_chatLayout->removeWidget(m_streamingBubble);
                m_streamingBubble->deleteLater();
                m_streamingBubble = nullptr;
                m_streamingTextLabel = nullptr;
            }
            setInputEnabled(true);
        }

        // 切换到视频模式
        GirlfriendSettings::instance()->setAvatarLevel(newLevel);
        m_avatarWidget->setAvatarLevel(newLevel);

        // 设置当前情绪（视频会播放对应的情绪视频）
        m_avatarWidget->setEmotion(currentEmotion);
        m_avatarWidget->resetIdleTimer();

    }
    // 切换到图片模式 (Level 1/2): 继续播放 TTS 直至完成
    else {
        // 切换到图片模式时，不停止 TTS，让它继续播放直至完成
        // TTS 播放期间保持 speaking 状态
        // TTS 完成后 onSpeakingFinished 会解锁状态并切换情绪

        // 如果正在流式输出，取消请求
        if (wasStreaming) {
            m_isStreaming = false;
            m_networkManager->abortCurrentRequest();
            if (m_streamingBubble) {
                m_chatLayout->removeWidget(m_streamingBubble);
                m_streamingBubble->deleteLater();
                m_streamingBubble = nullptr;
                m_streamingTextLabel = nullptr;
            }
            setInputEnabled(true);
        }

        // 停止视频（如果是从 Level 3 切换过来）
        GirlfriendSettings::instance()->setAvatarLevel(newLevel);
        m_avatarWidget->setAvatarLevel(newLevel);

        // 如果没有在播放 TTS，立即解锁并恢复情绪
        if (!wasSpeaking) {
            m_avatarWidget->unlockState();
            m_avatarWidget->setEmotion(currentEmotion);
            m_avatarWidget->resetIdleTimer();
        }
        // 如果正在播放 TTS，不解锁，等 onSpeakingFinished 处理

    }

    updateOverlayVisibility();

    qDebug() << "GirlfriendWindow: Avatar level changed to" << level
             << "from" << static_cast<int>(oldLevel) + 1
             << ", emotion:" << currentEmotion
             << ", wasSpeaking:" << wasSpeaking;
}

void GirlfriendWindow::onMoodInfluenceChanged(int level)
{
    GirlfriendSettings::instance()->setMoodInfluence(static_cast<MoodInfluenceLevel>(level));
    qDebug() << "GirlfriendWindow: Mood influence changed to" << level;
}

void GirlfriendWindow::onVideoSoundToggled()
{
    bool enabled = !GirlfriendSettings::instance()->videoSoundEnabled();
    GirlfriendSettings::instance()->setVideoSoundEnabled(enabled);
    qDebug() << "GirlfriendWindow: Video sound toggled to" << enabled;
}

void GirlfriendWindow::onSettingsAvatarLevelChanged(AvatarLevel level)
{
    m_avatarWidget->setAvatarLevel(level);
    updateOverlayVisibility();  // 根据等级更新overlay可见性
}

void GirlfriendWindow::onSettingsVideoSoundChanged(bool enabled)
{
    Q_UNUSED(enabled)
    // Video sound setting changed - could update video player here if needed
}

void GirlfriendWindow::onSettingsVoiceOutputChanged(bool enabled)
{
    Q_UNUSED(enabled)
    // Voice output setting changed - no immediate action needed
}

// ==================== Overlay Label Slots ====================

void GirlfriendWindow::onAvatarEmotionChanged(const QString &emotion)
{
    // 直接从 AvatarWidget 获取当前实际显示的情绪（确保严格同步）
    // 信号传递的值可能因为状态锁等原因不准确
    m_currentOverlayEmotion = m_avatarWidget->currentDisplayEmotion();
    updateOverlayLabels();

    // 同步到 session，确保等级切换时能保留当前情绪
    GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
    if (session) {
        session->setCurrentEmotion(m_currentOverlayEmotion);
    }
}

void GirlfriendWindow::onAvatarMoodChanged(double mood)
{
    // 直接使用信号传递的 mood 值（最准确）
    m_currentOverlayMood = mood;
    qDebug() << "GirlfriendWindow: Mood changed to" << mood << "percent:" << static_cast<int>(mood * 100);
    updateOverlayLabels();
}

void GirlfriendWindow::updateOverlayLabels()
{
    if (!m_overlayEmotionLabel || !m_overlayMoodBarLabel || !m_overlayMoodPercentLabel) {
        return;
    }

    // 首先更新可见性
    updateOverlayVisibility();

    AvatarLevel currentLevel = GirlfriendSettings::instance()->avatarLevel();

    // 在图片模式下（Level 1/2），必须确保进度条正确显示
    // 即使 isVisible() 返回 false（异常情况），也要强制显示
    if (currentLevel != AvatarLevel::Level3_Hotter) {
        // 强制确保标签可见
        m_overlayEmotionLabel->show();
        m_overlayMoodBarLabel->show();
        m_overlayMoodPercentLabel->show();
    }

    // 如果是视频模式且标签被隐藏，不需要更新内容
    if (currentLevel == AvatarLevel::Level3_Hotter && !m_overlayEmotionLabel->isVisible()) {
        return;
    }

    // 更新情绪标签文字
    QMap<QString, QString> emotionLabels = {
        {"default", GTr::emotionDefault()},
        {"happy", GTr::emotionHappy()},
        {"shy", GTr::emotionShy()},
        {"love", GTr::emotionLove()},
        {"hate", GTr::emotionHate()},
        {"sad", GTr::emotionSad()},
        {"angry", GTr::emotionAngry()},
        {"afraid", GTr::emotionAfraid()},
        {"awaiting", GTr::emotionAwaiting()},
        {"speaking", GTr::emotionSpeaking()},
        {"studying", GTr::emotionStudying()},
        {"worried", GTr::emotionWorried()},
        {"crying", GTr::emotionCrying()},
        {"travelling", GTr::emotionTravelling()}
    };

    QString labelText = emotionLabels.value(m_currentOverlayEmotion, GTr::emotionDefault());
    m_overlayEmotionLabel->setText(labelText);
    m_overlayEmotionLabel->adjustSize();
    m_overlayEmotionLabel->raise();

    // 更新mood bar - 使用固定px值确保正确渲染
    int percent = static_cast<int>(m_currentOverlayMood * 100);
    int barWidth = static_cast<int>(m_currentOverlayMood * 50);  // 50px总宽度
    if (barWidth < 2) barWidth = 2;  // 最小宽度2px确保可见

    // 进度条部分：粉红色，空白部分：根据主题
    QString barColor = "#e91e63";  // 粉红色（进度条部分）
    QString bgColor = m_isDarkTheme ? QStringLiteral("#444444") : QStringLiteral("#ffffff");

    // 使用固定px值，避免百分比渲染问题
    // 使用table结构确保渲染正确
    QString barHtml = QString(
        "<table border='0' cellpadding='0' cellspacing='0' width='50'>"
        "<tr><td width='%1' bgcolor='%2' style='border-radius:3px;height:6px;'></td>"
        "<td width='%3' bgcolor='%4' style='border-radius:3px;height:6px;'></td></tr>"
        "</table>"
    ).arg(barWidth).arg(barColor).arg(50 - barWidth).arg(bgColor);

    m_overlayMoodBarLabel->setText(barHtml);
    m_overlayMoodBarLabel->setTextFormat(Qt::RichText);
    m_overlayMoodBarLabel->setFixedWidth(50);  // 固定宽度50px
    m_overlayMoodBarLabel->setFixedHeight(8);  // 设置固定高度确保可见

    m_overlayMoodPercentLabel->setText(QString("%1%").arg(percent));
    m_overlayMoodPercentLabel->adjustSize();

    qDebug() << "updateOverlayLabels: mood=" << m_currentOverlayMood
             << "percent=" << percent << "barWidth=" << barWidth;

    // 重新定位mood bar - 在情绪标签下方
    int emotionLabelHeight = m_overlayEmotionLabel->sizeHint().height();
    m_overlayMoodBarLabel->move(12, 12 + emotionLabelHeight + 4);
    m_overlayMoodBarLabel->raise();
    m_overlayMoodPercentLabel->move(12 + 54, 12 + emotionLabelHeight + 2);
    m_overlayMoodPercentLabel->raise();
}

void GirlfriendWindow::updateOverlayVisibility()
{
    if (m_settingsButton) {
        m_settingsButton->show();
        m_settingsButton->raise();
        m_settingsButton->move(width() - 40, 12);
        m_settingsButton->repaint();
    }

    AvatarLevel currentLevel = GirlfriendSettings::instance()->avatarLevel();

    // Emotion label and mood bar — visible in all levels (including video mode)
    if (m_overlayEmotionLabel) {
        m_overlayEmotionLabel->show();
    }
    if (m_overlayMoodBarLabel) {
        m_overlayMoodBarLabel->show();
    }
    if (m_overlayMoodPercentLabel) {
        m_overlayMoodPercentLabel->show();
    }

    // Chat input area: hidden in video mode (Level 3) — intentional design choice
    QWidget *bottomOverlay = findChild<QWidget *>("bottomOverlay");
    if (bottomOverlay) {
        if (currentLevel == AvatarLevel::Level3_Hotter) {
            bottomOverlay->hide();
        } else {
            bottomOverlay->show();
            bottomOverlay->raise();
        }
    }
}

// ==================== Helper Methods ====================

void GirlfriendWindow::loadSessionMessages()
{
    GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
    if (!session) return;

    for (const GirlfriendMessage &msg : session->messages()) {
        addMessageBubble(msg.role, msg.content);
    }

    // Set avatar emotion from session
    m_avatarWidget->setEmotion(session->currentEmotion());
}

void GirlfriendWindow::clearChatUI()
{
    // Delete all message bubbles
    while (m_chatLayout->count() > 0) {
        QWidget *widget = m_chatLayout->itemAt(0)->widget();
        if (widget) {
            widget->deleteLater();
        }
        m_chatLayout->removeItem(m_chatLayout->itemAt(0));
    }

    // Clear streaming bubble reference
    m_streamingBubble = nullptr;
    m_streamingTextLabel = nullptr;
}

