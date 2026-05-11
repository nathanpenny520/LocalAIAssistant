/**
 * @file girlfriendwindow.h
 * @brief AI girlfriend main window: avatar display, voice interaction, personality, and memory.
 */
#pragma once

#ifndef GIRLFRIENDWINDOW_H
#define GIRLFRIENDWINDOW_H

#include <QCloseEvent>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "avatarwidget.h"
#include "datamodels.h"
#include "girlfriendsession.h"
#include "girlfriendsessionmanager.h"
#include "girlfriendsettings.h"
#include "memorymanager.h"
#include "networkmanager.h"
#include "personalityengine.h"
#include "voicemanager.h"

class GirlfriendWindow : public QWidget {
    Q_OBJECT

public:
    explicit GirlfriendWindow(QWidget* parent = nullptr);
    ~GirlfriendWindow();

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void retranslateUi();
    void updateOverlayLabels();
    void applyTheme();

private slots:
    void onSendClicked();
    void onVoiceClicked();
    void onSettingsClicked();
    void onClearClicked();
    void onToggleVoiceOutput();
    void onStreamChunkReceived(const QString& chunk);
    void onStreamFinished(const QString& fullContent);
    void onNetworkError(const QString& error);

    void onAsrPartialResult(const QString& text);
    void onAsrFinalResult(const QString& text);
    void onAsrError(const QString& error);
    void onSpeakingStarted();
    void onSpeakingFinished();
    void onVoiceStatusChanged(const QString& status);

    // Settings menu slots
    void onSessionChanged(int index);
    void onNewSessionClicked();
    void onManageConversations();
    void onAvatarLevelChanged(int level);
    void onMoodInfluenceChanged(int level);
    void onVideoSoundToggled();
    void onSettingsAvatarLevelChanged(AvatarLevel level);
    void onSettingsVideoSoundChanged(bool enabled);
    void onSettingsVoiceOutputChanged(bool enabled);
    void onAvatarEmotionChanged(const QString& emotion);
    void onAvatarMoodChanged(double mood);

private:
    void setupUI();
    void addMessageBubble(const QString& role, const QString& content);
    void updateStreamingBubble(const QString& content);
    void clearInput();
    void setInputEnabled(bool enabled);
    void updateAvatarEmotion(const QString& text);
    void loadSessionMessages();      // Load messages from current session
    void clearChatUI();              // Clear all message bubbles
    void updateOverlayVisibility();
    void showVoiceConfigDialog();    // 显示语音配置对话妰

    // Streaming thinking filter — strips <thinking>/<reasoning>/<think> blocks from stream
    QString filterThinkingFromChunk(const QString& chunk);
    bool m_inThinkBlock;          // whether currently inside a think block
    QString m_currentThinkTag;    // current think tag (thinking/reasoning/think)
    QString m_thinkFilterBuffer;  // filter buffer for streaming

    AvatarWidget* m_avatarWidget;
    PersonalityEngine* m_personalityEngine;
    MemoryManager* m_memoryManager;
    NetworkManager* m_networkManager;
    VoiceManager* m_voiceManager;

    // Overlay UI elements (above AvatarWidget, visible in video mode too)
    QLabel* m_overlayEmotionLabel;      // emotion label (direct child of GirlfriendWindow)
    QLabel* m_overlayMoodBarLabel;      // mood progress bar
    QLabel* m_overlayMoodPercentLabel;  // mood percentage
    QString m_currentOverlayEmotion;    // current emotion
    double m_currentOverlayMood;        // current mood value

    QScrollArea* m_chatScrollArea;
    QWidget* m_chatContainer;
    QVBoxLayout* m_chatLayout;

    QLineEdit* m_inputLine;
    QPushButton* m_sendButton;
    QPushButton* m_voiceButton;
    QPushButton* m_settingsButton;  // settings button (top-right)
    QMenu* m_settingsMenu;          // settings menu

    bool m_isStreaming;
    QString m_streamingContent;
    QFrame* m_streamingBubble;     // streaming message bubble
    QLabel* m_streamingTextLabel;  // streaming message text label
    QString m_lastReplyText;       // last reply text (used for TTS)
    QString m_lastUserInput;       // last user input (for affection fallback detection)
    bool m_isDarkTheme = false;    // whether dark theme is active
};

#endif  // GIRLFRIENDWINDOW_H