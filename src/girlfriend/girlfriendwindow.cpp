#include "girlfriendwindow.h"

#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QEvent>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScrollBar>
#include <QShortcut>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidgetAction>

#include "../ui/stylesheetmanager.h"
#include "apptheme.h"
#include "girlfriend_translations.h"
#include "translationmanager.h"

// Streaming thinking filter — character-by-character processing.
// Strips three thinking tag formats: <thinking>, <reasoning>, <think>
QString GirlfriendWindow::filterThinkingFromChunk(const QString& chunk) {
    QString output;

    // Process character by character
    for (int i = 0; i < chunk.length(); ++i) {
        QChar ch = chunk[i];
        m_thinkFilterBuffer += ch;

        // Detect opening tags
        // <thinking>
        if (m_thinkFilterBuffer.endsWith("<thinking>")) {
            m_inThinkBlock = true;
            m_currentThinkTag = "thinking";  // track which tag type
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

        // Detect closing tags
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

    // If not inside a think block, flush the buffer to output
    if (!m_inThinkBlock) {
        output = m_thinkFilterBuffer;
        m_thinkFilterBuffer.clear();
    }

    return output;
}

// Parse and remove thinking process tags (for final full-content filtering).
// Supports three formats: <thinking>...</thinking>, <reasoning>...</reasoning>, <think>...</think>
static QString parseThinkingContent(const QString& content) {
    QString pureResponse = content;

    // Match all three thinking tag formats
    QStringList patterns = {R"(<thinking>.*?</thinking>)", R"(<reasoning>.*?</reasoning>)",
                            R"(<think>.*?</think>)"};

    for (const QString& pattern : patterns) {
        QRegularExpression thinkingRegex(pattern, QRegularExpression::DotMatchesEverythingOption);
        pureResponse.remove(thinkingRegex);
    }

    // Remove excess whitespace
    pureResponse = pureResponse.trimmed();

    return pureResponse;
}

GirlfriendWindow::GirlfriendWindow(QWidget* parent)
        : QWidget(nullptr)  // nullptr parent — independent top-level window
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
        , m_currentOverlayMood(0.6) {
    // Independent top-level window with title bar, close button, and min/max buttons
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint |
                   Qt::WindowMinMaxButtonsHint);

    setupUI();

    // Load all sessions
    GirlfriendSessionManager::instance()->loadAll();

    // === Connect signals before loading session data ===

    // Must connect emotion-change signal before loadSessionMessages to populate overlay
    connect(m_avatarWidget, &AvatarWidget::emotionChanged, this,
            &GirlfriendWindow::onAvatarEmotionChanged);

    // Connect mood change signals
    connect(m_personalityEngine, &PersonalityEngine::moodChanged, m_avatarWidget,
            &AvatarWidget::setMood);
    connect(m_personalityEngine, &PersonalityEngine::moodChanged, this,
            &GirlfriendWindow::onAvatarMoodChanged);

    // Persist mood changes to session
    connect(m_personalityEngine, &PersonalityEngine::moodChanged, this, [this](double mood) {
        GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
        if (session) {
            session->setMood(mood);
        }
    });

    // Connect settings change signals
    connect(GirlfriendSettings::instance(), &GirlfriendSettings::avatarLevelChanged, this,
            &GirlfriendWindow::onSettingsAvatarLevelChanged);
    connect(GirlfriendSettings::instance(), &GirlfriendSettings::videoSoundChanged, this,
            &GirlfriendWindow::onSettingsVideoSoundChanged);
    connect(GirlfriendSettings::instance(), &GirlfriendSettings::voiceOutputChanged, this,
            &GirlfriendWindow::onSettingsVoiceOutputChanged);

    // === Load session data (signals are connected, overlay will update)

    loadSessionMessages();

    // Restore mood from session and sync to PersonalityEngine
    GirlfriendSession* initialSession = GirlfriendSessionManager::instance()->currentSessionData();
    if (initialSession) {
        double sessionMood = initialSession->mood();
        m_currentOverlayMood = sessionMood;
        m_personalityEngine->setMood(sessionMood);

        // Restore emotion (loadSessionMessages already set it; ensure overlay sync)
        QString sessionEmotion = initialSession->currentEmotion();
        m_currentOverlayEmotion = sessionEmotion;

        // Force overlay labels to reflect correct state on init
        updateOverlayLabels();

    }

    // === Connect remaining signals ===

    connect(m_sendButton, &QPushButton::clicked, this, &GirlfriendWindow::onSendClicked);
    connect(m_voiceButton, &QPushButton::clicked, this, &GirlfriendWindow::onVoiceClicked);
    connect(m_settingsButton, &QPushButton::clicked, this, &GirlfriendWindow::onSettingsClicked);
    connect(m_inputLine, &QLineEdit::returnPressed, this, &GirlfriendWindow::onSendClicked);

    connect(m_networkManager, &NetworkManager::streamChunkReceived, this,
            &GirlfriendWindow::onStreamChunkReceived);
    connect(m_networkManager, &NetworkManager::streamFinished, this,
            &GirlfriendWindow::onStreamFinished);
    connect(m_networkManager, &NetworkManager::errorOccurred, this,
            &GirlfriendWindow::onNetworkError);

    connect(m_voiceManager, &VoiceManager::asrPartialResult, this,
            &GirlfriendWindow::onAsrPartialResult);
    connect(m_voiceManager, &VoiceManager::asrFinalResult, this,
            &GirlfriendWindow::onAsrFinalResult);
    connect(m_voiceManager, &VoiceManager::asrError, this, &GirlfriendWindow::onAsrError);
    connect(m_voiceManager, &VoiceManager::speakingStarted, this,
            &GirlfriendWindow::onSpeakingStarted);
    connect(m_voiceManager, &VoiceManager::speakingFinished, this,
            &GirlfriendWindow::onSpeakingFinished);
    connect(m_voiceManager, &VoiceManager::statusChanged, this,
            &GirlfriendWindow::onVoiceStatusChanged);

    connect(StyleSheetManager::instance(), &StyleSheetManager::themeChanged, this,
            [this](StyleSheetManager::Theme) { applyTheme(); });
    connect(TranslationManager::instance(), &TranslationManager::languageChanged, this,
            [this]() {
                retranslateUi();
                m_personalityEngine->reloadPrompt();
            });

    // Ctrl+G closes this window (same shortcut that opens it from MainWindow)
    QShortcut* closeShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_G), this);
    closeShortcut->setContext(Qt::WindowShortcut);  // active only when window is focused
    connect(closeShortcut, &QShortcut::activated, this, &QWidget::close);

    // Load personality prompt
    QString personalityPrompt = m_personalityEngine->loadPersonalityPrompt();
    m_networkManager->setSystemPrompt(personalityPrompt);

    // Check voice configuration
    if (!m_voiceManager->isConfigured()) {
        m_voiceButton->setToolTip(GTr::voiceNotConfiguredTooltip());
    } else {
        m_voiceButton->setToolTip(GTr::voiceInputTooltip());
    }

    setWindowTitle(GTr::windowTitle());
    // 9:16 aspect ratio — fits Level 2 avatar images without clipping
    resize(360, 640);

    applyTheme();
}

GirlfriendWindow::~GirlfriendWindow() {
    GirlfriendSessionManager::instance()->saveAll();
}

void GirlfriendWindow::closeEvent(QCloseEvent* event) {
    GirlfriendSessionManager::instance()->saveAll();
    event->accept();
}

void GirlfriendWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    // Stretch AvatarWidget to fill the entire window
    m_avatarWidget->setGeometry(0, 0, width(), height());

    // Critical: keep AvatarWidget below all overlay widgets
    m_avatarWidget->lower();

    // Reposition overlay labels — keep them on top
    if (m_overlayEmotionLabel) {
        m_overlayEmotionLabel->move(12, 12);
        m_overlayEmotionLabel->raise();
    }
    if (m_overlayMoodBarLabel) {
        int emotionLabelHeight = m_overlayEmotionLabel ? m_overlayEmotionLabel->sizeHint().height()
                                                       : 24;
        m_overlayMoodBarLabel->move(12, 12 + emotionLabelHeight + 4);
        m_overlayMoodBarLabel->raise();
    }
    if (m_overlayMoodPercentLabel) {
        int emotionLabelHeight = m_overlayEmotionLabel ? m_overlayEmotionLabel->sizeHint().height()
                                                       : 24;
        m_overlayMoodPercentLabel->move(12 + 54, 12 + emotionLabelHeight + 2);
        m_overlayMoodPercentLabel->raise();
    }

    // Reposition settings button (top-right) — keep on top
    m_settingsButton->move(width() - 40, 12);
    m_settingsButton->raise();

    // Reposition bottom chat area — keep on top
    QWidget* bottomOverlay = findChild<QWidget*>("bottomOverlay");
    if (bottomOverlay) {
        int overlayHeight = 200;
        bottomOverlay->setGeometry(0, height() - overlayHeight, width(), overlayHeight);
        bottomOverlay->raise();
    }

    // Force window z-order refresh
    update();
}

void GirlfriendWindow::applyTheme() {
    const AppTheme& t = AppTheme::current();
    m_isDarkTheme = t.windowBg.lightness() < 128;

    // Derive theme colors from AppTheme tokens
    QString bg = t.windowBg.name();
    QString surface = t.surfaceBg.name();
    QString text = t.textPrimary.name();
    QString textInv = m_isDarkTheme ? QStringLiteral("#333333") : QStringLiteral("#ffffff");
    QString secondary = t.textSecondary.name();
    QString hintColor = m_isDarkTheme ? QStringLiteral("#888888") : QStringLiteral("#999999");
    QString border = t.border.name();
    QString hoverBg = t.hoverBg.name();
    QString inputBg = m_isDarkTheme ? QStringLiteral("rgba(233, 30, 99, 0.12)")
                                    : QStringLiteral("rgba(255, 182, 193, 0.5)");
    QString userBubble = m_isDarkTheme ? QStringLiteral("rgba(255, 255, 255, 0.06)")
                                       : QStringLiteral("rgba(100, 100, 100, 0.1)");
    QString gfBubble = m_isDarkTheme ? QStringLiteral("rgba(233, 30, 99, 0.12)")
                                     : QStringLiteral("rgba(233, 30, 99, 0.15)");
    QString pink = t.girlfriendAccent.name();
    QString pinkHover = t.girlfriendAccentHover.name();
    QString pinkDarker = t.girlfriendAccentPressed.name();
    QString red = QStringLiteral("#f44336");
    QString redHover = QStringLiteral("#d32f2f");
    QString grayBg = QStringLiteral("#9e9e9e");
    QString cancelBg = QStringLiteral("#555555");
    QString cancelHov = QStringLiteral("#777777");

    // Base stylesheet for the window
    setStyleSheet(QString());

    // Re-apply stylesheets to all child widgets
    // Settings button (top-right)
    if (m_settingsButton) {
        m_settingsButton->setStyleSheet(QStringLiteral("QPushButton { background: %1; color: %2; "
                                                       "border: none; "
                                                       "font-size: 14px; border-radius: 14px; }"
                                                       "QPushButton:hover { background: %3; }")
                                                .arg(surface, text, hoverBg));
    }

    // Settings menu
    if (m_settingsMenu) {
        m_settingsMenu->setStyleSheet(QStringLiteral("QMenu { background: %1; color: %2; border: "
                                                     "1px solid %3; border-radius: 8px; }"
                                                     "QMenu::item { padding: 8px 20px; color: %2; }"
                                                     "QMenu::item:selected { background: %4; "
                                                     "color: white; }")
                                              .arg(surface, text, border, pink));
    }

    // Voice button (normal state)
    // Only reset voice button style if in normal state (🎤), not recording (🔴) or waiting (⏳)
    if (m_voiceButton && m_voiceButton->text() == QStringLiteral("🎤")) {
        if (m_voiceButton) {
            m_voiceButton->setStyleSheet(QStringLiteral("QPushButton { background: %1; color: "
                                                        "white; border: none; "
                                                        "padding: 10px 18px; border-radius: 10px; "
                                                        "font-size: 13px; min-width: 70px; }"
                                                        "QPushButton:hover { background: %2; }")
                                                 .arg(pink, pinkHover));
        }
    }

    // Input line
    if (m_inputLine) {
        m_inputLine->setStyleSheet(QStringLiteral("QLineEdit { background: %1; border: none; "
                                                  "color: %2; "
                                                  "padding: 10px 14px; border-radius: 10px; "
                                                  "font-size: 13px; }")
                                           .arg(inputBg, text));
    }

    // Send button
    if (m_sendButton) {
        m_sendButton->setStyleSheet(QStringLiteral("QPushButton { background: %1; color: white; "
                                                   "border: none; "
                                                   "padding: 10px 18px; border-radius: 10px; "
                                                   "font-size: 13px; min-width: 70px; }"
                                                   "QPushButton:hover { background: %2; }")
                                            .arg(pink, pinkHover));
    }

}

void GirlfriendWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    } else if (event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange) {
        applyTheme();
    }
    QWidget::changeEvent(event);
}

void GirlfriendWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    // Re-apply theme each time window is shown
    applyTheme();

    // Update overlay state each time window is shown
    updateOverlayVisibility();

    // Ensure all overlay widgets are on top and visible
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
    QWidget* bottomOverlay = findChild<QWidget*>("bottomOverlay");
    if (bottomOverlay) {
        bottomOverlay->raise();
    }

    // Restore emotion and mood label display from session
    GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
    if (session) {
        m_currentOverlayEmotion = session->currentEmotion();
        m_currentOverlayMood = session->mood();
        updateOverlayLabels();
    }

}

void GirlfriendWindow::retranslateUi() {
    setWindowTitle(GTr::windowTitle());
    m_inputLine->setPlaceholderText(GTr::inputPlaceholder());
    m_sendButton->setText(GTr::sendButton());

    if (m_voiceManager->isConfigured()) {
        m_voiceButton->setToolTip(GTr::voiceInputTooltip());
    } else {
        m_voiceButton->setToolTip(GTr::voiceNotConfiguredTooltip());
    }

    m_avatarWidget->retranslateUi();
    updateOverlayLabels();

}

void GirlfriendWindow::setupUI() {
    // Absolute positioning (no layout) — AvatarWidget fills entire window as background
    m_avatarWidget->setParent(this);
    m_avatarWidget->setGeometry(0, 0, width(), height());

    // === Overlay labels (direct children of GirlfriendWindow, above AvatarWidget) ===
    // Emotion label — pink background, white text
    m_overlayEmotionLabel = new QLabel(this);
    m_overlayEmotionLabel->setStyleSheet(
            "QLabel { background: rgba(233, 30, 99, 0.95); color: white; "
            "padding: 4px 12px; font-size: 12px; border-radius: 6px; }");
    m_overlayEmotionLabel->setText(GTr::emotionDefault());
    m_overlayEmotionLabel->adjustSize();
    m_overlayEmotionLabel->move(12, 12);

    // Mood bar — horizontal progress indicator
    m_overlayMoodBarLabel = new QLabel(this);
    m_overlayMoodBarLabel->setStyleSheet("QLabel { background: transparent; }");
    m_overlayMoodBarLabel->setFixedHeight(6);
    m_overlayMoodBarLabel->setFixedWidth(50);

    // Mood percentage label
    m_overlayMoodPercentLabel = new QLabel(this);
    m_overlayMoodPercentLabel->setStyleSheet(
            "QLabel { background: transparent; font-size: 10px; color: white; }");

    updateOverlayLabels();

    // Hide AvatarWidget's internal emotion/mood labels — use our overlay labels instead
    m_avatarWidget->hideInternalLabels(true);

    // Settings button — top-right corner
    m_settingsButton->setFixedSize(28, 28);
    m_settingsButton->setStyleSheet(
            "QPushButton { background: white; color: black; border: none; "
            "font-size: 14px; border-radius: 14px; }"
            "QPushButton:hover { background: #f0f0f0; }");
    m_settingsButton->move(width() - 40, 12);
    m_settingsButton->raise();

    // Create settings menu — white background, dark text
    m_settingsMenu = new QMenu(this);
    m_settingsMenu->setStyleSheet(
            "QMenu { background: white; color: black; border: 1px solid #ccc; border-radius: 8px; }"
            "QMenu::item { padding: 8px 20px; color: black; }"
            "QMenu::item:selected { background: #e91e63; color: white; }");

    // Chat area overlaid at the bottom
    QWidget* bottomOverlay = new QWidget(this);
    bottomOverlay->setObjectName("bottomOverlay");
    bottomOverlay->setStyleSheet("background: transparent;");
    bottomOverlay->setGeometry(0, height() - 200, width(), 200);

    QVBoxLayout* overlayLayout = new QVBoxLayout(bottomOverlay);
    overlayLayout->setContentsMargins(12, 8, 12, 8);
    overlayLayout->setSpacing(8);

    // Message scroll area — fully transparent, only bubbles wrap text
    m_chatScrollArea->setWidget(m_chatContainer);
    m_chatScrollArea->setWidgetResizable(true);
    m_chatScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_chatScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_chatScrollArea->setStyleSheet(
            "QScrollArea { background: transparent; border: none; }"
            "QScrollBar:vertical { width: 6px; background: transparent; }");
    m_chatContainer->setStyleSheet("background: transparent;");
    m_chatLayout->setAlignment(Qt::AlignTop);
    m_chatLayout->setSpacing(6);
    overlayLayout->addWidget(m_chatScrollArea, 1);

    // Input area
    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(8);

    m_voiceButton->setStyleSheet(
            "QPushButton { background: #e91e63; color: white; border: none; "
            "padding: 10px 18px; border-radius: 10px; font-size: 13px; min-width: 70px; }"
            "QPushButton:hover { background: #c2185b; }");
    inputLayout->addWidget(m_voiceButton);

    m_inputLine->setStyleSheet(
            "QLineEdit { background: rgba(255, 182, 193, 0.5); border: none; "  // light pink
            "padding: 10px 14px; border-radius: 10px; font-size: 13px; }");
    m_inputLine->setPlaceholderText(GTr::inputPlaceholder());
    inputLayout->addWidget(m_inputLine, 1);

    m_sendButton->setStyleSheet(
            "QPushButton { background: #e91e63; color: white; border: none; "
            "padding: 10px 18px; border-radius: 10px; font-size: 13px; min-width: 70px; }"
            "QPushButton:hover { background: #c2185b; }");
    inputLayout->addWidget(m_sendButton);

    overlayLayout->addLayout(inputLayout);

    // Ensure correct z-order: avatar at bottom, overlay on top
    m_avatarWidget->lower();
    bottomOverlay->raise();
}

void GirlfriendWindow::addMessageBubble(const QString& role, const QString& content) {
    QFrame* bubble = new QFrame(m_chatContainer);

    QString style;
    if (role == "girlfriend") {
        style = m_isDarkTheme ? QStringLiteral(
                                        "QFrame { background: rgba(233, 30, 99, 0.25); "
                                        "border-radius: 12px; }")
                              : QStringLiteral(
                                        "QFrame { background: rgba(233, 30, 99, 0.15); "
                                        "border-radius: 12px; }");
        bubble->setLayoutDirection(Qt::LeftToRight);
    } else {
        style = m_isDarkTheme ? QStringLiteral(
                                        "QFrame { background: rgba(255, 255, 255, 0.18); "
                                        "border-radius: 12px; }")
                              : QStringLiteral(
                                        "QFrame { background: rgba(100, 100, 100, 0.1); "
                                        "border-radius: 12px; }");
        bubble->setLayoutDirection(Qt::RightToLeft);
    }
    bubble->setStyleSheet(style);

    QHBoxLayout* bubbleLayout = new QHBoxLayout(bubble);
    bubbleLayout->setContentsMargins(12, 8, 12, 8);  // generous padding

    QLabel* textLabel = new QLabel(content, bubble);
    textLabel->setStyleSheet("QLabel { background: transparent; font-size: 12px; }");
    textLabel->setWordWrap(true);
    textLabel->setTextFormat(Qt::PlainText);
    textLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    // Minimum width ensures text is visible
    textLabel->setMinimumWidth(100);
    bubbleLayout->addWidget(textLabel);

    m_chatLayout->addWidget(bubble);

    // Scroll to bottom
    QTimer::singleShot(10, this, [this]() {
        m_chatScrollArea->verticalScrollBar()->setValue(
                m_chatScrollArea->verticalScrollBar()->maximum());
    });
}

void GirlfriendWindow::updateStreamingBubble(const QString& content) {
    // Create streaming bubble if it doesn't exist yet
    if (!m_streamingBubble) {
        m_streamingBubble = new QFrame(m_chatContainer);
        m_streamingBubble->setStyleSheet(m_isDarkTheme ? QStringLiteral("QFrame { background: "
                                                                        "rgba(233, 30, 99, 0.25); "
                                                                        "border-radius: 12px; }")
                                                       : QStringLiteral("QFrame { background: "
                                                                        "rgba(233, 30, 99, 0.15); "
                                                                        "border-radius: 12px; }"));
        m_streamingBubble->setLayoutDirection(Qt::LeftToRight);

        QHBoxLayout* bubbleLayout = new QHBoxLayout(m_streamingBubble);
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

    // Display filtered content directly
    QString displayContent = content.trimmed();
    if (displayContent.isEmpty() && m_inThinkBlock) {
        // Still in thinking phase — show indicator
        m_streamingTextLabel->setText(GTr::thinking());
    } else if (!displayContent.isEmpty()) {
        m_streamingTextLabel->setText(displayContent);
    }

    // Scroll to bottom
    QTimer::singleShot(10, this, [this]() {
        m_chatScrollArea->verticalScrollBar()->setValue(
                m_chatScrollArea->verticalScrollBar()->maximum());
    });
}

void GirlfriendWindow::clearInput() {
    m_inputLine->clear();
    m_inputLine->setFocus();
}

void GirlfriendWindow::setInputEnabled(bool enabled) {
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

void GirlfriendWindow::updateAvatarEmotion(const QString& text) {
    QString emotion = m_personalityEngine->detectEmotion(text);
    m_avatarWidget->setEmotion(emotion);
    GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
    if (session) {
        session->setCurrentEmotion(emotion);
    }
}

void GirlfriendWindow::onSendClicked() {
    QString userInput = m_inputLine->text().trimmed();
    if (userInput.isEmpty()) {
        return;
    }

    GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
    if (!session) {
        return;
    }

    // Save user input for later affection fallback detection
    m_lastUserInput = userInput;

    m_avatarWidget->resetIdleTimer();

    // Rebuild system prompt with updated mood, time context, and user memory
    QString memoryContent = m_memoryManager->getMemoryContent();
    // Only inject memory if it contains actual entries ("- xxx") — avoid sending empty template to AI
    if (!memoryContent.contains(QStringLiteral("- "))) memoryContent.clear();
    QString systemPrompt = m_personalityEngine->buildSystemPrompt(memoryContent);
    m_networkManager->setSystemPrompt(systemPrompt);

    // Display user message
    addMessageBubble("user", userInput);
    session->addMessage("user", userInput, "default");

    clearInput();
    setInputEnabled(false);

    // Build request — history only; system prompt is managed by NetworkManager
    QVector<ChatMessage> messages;

    // Include history messages
    for (const GirlfriendMessage& gfMsg : session->messages()) {
        QString role = (gfMsg.role == "user") ? "user" : "assistant";
        messages.append(ChatMessage(role, gfMsg.content));
    }

    m_isStreaming = true;
    m_streamingContent.clear();
    m_inThinkBlock = false;       // reset think block state
    m_currentThinkTag.clear();    // reset current tag type
    m_thinkFilterBuffer.clear();  // reset filter buffer
    m_streamingBubble = nullptr;  // discard old streaming bubble
    m_streamingTextLabel = nullptr;

    // Lock state and enter speaking mode — held from send through TTS completion
    m_avatarWidget->lockState();
    m_avatarWidget->setSpeaking(true);
    m_avatarWidget->setEmotion("speaking");

    // Send request
    m_networkManager->sendChatRequestWithContext(messages);
}

void GirlfriendWindow::onVoiceClicked() {
    if (!m_voiceManager->isConfigured()) {
        return;
    }

    if (m_voiceManager->isRecording()) {
        // Stop recording
        m_voiceManager->stopRecording();
        m_voiceButton->setText("⏳");
        m_voiceButton->setStyleSheet(
                "QPushButton { background: #9e9e9e; color: white; border: none; "
                "padding: 10px 18px; border-radius: 10px; font-size: 13px; min-width: 70px; }");
    } else {
        // Start recording
        m_voiceManager->startRecording();
        m_voiceButton->setText("🔴");
        m_voiceButton->setStyleSheet(
                "QPushButton { background: #f44336; color: white; border: none; "
                "padding: 10px 18px; border-radius: 10px; font-size: 13px; min-width: 70px; }"
                "QPushButton:hover { background: #d32f2f; }");
    }
}

void GirlfriendWindow::onSettingsClicked() {
    // Clear menu
    m_settingsMenu->clear();

    // === Sessions Section ===
    // Sessions label (disabled action for visual separation)
    QAction* sessionsLabelAction = m_settingsMenu->addAction(GTr::sessionsLabel());
    sessionsLabelAction->setEnabled(false);
    QFont labelFont = sessionsLabelAction->font();
    labelFont.setBold(true);
    sessionsLabelAction->setFont(labelFont);

    // Session dropdown using QWidgetAction
    QWidgetAction* sessionComboAction = new QWidgetAction(m_settingsMenu);
    QComboBox* sessionComboBox = new QComboBox(m_settingsMenu);
    sessionComboBox->setStyleSheet(
            QStringLiteral("QComboBox { background: %1; color: %2; border: 1px solid %3; "
                           "padding: 4px 8px; border-radius: 4px; min-width: 150px; }"
                           "QComboBox::drop-down { border: none; }"
                           "QComboBox::down-arrow { image: none; border-left: 4px solid "
                           "transparent; "
                           "border-right: 4px solid transparent; border-top: 6px solid %4; "
                           "margin-right: 8px; }"
                           "QComboBox QAbstractItemView { background: %1; color: %2; "
                           "selection-background-color: #e91e63; }")
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
    connect(sessionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &GirlfriendWindow::onSessionChanged);
    sessionComboAction->setDefaultWidget(sessionComboBox);
    m_settingsMenu->addAction(sessionComboAction);

    // New Session action
    QAction* newSessionAction = m_settingsMenu->addAction("+ " + GTr::newSession());
    connect(newSessionAction, &QAction::triggered, this, &GirlfriendWindow::onNewSessionClicked);

    // Manage Conversations button
    {
        QWidgetAction* manageAction = new QWidgetAction(m_settingsMenu);
        QPushButton* manageBtn = new QPushButton(GTr::manageConversations(), m_settingsMenu);
        manageBtn->setStyleSheet(
                "QPushButton { background: #e91e63; color: white; padding: 4px 8px; "
                "border-radius: 4px; font-size: 11px; border: none; }"
                "QPushButton:hover { background: #d81b60; }");
        connect(manageBtn, &QPushButton::clicked, this, &GirlfriendWindow::onManageConversations);
        manageAction->setDefaultWidget(manageBtn);
        m_settingsMenu->addAction(manageAction);
    }

    m_settingsMenu->addSeparator();

    // === Avatar Level Section ===
    QAction* avatarLevelLabel = m_settingsMenu->addAction(GTr::avatarLevelLabel());
    avatarLevelLabel->setEnabled(false);
    avatarLevelLabel->setFont(labelFont);

    // Avatar level buttons using QWidgetAction with exclusive selection
    QWidgetAction* avatarLevelAction = new QWidgetAction(m_settingsMenu);
    QWidget* avatarLevelWidget = new QWidget(m_settingsMenu);
    QHBoxLayout* avatarLevelLayout = new QHBoxLayout(avatarLevelWidget);
    avatarLevelLayout->setContentsMargins(8, 4, 8, 4);
    avatarLevelLayout->setSpacing(4);

    // Use QButtonGroup for exclusive selection
    QButtonGroup* avatarLevelGroup = new QButtonGroup(avatarLevelWidget);
    avatarLevelGroup->setExclusive(true);

    AvatarLevel currentLevel = GirlfriendSettings::instance()->avatarLevel();
    for (int i = 1; i <= 3; ++i) {
        QPushButton* levelBtn = new QPushButton(QString::number(i), avatarLevelWidget);
        levelBtn->setCheckable(true);
        levelBtn->setChecked(static_cast<int>(currentLevel) == i - 1);
        levelBtn->setFixedSize(40, 28);
        levelBtn->setStyleSheet(
                QStringLiteral("QPushButton { background: %1; color: %2; border: 1px solid %3; "
                               "border-radius: 4px; }"
                               "QPushButton:checked { background: #e91e63; color: white; border: "
                               "1px solid #e91e63; }"
                               "QPushButton:hover { background: %4; }")
                        .arg(m_isDarkTheme ? QStringLiteral("#2d2d2d") : QStringLiteral("white"),
                             m_isDarkTheme ? QStringLiteral("#e0e0e0") : QStringLiteral("black"),
                             m_isDarkTheme ? QStringLiteral("#3d3d3d") : QStringLiteral("#cccccc"),
                             m_isDarkTheme ? QStringLiteral("#3d3d3d")
                                           : QStringLiteral("#f8bbd9")));
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
    QAction* moodInfluenceLabel = m_settingsMenu->addAction(GTr::moodInfluenceLabel());
    moodInfluenceLabel->setEnabled(false);
    moodInfluenceLabel->setFont(labelFont);

    // Mood influence buttons using QWidgetAction with exclusive selection
    QWidgetAction* moodInfluenceAction = new QWidgetAction(m_settingsMenu);
    QWidget* moodInfluenceWidget = new QWidget(m_settingsMenu);
    QHBoxLayout* moodInfluenceLayout = new QHBoxLayout(moodInfluenceWidget);
    moodInfluenceLayout->setContentsMargins(8, 4, 8, 4);
    moodInfluenceLayout->setSpacing(4);

    // Use QButtonGroup for exclusive selection
    QButtonGroup* moodInfluenceGroup = new QButtonGroup(moodInfluenceWidget);
    moodInfluenceGroup->setExclusive(true);

    MoodInfluenceLevel currentMood = GirlfriendSettings::instance()->moodInfluence();
    QStringList moodLabels = {GTr::moodLow(), GTr::moodMedium(), GTr::moodHigh()};
    for (int i = 0; i < 3; ++i) {
        QPushButton* moodBtn = new QPushButton(moodLabels[i], moodInfluenceWidget);
        moodBtn->setCheckable(true);
        moodBtn->setChecked(static_cast<int>(currentMood) == i);
        moodBtn->setFixedSize(50, 28);
        moodBtn->setStyleSheet(
                QStringLiteral("QPushButton { background: %1; color: %2; border: 1px solid %3; "
                               "border-radius: 4px; }"
                               "QPushButton:checked { background: #e91e63; color: white; border: "
                               "1px solid #e91e63; }"
                               "QPushButton:hover { background: %4; }")
                        .arg(m_isDarkTheme ? QStringLiteral("#2d2d2d") : QStringLiteral("white"),
                             m_isDarkTheme ? QStringLiteral("#e0e0e0") : QStringLiteral("black"),
                             m_isDarkTheme ? QStringLiteral("#3d3d3d") : QStringLiteral("#cccccc"),
                             m_isDarkTheme ? QStringLiteral("#3d3d3d")
                                           : QStringLiteral("#f8bbd9")));
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
                                 (GirlfriendSettings::instance()->videoSoundEnabled()
                                          ? GTr::videoSoundOn()
                                          : GTr::videoSoundOff());
        QAction* videoSoundAction = m_settingsMenu->addAction(videoSoundText);
        connect(videoSoundAction, &QAction::triggered, this,
                &GirlfriendWindow::onVideoSoundToggled);

        m_settingsMenu->addSeparator();
    }

    // === Voice Output Toggle ===
    QString voiceOutputText = (GirlfriendSettings::instance()->voiceOutputEnabled()
                                       ? GTr::voiceOutputEnabled()
                                       : GTr::voiceOutputDisabled());
    QAction* voiceOutputAction = m_settingsMenu->addAction(voiceOutputText);
    connect(voiceOutputAction, &QAction::triggered, this, &GirlfriendWindow::onToggleVoiceOutput);

    // === Configure Voice ===
    QAction* voiceConfigAction = m_settingsMenu->addAction(GTr::configureVoice());
    connect(voiceConfigAction, &QAction::triggered, this, &GirlfriendWindow::showVoiceConfigDialog);

    m_settingsMenu->addSeparator();

    // === Clear History ===
    QAction* clearAction = m_settingsMenu->addAction(GTr::clearHistory());
    connect(clearAction, &QAction::triggered, this, &GirlfriendWindow::onClearClicked);

    // Show menu
    m_settingsMenu->exec(m_settingsButton->mapToGlobal(QPoint(0, m_settingsButton->height())));
}

void GirlfriendWindow::onToggleVoiceOutput() {
    bool enabled = !GirlfriendSettings::instance()->voiceOutputEnabled();
    GirlfriendSettings::instance()->setVoiceOutputEnabled(enabled);

    // If turning off voice output while speaking, stop immediately and unlock state
    if (!enabled && m_voiceManager->isSpeaking()) {
        m_voiceManager->stopSpeaking();
        m_avatarWidget->unlockState();
    }
}

void GirlfriendWindow::onClearClicked() {
    // Confirmation dialog
    QMessageBox::StandardButton reply = QMessageBox::question(this, GTr::clearConfirmTitle(),
                                                              GTr::clearConfirmMessage(),
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Stop voice and unlock before clearing history
        if (m_voiceManager->isSpeaking()) {
            m_voiceManager->stopSpeaking();
        }
        if (m_isStreaming) {
            m_isStreaming = false;
        }
        m_avatarWidget->unlockState();

        GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
        if (session) {
            session->clearMessages();
            // Reset mood to default 60%
            session->setMood(0.6);
            session->setCurrentEmotion("default");
            GirlfriendSessionManager::instance()->saveAll();
        }

        clearChatUI();

        // Reset emotion and mood state
        m_avatarWidget->resetIdleTimer();
        m_avatarWidget->setEmotion("default");
        m_personalityEngine->setMood(0.6);
        m_avatarWidget->setMood(0.6);
        m_currentOverlayEmotion = "default";
        m_currentOverlayMood = 0.6;
        updateOverlayLabels();

    }
}

void GirlfriendWindow::showVoiceConfigDialog() {
    GirlfriendSettings* gs = GirlfriendSettings::instance();

    QDialog dialog(this);
    dialog.setWindowTitle(GTr::voiceConfigTitle());
    dialog.setMinimumWidth(420);
    dialog.setStyleSheet(
            QStringLiteral("QDialog { background: %1; }"
                           "QLabel { color: %2; }"
                           "QLineEdit { background: %3; color: %2; border: 1px solid %4; padding: "
                           "6px; border-radius: 4px; }")
                    .arg(m_isDarkTheme ? QStringLiteral("#1e1e1e") : QStringLiteral("#ffffff"),
                         m_isDarkTheme ? QStringLiteral("#e0e0e0") : QStringLiteral("#333333"),
                         m_isDarkTheme ? QStringLiteral("#2d2d2d") : QStringLiteral("#ffffff"),
                         m_isDarkTheme ? QStringLiteral("#3d3d3d") : QStringLiteral("#cccccc")));

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setSpacing(12);

    // Description
    QLabel* descLabel = new QLabel(GTr::voiceConfigDescription(), &dialog);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(
            QStringLiteral("QLabel { color: %1; font-size: 11px; }")
                    .arg(m_isDarkTheme ? QStringLiteral("#999999") : QStringLiteral("#666666")));
    mainLayout->addWidget(descLabel);

    // Form
    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(8);
    formLayout->setContentsMargins(0, 8, 0, 8);

    QLineEdit* appIdEdit = new QLineEdit(gs->xfyunAppId(), &dialog);
    formLayout->addRow(GTr::voiceConfigAppId() + ":", appIdEdit);

    QLineEdit* apiKeyEdit = new QLineEdit(gs->xfyunApiKey(), &dialog);
    formLayout->addRow(GTr::voiceConfigApiKey() + ":", apiKeyEdit);

    QLineEdit* apiSecretEdit = new QLineEdit(gs->xfyunApiSecret(), &dialog);
    formLayout->addRow(GTr::voiceConfigApiSecret() + ":", apiSecretEdit);

    QLineEdit* asrUrlEdit = new QLineEdit(gs->xfyunAsrUrl(), &dialog);
    formLayout->addRow(GTr::voiceConfigAsrUrl() + ":", asrUrlEdit);

    QLineEdit* ttsUrlEdit = new QLineEdit(gs->xfyunTtsUrl(), &dialog);
    formLayout->addRow(GTr::voiceConfigTtsUrl() + ":", ttsUrlEdit);

    QLineEdit* voiceTypeEdit = new QLineEdit(gs->xfyunVoiceType(), &dialog);
    formLayout->addRow(GTr::voiceConfigVoiceType() + ":", voiceTypeEdit);

    mainLayout->addLayout(formLayout);

    // Optional fields hint
    QLabel* optionalHint = new QLabel(GTr::voiceConfigOptionalHint(), &dialog);
    optionalHint->setWordWrap(true);
    optionalHint->setStyleSheet(
            QStringLiteral("QLabel { color: %1; font-size: 10px; font-style: italic; }")
                    .arg(m_isDarkTheme ? QStringLiteral("#888888") : QStringLiteral("#999999")));
    mainLayout->addWidget(optionalHint);

    // Restart hint
    QLabel* hintLabel = new QLabel(GTr::voiceConfigTestHint(), &dialog);
    hintLabel->setWordWrap(true);
    hintLabel->setStyleSheet(
            QStringLiteral("QLabel { color: %1; font-size: 10px; font-style: italic; }")
                    .arg(m_isDarkTheme ? QStringLiteral("#888888") : QStringLiteral("#999999")));
    mainLayout->addWidget(hintLabel);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton* cancelBtn = new QPushButton(GTr::voiceConfigCancel(), &dialog);
    cancelBtn->setStyleSheet(
            "QPushButton { background: #555; color: white; border: none; "
            "padding: 8px 20px; border-radius: 6px; font-size: 12px; }"
            "QPushButton:hover { background: #777; }");
    cancelBtn->setMinimumWidth(80);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    buttonLayout->addWidget(cancelBtn);

    QPushButton* saveBtn = new QPushButton(GTr::voiceConfigSave(), &dialog);
    saveBtn->setStyleSheet(
            "QPushButton { background: #e91e63; color: white; border: none; "
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

void GirlfriendWindow::onStreamChunkReceived(const QString& chunk) {
    if (!m_isStreaming) {
        return;
    }

    // Filter out thinking tags from the chunk
    QString filteredChunk = filterThinkingFromChunk(chunk);
    m_streamingContent += filteredChunk;

    updateStreamingBubble(m_streamingContent);

    // Real-time emotion detection from filtered content
    if (!m_streamingContent.isEmpty()) {
        updateAvatarEmotion(m_streamingContent);
    }
}

void GirlfriendWindow::onStreamFinished(const QString& fullContent) {
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

    // Strip thinking process
    QString pureContent = parseThinkingContent(fullContent);

    // Remove streaming bubble, add final message bubble
    if (m_streamingBubble) {
        m_chatLayout->removeWidget(m_streamingBubble);
        m_streamingBubble->deleteLater();
        m_streamingBubble = nullptr;
        m_streamingTextLabel = nullptr;
    }

    // Parse emotion tag to get emotion ID and cleaned text
    PersonalityEngine::EmotionResult emotionResult =
            m_personalityEngine->parseEmotionFromResponse(pureContent);

    // Parse affection tag and update mood (AI-based or keyword fallback)
    PersonalityEngine::AffectionResult affectionResult =
            m_personalityEngine->parseAffectionFromResponse(emotionResult.cleanText, m_lastUserInput);
    m_personalityEngine->updateMoodFromAI(affectionResult.change);

    // Fallback: parse memory update tags via regex (if AI didn't use tool calls)
    QList<MemoryManager::MemoryUpdate> memoryUpdates =
            m_memoryManager->parseMemoryUpdates(affectionResult.cleanText);

    // Strip memory update markers from display text
    QString displayText = affectionResult.cleanText;
    QRegularExpression memoryRegex(R"(\[(?:更新记忆|memory):[^\]]+\])");
    displayText.remove(memoryRegex);
    displayText = displayText.trimmed();

    addMessageBubble("girlfriend", displayText);
    session->addMessage("girlfriend", displayText, emotionResult.emotion);

    // Auto-name: generate session title from first user message if not yet named
    QString currentId = GirlfriendSessionManager::instance()->currentSessionId();
    const auto& sessions = GirlfriendSessionManager::instance()->sessions();
    for (const auto& meta : sessions) {
        if (meta.id == currentId && !meta.autoNamed && session->messages().size() >= 2) {
            QString firstUserMsg;
            for (const auto& msg : session->messages()) {
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

    // Save reply text for TTS
    m_lastReplyText = displayText;

    // Fallback: apply memory updates via regex
    if (!memoryUpdates.isEmpty()) {
        m_memoryManager->applyMemoryUpdates(memoryUpdates);
    }

    // Update emotion (applied immediately, not deferred for TTS)
    m_avatarWidget->setEmotion(emotionResult.emotion);
    m_avatarWidget->setSpeaking(false);  // streaming output ended; TTS may still be playing

    setInputEnabled(true);
    GirlfriendSessionManager::instance()->saveAll();

    // If voice output is enabled, speak the reply
    if (GirlfriendSettings::instance()->voiceOutputEnabled() && m_voiceManager->isConfigured() &&
        !displayText.isEmpty()) {
        // Clean text: strip all special markers, keep only plain text for TTS
        QString ttsText = displayText;

        // Remove emotion markers and replace with space
        QRegularExpression emotionRegex(R"(\[(?:情绪|emotion):[^\]]+\])");
        ttsText.replace(emotionRegex, " ");

        // Remove action words in parentheses (both English and Chinese brackets, handles nesting)
        QRegularExpression actionRegex(R"(\([^)]*\)|（[^）]*）)");
        // Iterate to handle nested brackets, e.g., (smile(grin))
        while (actionRegex.match(ttsText).hasMatch()) {
            ttsText.replace(actionRegex, " ");
        }

        // Remove memory update markers
        QRegularExpression memoryRegex(R"(\[(?:更新记忆|memory):[^\]]+\])");
        ttsText.replace(memoryRegex, " ");

        // Remove affection markers
        QRegularExpression affectionRegex(R"(\[(?:好感度|affection):[^\]]+\])");
        ttsText.replace(affectionRegex, " ");

        // Remove thinking block content
        QRegularExpression thinkRegex(R"(<thinking>.*?</thinking>|<reasoning>.*?</reasoning>)");
        thinkRegex.setPatternOptions(QRegularExpression::DotMatchesEverythingOption);
        ttsText.replace(thinkRegex, " ");

        // Remove special symbols; keep only Chinese, English, digits, and basic punctuation
        QString cleanText;
        for (const QChar& c : ttsText) {
            ushort unicode = c.unicode();
            bool keep = false;

            // Chinese characters
            if (unicode >= 0x4E00 && unicode <= 0x9FFF) {
                keep = true;
            }
            // English letters
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                keep = true;
            }
            // Digits
            if (c >= '0' && c <= '9') {
                keep = true;
            }
            // English punctuation: comma, period, question mark, exclamation
            if (c == ',' || c == '.' || c == '?' || c == '!') {
                keep = true;
            }
            // Chinese punctuation: comma(U+FF0C), period(3002), question(FF1F), exclamation(FF01)
            if (unicode == 0xFF0C || unicode == 0x3002 || unicode == 0xFF1F || unicode == 0xFF01) {
                keep = true;
            }
            // Space (word separator)
            if (c == ' ') {
                keep = true;
            }

            if (keep) {
                cleanText += c;
            }
        }

        // Collapse whitespace
        cleanText = cleanText.simplified();


        if (!cleanText.isEmpty()) {
            // Keep speaking state during TTS playback
            m_avatarWidget->setSpeaking(true);
            m_avatarWidget->setEmotion("speaking");
            m_voiceManager->speak(cleanText);
        } else {
            // No TTS content — unlock and restore immediately
            m_avatarWidget->unlockState();
        }
    } else {
        // Voice output disabled — unlock and restore immediately
        m_avatarWidget->unlockState();
    }
}

void GirlfriendWindow::onNetworkError(const QString& error) {
    m_isStreaming = false;
    m_avatarWidget->setSpeaking(false);
    m_avatarWidget->unlockState();

    // Remove streaming bubble
    if (m_streamingBubble) {
        m_chatLayout->removeWidget(m_streamingBubble);
        m_streamingBubble->deleteLater();
        m_streamingBubble = nullptr;
        m_streamingTextLabel = nullptr;
    }

    addMessageBubble("girlfriend", GTr::errorPrefix() + error);
    setInputEnabled(true);
}

// ==================== Voice-related Slots ====================

void GirlfriendWindow::onAsrPartialResult(const QString& text) {
    m_inputLine->setText(text);
}

void GirlfriendWindow::onAsrFinalResult(const QString& text) {
    m_inputLine->setText(text);

    // Reset voice button state
    m_voiceButton->setText("🎤");
    m_voiceButton->setStyleSheet(
            "QPushButton { background: #e91e63; color: white; border: none; "
            "padding: 10px 18px; border-radius: 10px; font-size: 13px; min-width: 70px; }"
            "QPushButton:hover { background: #c2185b; }");

    if (!text.isEmpty()) {
        QTimer::singleShot(100, this, &GirlfriendWindow::onSendClicked);
    }
}

void GirlfriendWindow::onAsrError(const QString& error) {
    // Reset voice button state
    m_voiceButton->setText("🎤");
    m_voiceButton->setStyleSheet(
            "QPushButton { background: #e91e63; color: white; border: none; "
            "padding: 10px 18px; border-radius: 10px; font-size: 13px; min-width: 70px; }"
            "QPushButton:hover { background: #c2185b; }");
}

void GirlfriendWindow::onSpeakingStarted() {
    // TTS playback started — state is locked; ensure speaking flag is set
    m_avatarWidget->setSpeaking(true);
}

void GirlfriendWindow::onSpeakingFinished() {
    m_avatarWidget->setSpeaking(false);

    // Only unlock and restore emotion if the level hasn't changed.
    // If already switched, emotion was restored in onAvatarLevelChanged.
    AvatarLevel currentSettingsLevel = GirlfriendSettings::instance()->avatarLevel();
    AvatarLevel currentWidgetLevel = m_avatarWidget->currentLevel();

    if (currentSettingsLevel == currentWidgetLevel) {
        // Level unchanged — unlock and restore emotion normally
        m_avatarWidget->unlockState();

        // Restore session emotion
        GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
        if (session) {
            QString sessionEmotion = session->currentEmotion();
            if (!sessionEmotion.isEmpty() && sessionEmotion != "speaking") {
                m_avatarWidget->setEmotion(sessionEmotion);
            }
        }
    } else {
        // Level already switched — emotion was restored during the switch; just unlock
        m_avatarWidget->unlockState();
    }
}

void GirlfriendWindow::onVoiceStatusChanged(const QString& status) {
    // Status-only signal — no UI update needed here
}

// ==================== Settings Menu Slots ====================

void GirlfriendWindow::onSessionChanged(int index) {
    QComboBox* comboBox = qobject_cast<QComboBox*>(sender());
    if (!comboBox) return;

    QString sessionId = comboBox->itemData(index).toString();
    QString currentId = GirlfriendSessionManager::instance()->currentSessionId();

    if (sessionId != currentId) {
        // Stop voice and unlock before switching sessions
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
                // Load mood from new session
                double sessionMood = session->mood();
                m_personalityEngine->setMood(sessionMood);
                m_avatarWidget->setMood(sessionMood);
                m_currentOverlayMood = sessionMood;
            }

        }
    }
}

void GirlfriendWindow::onNewSessionClicked() {
    // Stop voice and unlock before creating new session
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
    // New session uses default mood (resetMood triggers moodChanged signal to update UI)
    m_personalityEngine->resetMood();
    updateOverlayLabels();

}

void GirlfriendWindow::onManageConversations() {
    QVector<SessionMetadata> sessions = GirlfriendSessionManager::instance()->sessions();
    QString currentId = GirlfriendSessionManager::instance()->currentSessionId();

    // Sort: pinned first, then by name
    std::sort(sessions.begin(), sessions.end(),
              [](const SessionMetadata& a, const SessionMetadata& b) {
                  if (a.pinned != b.pinned) return a.pinned > b.pinned;
                  return a.name.toLower() < b.name.toLower();
              });

    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(GTr::manageConversations());
    dialog->setMinimumWidth(380);
    dialog->setMinimumHeight(300);
    dialog->setStyleSheet(
            QStringLiteral("QDialog { background: %1; }"
                           "QLabel { color: %2; }"
                           "QListWidget { background: %3; color: %2; border: 1px solid %4; }"
                           "QListWidget::item { padding: 4px; }"
                           "QListWidget::item:selected { background: %5; }"
                           "QPushButton { color: %2; }")
                    .arg(m_isDarkTheme ? QStringLiteral("#1e1e1e") : QStringLiteral("#ffffff"),
                         m_isDarkTheme ? QStringLiteral("#e0e0e0") : QStringLiteral("#333333"),
                         m_isDarkTheme ? QStringLiteral("#2d2d2d") : QStringLiteral("#f5f5f5"),
                         m_isDarkTheme ? QStringLiteral("#3d3d3d") : QStringLiteral("#cccccc"),
                         m_isDarkTheme ? QStringLiteral("rgba(233, 30, 99, 0.15)")
                                       : QStringLiteral("rgba(233, 30, 99, 0.12)")));

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    QLabel* label = new QLabel(GTr::selectSessionToManage(), dialog);
    layout->addWidget(label);

    QListWidget* sessionList = new QListWidget(dialog);
    for (const auto& meta : sessions) {
        QListWidgetItem* item = new QListWidgetItem();
        item->setData(Qt::UserRole, meta.id);
        item->setSizeHint(QSize(0, 36));

        QWidget* itemWidget = new QWidget();
        itemWidget->setStyleSheet(QStringLiteral("background: transparent;"));
        QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);
        itemLayout->setContentsMargins(8, 2, 8, 2);
        itemLayout->setSpacing(4);

        QString displayName = meta.name;
        if (meta.id == currentId)
            displayName += QStringLiteral("  (") + GTr::currentSessionLabel() + QStringLiteral(")");

        if (meta.pinned) {
            QFont f = QFont();
            f.setBold(true);
            QLabel* nameLabel = new QLabel(QStringLiteral("📌 ") + displayName);
            nameLabel->setFont(f);
            itemLayout->addWidget(nameLabel);
        } else {
            itemLayout->addWidget(new QLabel(displayName));
        }
        itemLayout->addStretch();

        QPushButton* menuBtn = new QPushButton(QStringLiteral("⋯"));
        menuBtn->setFixedSize(28, 28);
        menuBtn->setCursor(Qt::PointingHandCursor);
        menuBtn->setStyleSheet(
                QStringLiteral("QPushButton { background: transparent; color: %1; border: 1px "
                               "solid %2; "
                               "border-radius: 4px; font-size: 14px; }"
                               "QPushButton:hover { background: #e91e63; color: white; "
                               "border-color: #e91e63; }")
                        .arg(m_isDarkTheme ? QStringLiteral("#e0e0e0") : QStringLiteral("#555555"),
                             m_isDarkTheme ? QStringLiteral("#555555")
                                           : QStringLiteral("#cccccc")));

        QString sid = meta.id;
        QString sname = meta.name;
        bool sPinned = meta.pinned;
        connect(menuBtn, &QPushButton::clicked, this,
                [this, dialog, sessionList, sid, sname, sPinned]() {
                    QMenu menu(dialog);
                    QAction* renameAct = menu.addAction(GTr::renameLabel());
                    QAction* pinAct = menu.addAction(sPinned ? GTr::unpinLabel() : GTr::pinLabel());
                    menu.addSeparator();
                    QAction* deleteAct = menu.addAction(GTr::deleteLabel());
                    // Disable delete for current session
                    if (sid == GirlfriendSessionManager::instance()->currentSessionId())
                        deleteAct->setEnabled(false);

                    QAction* chosen = menu.exec(QCursor::pos());
                    if (chosen == renameAct) {
                        bool ok;
                        QString newName =
                                QInputDialog::getText(dialog, GTr::renameLabel(),
                                                      GTr::renameLabel() + QStringLiteral(":"),
                                                      QLineEdit::Normal, sname, &ok);
                        if (ok && !newName.trimmed().isEmpty()) {
                            GirlfriendSessionManager::instance()->renameSession(sid,
                                                                                newName.trimmed());
                            dialog->accept();  // Close and reopen to refresh
                            QMetaObject::invokeMethod(this, "onManageConversations",
                                                      Qt::QueuedConnection);
                        }
                    } else if (chosen == pinAct) {
                        GirlfriendSessionManager::instance()->setSessionPinned(sid, !sPinned);
                        dialog->accept();
                        QMetaObject::invokeMethod(this, "onManageConversations",
                                                  Qt::QueuedConnection);
                    } else if (chosen == deleteAct) {
                        auto reallyDelete = QMessageBox::question(
                                dialog, GTr::deleteLabel(), GTr::deleteSessionConfirmMessage(sname),
                                QMessageBox::Yes | QMessageBox::No);
                        if (reallyDelete == QMessageBox::Yes) {
                            GirlfriendSessionManager::instance()->deleteSession(sid);
                            dialog->accept();
                            QMetaObject::invokeMethod(this, "onManageConversations",
                                                      Qt::QueuedConnection);
                        }
                    }
                });

        itemLayout->addWidget(menuBtn);
        sessionList->addItem(item);
        sessionList->setItemWidget(item, itemWidget);
    }
    layout->addWidget(sessionList);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton* closeBtn = new QPushButton(GTr::closeButton(), dialog);
    closeBtn->setStyleSheet(
            QStringLiteral("QPushButton { background: transparent; color: %1; border: 1px solid "
                           "#e91e63; "
                           "padding: 6px 16px; border-radius: 4px; font-size: 12px; }"
                           "QPushButton:hover { background: #e91e63; color: white; }")
                    .arg(m_isDarkTheme ? QStringLiteral("#e0e0e0") : QStringLiteral("#333333")));
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    dialog->exec();
    dialog->deleteLater();
}

void GirlfriendWindow::onAvatarLevelChanged(int level) {
    AvatarLevel newLevel = static_cast<AvatarLevel>(level - 1);
    AvatarLevel oldLevel = GirlfriendSettings::instance()->avatarLevel();

    bool wasSpeaking = m_voiceManager->isSpeaking();
    bool wasStreaming = m_isStreaming;

    // Get actual displayed emotion from AvatarWidget (session may be stale)
    QString currentEmotion = m_avatarWidget->currentDisplayEmotion();
    if (currentEmotion.isEmpty() || currentEmotion == "speaking") {
        currentEmotion = "default";
    }

    // === Handle logic based on switch direction ===

    // Switching to video mode (Level 3): immediately stop TTS, play video audio
    if (newLevel == AvatarLevel::Level3_Hotter) {
        // Immediately stop TTS and clear audio buffer
        if (wasSpeaking) {
            m_voiceManager->stopSpeaking();
            // Unlock so the video can play
            m_avatarWidget->unlockState();

            // Process event queue to let audio cleanup complete
            QCoreApplication::processEvents();

        } else {
            m_avatarWidget->unlockState();
        }

        // Cancel streaming request if active
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

        // Switch to video mode
        GirlfriendSettings::instance()->setAvatarLevel(newLevel);
        m_avatarWidget->setAvatarLevel(newLevel);

        // Set current emotion (video will play the matching clip)
        m_avatarWidget->setEmotion(currentEmotion);
        m_avatarWidget->resetIdleTimer();

    }
    // Switching to image mode (Level 1/2): let TTS continue playing to completion
    else {
        // Don't stop TTS when switching to image mode — let it finish.
        // Speaking state persists; onSpeakingFinished will unlock and restore emotion.

        // Cancel streaming request if active
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

        // Stop video if switching from Level 3
        GirlfriendSettings::instance()->setAvatarLevel(newLevel);
        m_avatarWidget->setAvatarLevel(newLevel);

        // If not playing TTS, unlock and restore emotion immediately
        if (!wasSpeaking) {
            m_avatarWidget->unlockState();
            m_avatarWidget->setEmotion(currentEmotion);
            m_avatarWidget->resetIdleTimer();
        }
        // If TTS is playing, stay locked — onSpeakingFinished will handle it
    }

    updateOverlayVisibility();

}

void GirlfriendWindow::onMoodInfluenceChanged(int level) {
    GirlfriendSettings::instance()->setMoodInfluence(static_cast<MoodInfluenceLevel>(level));
}

void GirlfriendWindow::onVideoSoundToggled() {
    bool enabled = !GirlfriendSettings::instance()->videoSoundEnabled();
    GirlfriendSettings::instance()->setVideoSoundEnabled(enabled);
}

void GirlfriendWindow::onSettingsAvatarLevelChanged(AvatarLevel level) {
    m_avatarWidget->setAvatarLevel(level);
    updateOverlayVisibility();
}

void GirlfriendWindow::onSettingsVideoSoundChanged(bool enabled) {
    Q_UNUSED(enabled)
    // Video sound setting changed - could update video player here if needed
}

void GirlfriendWindow::onSettingsVoiceOutputChanged(bool enabled) {
    Q_UNUSED(enabled)
    // Voice output setting changed - no immediate action needed
}

// ==================== Overlay Label Slots ====================

void GirlfriendWindow::onAvatarEmotionChanged(const QString& emotion) {
    // Read the actual displayed emotion directly from AvatarWidget (guarantees strict sync).
    // The signal value may be stale due to state locking.
    m_currentOverlayEmotion = m_avatarWidget->currentDisplayEmotion();
    updateOverlayLabels();

    // Sync to session so the emotion survives level switches
    GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
    if (session) {
        session->setCurrentEmotion(m_currentOverlayEmotion);
    }
}

void GirlfriendWindow::onAvatarMoodChanged(double mood) {
    // Use mood value directly from signal (most accurate)
    m_currentOverlayMood = mood;
    updateOverlayLabels();
}

void GirlfriendWindow::updateOverlayLabels() {
    if (!m_overlayEmotionLabel || !m_overlayMoodBarLabel || !m_overlayMoodPercentLabel) {
        return;
    }

    // Update visibility first
    updateOverlayVisibility();

    AvatarLevel currentLevel = GirlfriendSettings::instance()->avatarLevel();

    // In image mode (Level 1/2), force the progress bar to show even if isVisible() is false
    if (currentLevel != AvatarLevel::Level3_Hotter) {
        // Force labels visible
        m_overlayEmotionLabel->show();
        m_overlayMoodBarLabel->show();
        m_overlayMoodPercentLabel->show();
    }

    // In video mode with labels hidden, no need to update content
    if (currentLevel == AvatarLevel::Level3_Hotter && !m_overlayEmotionLabel->isVisible()) {
        return;
    }

    // Update emotion label text
    QMap<QString, QString> emotionLabels = {
            {"default", GTr::emotionDefault()},   {"happy", GTr::emotionHappy()},
            {"shy", GTr::emotionShy()},           {"love", GTr::emotionLove()},
            {"hate", GTr::emotionHate()},         {"sad", GTr::emotionSad()},
            {"angry", GTr::emotionAngry()},       {"afraid", GTr::emotionAfraid()},
            {"awaiting", GTr::emotionAwaiting()}, {"speaking", GTr::emotionSpeaking()},
            {"studying", GTr::emotionStudying()}, {"worried", GTr::emotionWorried()},
            {"crying", GTr::emotionCrying()},     {"travelling", GTr::emotionTravelling()}};

    QString labelText = emotionLabels.value(m_currentOverlayEmotion, GTr::emotionDefault());
    m_overlayEmotionLabel->setText(labelText);
    m_overlayEmotionLabel->adjustSize();
    m_overlayEmotionLabel->raise();

    // Update mood bar — use fixed px values for reliable rendering
    int percent = static_cast<int>(m_currentOverlayMood * 100);
    int barWidth = static_cast<int>(m_currentOverlayMood * 50);  // 50px total width
    if (barWidth < 2) barWidth = 2;                              // min 2px for visibility

    // Progress fill: pink; empty: theme-dependent
    QString barColor = "#e91e63";
    QString bgColor = m_isDarkTheme ? QStringLiteral("#444444") : QStringLiteral("#ffffff");

    // Use fixed px values to avoid percentage rendering bugs.
    // Table structure ensures correct rendering.
    QString barHtml = QString("<table border='0' cellpadding='0' cellspacing='0' width='50'>"
                              "<tr><td width='%1' bgcolor='%2' "
                              "style='border-radius:3px;height:6px;'></td>"
                              "<td width='%3' bgcolor='%4' "
                              "style='border-radius:3px;height:6px;'></td></tr>"
                              "</table>")
                              .arg(barWidth)
                              .arg(barColor)
                              .arg(50 - barWidth)
                              .arg(bgColor);

    m_overlayMoodBarLabel->setText(barHtml);
    m_overlayMoodBarLabel->setTextFormat(Qt::RichText);
    m_overlayMoodBarLabel->setFixedWidth(50);  // fixed 50px width
    m_overlayMoodBarLabel->setFixedHeight(8);  // fixed height for visibility

    m_overlayMoodPercentLabel->setText(QString("%1%").arg(percent));
    m_overlayMoodPercentLabel->adjustSize();


    // Reposition mood bar below emotion label
    int emotionLabelHeight = m_overlayEmotionLabel->sizeHint().height();
    m_overlayMoodBarLabel->move(12, 12 + emotionLabelHeight + 4);
    m_overlayMoodBarLabel->raise();
    m_overlayMoodPercentLabel->move(12 + 54, 12 + emotionLabelHeight + 2);
    m_overlayMoodPercentLabel->raise();
}

void GirlfriendWindow::updateOverlayVisibility() {
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
    QWidget* bottomOverlay = findChild<QWidget*>("bottomOverlay");
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

void GirlfriendWindow::loadSessionMessages() {
    GirlfriendSession* session = GirlfriendSessionManager::instance()->currentSessionData();
    if (!session) return;

    for (const GirlfriendMessage& msg : session->messages()) {
        if (msg.isSystemNotification) continue;
        addMessageBubble(msg.role, msg.content);
    }

    // Set avatar emotion from session
    m_avatarWidget->setEmotion(session->currentEmotion());
}

void GirlfriendWindow::clearChatUI() {
    // Delete all message bubbles
    while (m_chatLayout->count() > 0) {
        QWidget* widget = m_chatLayout->itemAt(0)->widget();
        if (widget) {
            widget->deleteLater();
        }
        m_chatLayout->removeItem(m_chatLayout->itemAt(0));
    }

    // Clear streaming bubble reference
    m_streamingBubble = nullptr;
    m_streamingTextLabel = nullptr;
}
