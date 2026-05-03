#ifndef GIRLFRIENDWINDOW_H
#define GIRLFRIENDWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QMenu>
#include "avatarwidget.h"
#include "personalityengine.h"
#include "girlfriendsession.h"
#include "memorymanager.h"
#include "networkmanager.h"
#include "datamodels.h"
#include "voicemanager.h"

class GirlfriendWindow : public QWidget
{
    Q_OBJECT

public:
    explicit GirlfriendWindow(QWidget *parent = nullptr);
    ~GirlfriendWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void retranslateUi();  // 更新界面文字

private slots:
    void onSendClicked();
    void onVoiceClicked();
    void onSettingsClicked();       // 设置按钮点击
    void onClearClicked();
    void onToggleVoiceOutput();     // 切换语音输出
    void onStreamChunkReceived(const QString &chunk);
    void onStreamFinished(const QString &fullContent);
    void onNetworkError(const QString &error);

    // 语音相关槽函数
    void onAsrPartialResult(const QString &text);     // ASR中间结果
    void onAsrFinalResult(const QString &text);       // ASR最终结果
    void onAsrError(const QString &error);
    void onSpeakingStarted();
    void onSpeakingFinished();
    void onVoiceStatusChanged(const QString &status);

private:
    void setupUI();
    void addMessageBubble(const QString &role, const QString &content);
    void updateStreamingBubble(const QString &content);  // 更新流式消息
    void clearInput();
    void setInputEnabled(bool enabled);
    void updateAvatarEmotion(const QString &text);

    // 流式思考过滤器
    QString filterThinkingFromChunk(const QString &chunk);
    bool m_inThinkBlock;            // 是否在思考块内
    QString m_currentThinkTag;      // 当前思考块标签类型 (thinking/reasoning/think)
    QString m_thinkFilterBuffer;    // 过滤器缓冲区

    AvatarWidget *m_avatarWidget;
    PersonalityEngine *m_personalityEngine;
    GirlfriendSession *m_session;
    MemoryManager *m_memoryManager;
    NetworkManager *m_networkManager;
    VoiceManager *m_voiceManager;      // 语音管理器

    QScrollArea *m_chatScrollArea;
    QWidget *m_chatContainer;
    QVBoxLayout *m_chatLayout;

    QLineEdit *m_inputLine;
    QPushButton *m_sendButton;
    QPushButton *m_voiceButton;
    QPushButton *m_settingsButton;  // 设置按钮（右上角）
    QMenu *m_settingsMenu;          // 设置菜单

    bool m_isStreaming;
    bool m_enableVoiceOutput;       // 是否启用语音输出
    QString m_streamingContent;
    QFrame *m_streamingBubble;      // 流式消息气泡
    QLabel *m_streamingTextLabel;   // 流式消息文本标签
    QString m_lastReplyText;        // 最后一条回复文本（用于TTS）
};

#endif // GIRLFRIENDWINDOW_H