#pragma once

#ifndef AVATARWIDGET_H
#define AVATARWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QMap>
#include <QString>
#include <QResizeEvent>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsVideoItem>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include "girlfriendsettings.h"

class AvatarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AvatarWidget(QWidget *parent = nullptr);

    void setEmotion(const QString &emotion, bool forceUpdate = false);
    void setSpeaking(bool speaking);
    void setMood(double mood);
    QString currentEmotion() const { return m_currentEmotion; }
    QString currentDisplayEmotion() const;
    double currentMood() const { return m_currentMood; }
    AvatarLevel currentLevel() const { return m_currentLevel; }
    void retranslateUi();
    void setAvatarLevel(AvatarLevel level);
    void hideInternalLabels(bool hide);
    void lockState();
    void unlockState();
    void resetIdleTimer();

signals:
    void emotionChanged(const QString &emotion);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void loadAvatarImages();
    void updateDisplay();
    void updateMoodDisplay();
    QString getAvatarPath(const QString &emotion) const;
    void playVideo(const QString &emotion);
    void stopVideo();
    void onIdleCycle();

    QLabel *m_avatarLabel;
    QLabel *m_emotionTagLabel;
    QLabel *m_moodBarWidget;
    QLabel *m_moodPercentLabel;
    QMap<QString, QPixmap> m_avatarImages;

    // Video player for Level 3 — uses QGraphicsView for proper widget layering
    QMediaPlayer *m_videoPlayer;
    QAudioOutput *m_audioOutput;
    QGraphicsView *m_graphicsView;
    QGraphicsScene *m_graphicsScene;
    QGraphicsVideoItem *m_videoItem;
    QString m_currentVideoEmotion;

    // Idle emotion cycling — adds visual variety when user is inactive
    QTimer *m_idleTimer;
    QTimer *m_idleCycleTimer;
    bool m_idleCycling = false;

    QString m_currentEmotion;
    bool m_isSpeaking;
    double m_currentMood = 0.6;
    AvatarLevel m_currentLevel;

    bool m_stateLocked = false;
    QString m_pendingEmotion;
};

#endif // AVATARWIDGET_H
