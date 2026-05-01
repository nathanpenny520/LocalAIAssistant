#include "girlfriendwindow.h"
#include <QDebug>
#include <QScrollBar>

GirlfriendWindow::GirlfriendWindow(QWidget *parent)
    : QWidget(parent)
    , m_avatarWidget(new AvatarWidget(this))
    , m_personalityEngine(new PersonalityEngine(this))
    , m_session(new GirlfriendSession())
    , m_networkManager(new NetworkManager(this))
    , m_chatScrollArea(new QScrollArea(this))
    , m_chatContainer(new QWidget())
    , m_chatLayout(new QVBoxLayout(m_chatContainer))
    , m_inputLine(new QLineEdit(this))
    , m_sendButton(new QPushButton("发送", this))
    , m_voiceButton(new QPushButton("🎤", this))
    , m_isStreaming(false)
{
    setupUI();

    // 加载会话历史
    m_session->loadFromFile();

    // 连接信号
    connect(m_sendButton, &QPushButton::clicked, this, &GirlfriendWindow::onSendClicked);
    connect(m_voiceButton, &QPushButton::clicked, this, &GirlfriendWindow::onVoiceClicked);
    connect(m_inputLine, &QLineEdit::returnPressed, this, &GirlfriendWindow::onSendClicked);

    connect(m_networkManager, &NetworkManager::streamChunkReceived, this, &GirlfriendWindow::onStreamChunkReceived);
    connect(m_networkManager, &NetworkManager::streamFinished, this, &GirlfriendWindow::onStreamFinished);
    connect(m_networkManager, &NetworkManager::errorOccurred, this, &GirlfriendWindow::onNetworkError);

    // 加载人设 Prompt
    m_personalityEngine->loadPersonalityPrompt();

    // 设置窗口属性
    setWindowTitle("AI女友 - 小清");
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
}

void GirlfriendWindow::setupUI()
{
    // 整体布局：形象背景 + 底部聊天区域
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 形象区域（占约 70%）
    m_avatarWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(m_avatarWidget, 7);

    // 底部聊天区域（占约 30%，半透明）
    QFrame *chatArea = new QFrame(this);
    chatArea->setStyleSheet(
        "QFrame { background: rgba(255, 255, 255, 0.3); border: none; }"
    );
    QVBoxLayout *chatAreaLayout = new QVBoxLayout(chatArea);
    chatAreaLayout->setContentsMargins(12, 12, 12, 12);
    chatAreaLayout->setSpacing(8);

    // 消息滚动区域
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
    m_chatLayout->setSpacing(8);
    chatAreaLayout->addWidget(m_chatScrollArea, 1);

    // 输入区域
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(8);

    m_voiceButton->setStyleSheet(
        "QPushButton { background: #e91e63; color: white; border: none; "
        "padding: 8px 16px; border-radius: 8px; font-size: 12px; min-width: 60px; }"
        "QPushButton:hover { background: #c2185b; }"
    );
    inputLayout->addWidget(m_voiceButton);

    m_inputLine->setStyleSheet(
        "QLineEdit { background: rgba(255, 255, 255, 0.5); border: none; "
        "padding: 8px 12px; border-radius: 8px; font-size: 12px; }"
    );
    m_inputLine->setPlaceholderText("输入消息...");
    inputLayout->addWidget(m_inputLine, 1);

    m_sendButton->setStyleSheet(
        "QPushButton { background: #e91e63; color: white; border: none; "
        "padding: 8px 16px; border-radius: 8px; font-size: 12px; min-width: 60px; }"
        "QPushButton:hover { background: #c2185b; }"
    );
    inputLayout->addWidget(m_sendButton);

    chatAreaLayout->addLayout(inputLayout);
    mainLayout->addWidget(chatArea, 3);
}

void GirlfriendWindow::addMessageBubble(const QString &role, const QString &content)
{
    QFrame *bubble = new QFrame(m_chatContainer);

    QString style;
    if (role == "girlfriend") {
        style = "QFrame { background: rgba(233, 30, 99, 0.15); border-radius: 12px; padding: 6px 12px; }";
        bubble->setLayoutDirection(Qt::LeftToRight);
    } else {
        style = "QFrame { background: rgba(100, 100, 100, 0.1); border-radius: 12px; padding: 6px 12px; }";
        bubble->setLayoutDirection(Qt::RightToLeft);
    }
    bubble->setStyleSheet(style);

    QHBoxLayout *bubbleLayout = new QHBoxLayout(bubble);
    bubbleLayout->setContentsMargins(8, 6, 8, 6);

    QLabel *textLabel = new QLabel(content, bubble);
    textLabel->setStyleSheet("QLabel { background: transparent; font-size: 11px; }");
    textLabel->setWordWrap(true);
    textLabel->setTextFormat(Qt::PlainText);
    bubbleLayout->addWidget(textLabel);

    m_chatLayout->addWidget(bubble);

    // 滚动到底部
    m_chatScrollArea->verticalScrollBar()->setValue(
        m_chatScrollArea->verticalScrollBar()->maximum()
    );
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
        m_inputLine->setPlaceholderText("输入消息...");
        m_sendButton->setText("发送");
    } else {
        m_inputLine->setPlaceholderText("正在回复...");
        m_sendButton->setText("等待中");
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

    // 显示用户消息
    addMessageBubble("user", userInput);
    m_session->addMessage("user", userInput, "default");

    clearInput();
    setInputEnabled(false);

    // 构建请求消息
    QVector<ChatMessage> messages;

    // 添加系统 Prompt
    QString systemPrompt = m_personalityEngine->buildSystemPrompt();
    ChatMessage systemMsg("system", systemPrompt);
    messages.append(systemMsg);

    // 添加历史消息
    for (const GirlfriendMessage &gfMsg : m_session->messages()) {
        QString role = (gfMsg.role == "user") ? "user" : "assistant";
        messages.append(ChatMessage(role, gfMsg.content));
    }

    m_isStreaming = true;
    m_streamingContent.clear();
    m_avatarWidget->setSpeaking(false);

    // 发送请求
    m_networkManager->sendChatRequestWithContext(messages);
}

void GirlfriendWindow::onVoiceClicked()
{
    // TODO: Phase 2 语音集成
    qDebug() << "Voice button clicked - not implemented yet";
}

void GirlfriendWindow::onStreamChunkReceived(const QString &chunk)
{
    if (!m_isStreaming) {
        return;
    }

    m_streamingContent += chunk;

    // 实时情绪检测
    updateAvatarEmotion(m_streamingContent);
}

void GirlfriendWindow::onStreamFinished(const QString &fullContent)
{
    if (!m_isStreaming) {
        return;
    }

    m_isStreaming = false;

    // 显示女友回复
    QString emotion = m_personalityEngine->detectEmotion(fullContent);
    addMessageBubble("girlfriend", fullContent);
    m_session->addMessage("girlfriend", fullContent, emotion);

    // 更新表情
    m_avatarWidget->setEmotion(emotion);
    m_avatarWidget->setSpeaking(false);

    setInputEnabled(true);
    m_session->saveToFile();
}

void GirlfriendWindow::onNetworkError(const QString &error)
{
    m_isStreaming = false;
    m_avatarWidget->setSpeaking(false);

    addMessageBubble("girlfriend", "错误: " + error);
    setInputEnabled(true);
}