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
    , m_session(new GirlfriendSession())
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
    , m_enableVoiceOutput(true)
    , m_inThinkBlock(false)
    , m_currentThinkTag("")
    , m_thinkFilterBuffer("")
    , m_streamingBubble(nullptr)
    , m_streamingTextLabel(nullptr)
{
    // 设置为独立顶层窗口，有标题栏和关闭按钮
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);

    setupUI();

    // 加载会话历史
    m_session->loadFromFile();

    // 显示历史消息
    for (const GirlfriendMessage &msg : m_session->messages()) {
        addMessageBubble(msg.role, msg.content);
    }

    // 连接信号
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
    resize(400, 600);
}

GirlfriendWindow::~GirlfriendWindow()
{
    m_session->saveToFile();
}

void GirlfriendWindow::closeEvent(QCloseEvent *event)
{
    m_session->saveToFile();
    event->accept();
}

void GirlfriendWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // 更新 AvatarWidget 覆盖整个窗口
    m_avatarWidget->setGeometry(0, 0, width(), height());

    // 更新设置按钮位置（右上角）
    m_settingsButton->move(width() - 40, 12);
    m_settingsButton->raise();

    // 更新底部聊天区域的位置
    QWidget *bottomOverlay = findChild<QWidget *>("bottomOverlay");
    if (bottomOverlay) {
        int overlayHeight = 200;
        bottomOverlay->setGeometry(0, height() - overlayHeight, width(), overlayHeight);
        bottomOverlay->raise();
    }
}

void GirlfriendWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
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

    // 更新左上角情绪标签
    m_avatarWidget->retranslateUi();

    // 更新设置菜单项（菜单会在每次点击时重建，所以这里不需要更新）

    qDebug() << "GirlfriendWindow: UI retranslated";
}

void GirlfriendWindow::setupUI()
{
    // 不使用布局，使用绝对定位叠加
    // AvatarWidget 作为整个窗口的背景（覆盖100%）
    m_avatarWidget->setParent(this);
    m_avatarWidget->setGeometry(0, 0, width(), height());

    // 设置按钮 - 右上角，白色背景黑色文字
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
        style = "QFrame { background: rgba(233, 30, 99, 0.15); border-radius: 12px; }";
        bubble->setLayoutDirection(Qt::LeftToRight);
    } else {
        style = "QFrame { background: rgba(100, 100, 100, 0.1); border-radius: 12px; }";
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
            "QFrame { background: rgba(233, 30, 99, 0.15); border-radius: 12px; }"
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
    m_session->setCurrentEmotion(emotion);
}

void GirlfriendWindow::onSendClicked()
{
    QString userInput = m_inputLine->text().trimmed();
    if (userInput.isEmpty()) {
        return;
    }

    // 更新心情值
    m_personalityEngine->updateMood(userInput);

    // 重新构建系统提示（包含更新后的心情和时间感知）
    QString systemPrompt = m_personalityEngine->buildSystemPrompt();
    m_networkManager->setSystemPrompt(systemPrompt);

    // 显示用户消息
    addMessageBubble("user", userInput);
    m_session->addMessage("user", userInput, "default");

    clearInput();
    setInputEnabled(false);

    // 构建请求消息 - 只包含历史消息，system prompt 由 NetworkManager 处理
    QVector<ChatMessage> messages;

    // 添加历史消息
    for (const GirlfriendMessage &gfMsg : m_session->messages()) {
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
    m_avatarWidget->setSpeaking(true);  // 开始说话动画

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
    // 更新菜单项状态
    m_settingsMenu->clear();

    // 语音输出开关 - 显示当前状态，点击后切换
    QAction *voiceOutputAction = m_settingsMenu->addAction(
        m_enableVoiceOutput ? GTr::voiceOutputEnabled() : GTr::voiceOutputDisabled()
    );
    connect(voiceOutputAction, &QAction::triggered, this, &GirlfriendWindow::onToggleVoiceOutput);

    // 清空历史
    m_settingsMenu->addSeparator();
    QAction *clearAction = m_settingsMenu->addAction(GTr::clearHistory());
    connect(clearAction, &QAction::triggered, this, &GirlfriendWindow::onClearClicked);

    // 显示菜单
    m_settingsMenu->exec(m_settingsButton->mapToGlobal(QPoint(0, m_settingsButton->height())));
}

void GirlfriendWindow::onToggleVoiceOutput()
{
    m_enableVoiceOutput = !m_enableVoiceOutput;
    qDebug() << "GirlfriendWindow: Voice output toggled to" << m_enableVoiceOutput;

    // 如果正在播放语音，立即停止
    if (!m_enableVoiceOutput && m_voiceManager->isSpeaking()) {
        m_voiceManager->stopSpeaking();
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
        // 清空会话历史
        m_session->clearMessages();
        m_session->saveToFile();

        // 清空界面上的消息气泡
        // 删除 m_chatContainer 中的所有子控件
        while (m_chatLayout->count() > 0) {
            QWidget *widget = m_chatLayout->itemAt(0)->widget();
            if (widget) {
                widget->deleteLater();
            }
            m_chatLayout->removeItem(m_chatLayout->itemAt(0));
        }

        // 重置状态
        m_avatarWidget->setEmotion("default");
    }
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
    QRegularExpression memoryRegex(R"(\[更新记忆:[^\]]+\])");
    displayText.remove(memoryRegex);
    displayText = displayText.trimmed();

    // 显示女友回复
    addMessageBubble("girlfriend", displayText);
    m_session->addMessage("girlfriend", displayText, emotionResult.emotion);

    // 保存回复文本用于TTS
    m_lastReplyText = displayText;

    // Fallback: 应用记忆更新（正则解析方式）
    if (!memoryUpdates.isEmpty()) {
        m_memoryManager->applyMemoryUpdates(memoryUpdates);
        qDebug() << "GirlfriendWindow: Applied memory updates via fallback (regex)";
    }

    // 更新表情
    m_avatarWidget->setEmotion(emotionResult.emotion);
    m_avatarWidget->setSpeaking(false);

    setInputEnabled(true);
    m_session->saveToFile();

    // 如果启用语音输出，播报回复
    if (m_enableVoiceOutput && m_voiceManager->isConfigured() && !displayText.isEmpty()) {
        // 清理文本：移除所有特殊标记，只保留纯净文本给TTS
        QString ttsText = displayText;

        // 移除情绪标记 [情绪:xxx] 并替换为空格（保持语义分隔）
        QRegularExpression emotionRegex(R"(\[情绪:[^\]]+\])");
        ttsText.replace(emotionRegex, " ");

        // 移除动作词 (xxx) 和 （xxx） 并替换为空格（支持英文和中文括号，处理嵌套）
        QRegularExpression actionRegex(R"(\([^)]*\)|（[^）]*）)");
        // 循环替换处理嵌套括号，如 (微笑(开心))
        while (actionRegex.match(ttsText).hasMatch()) {
            ttsText.replace(actionRegex, " ");
        }

        // 移除记忆更新标记 [更新记忆:xxx]
        QRegularExpression memoryRegex(R"(\[更新记忆:[^\]]+\])");
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
            m_voiceManager->speak(cleanText);
        }
    }
}

void GirlfriendWindow::onNetworkError(const QString &error)
{
    m_isStreaming = false;
    m_avatarWidget->setSpeaking(false);

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
    // TTS开始播放，切换到speaking表情
    m_avatarWidget->setEmotion("speaking");
    m_avatarWidget->setSpeaking(true);
}

void GirlfriendWindow::onSpeakingFinished()
{
    // TTS播放结束，恢复表情
    m_avatarWidget->setSpeaking(false);

    // 恢复到上次识别的情绪或默认
    QString lastEmotion = m_session->currentEmotion();
    if (lastEmotion.isEmpty() || lastEmotion == "speaking") {
        lastEmotion = "default";
    }
    m_avatarWidget->setEmotion(lastEmotion);
}

void GirlfriendWindow::onVoiceStatusChanged(const QString &status)
{
    // 状态变化，不做UI显示
}