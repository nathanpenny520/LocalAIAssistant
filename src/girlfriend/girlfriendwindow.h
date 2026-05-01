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
#include "avatarwidget.h"
#include "personalityengine.h"
#include "girlfriendsession.h"
#include "networkmanager.h"
#include "datamodels.h"

class GirlfriendWindow : public QWidget
{
    Q_OBJECT

public:
    explicit GirlfriendWindow(QWidget *parent = nullptr);
    ~GirlfriendWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onSendClicked();
    void onVoiceClicked();
    void onStreamChunkReceived(const QString &chunk);
    void onStreamFinished(const QString &fullContent);
    void onNetworkError(const QString &error);

private:
    void setupUI();
    void addMessageBubble(const QString &role, const QString &content);
    void clearInput();
    void setInputEnabled(bool enabled);
    void updateAvatarEmotion(const QString &text);

    AvatarWidget *m_avatarWidget;
    PersonalityEngine *m_personalityEngine;
    GirlfriendSession *m_session;
    NetworkManager *m_networkManager;

    QScrollArea *m_chatScrollArea;
    QWidget *m_chatContainer;
    QVBoxLayout *m_chatLayout;

    QLineEdit *m_inputLine;
    QPushButton *m_sendButton;
    QPushButton *m_voiceButton;

    bool m_isStreaming;
    QString m_streamingContent;
};

#endif // GIRLFRIENDWINDOW_H