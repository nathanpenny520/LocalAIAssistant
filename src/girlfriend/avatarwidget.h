#ifndef AVATARWIDGET_H
#define AVATARWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QMap>
#include <QString>
#include <QResizeEvent>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include "girlfriendsettings.h"

class AvatarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AvatarWidget(QWidget *parent = nullptr);

    void setEmotion(const QString &emotion);
    void setSpeaking(bool speaking);
    void setMood(double mood);
    QString currentEmotion() const { return m_currentEmotion; }
    QString currentDisplayEmotion() const;  // 获取当前实际显示的情绪（考虑 speaking 状态）
    double currentMood() const { return m_currentMood; }
    AvatarLevel currentLevel() const { return m_currentLevel; }
    void retranslateUi();  // 更新情绪标签文字
    void setAvatarLevel(AvatarLevel level);
    void hideInternalLabels(bool hide);  // 隐藏内部情绪/mood标签
    void lockState();    // 锁定状态，禁止情绪切换
    void unlockState();  // 解锁状态，应用暂存的情绪

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

    QLabel *m_avatarLabel;
    QLabel *m_emotionTagLabel;
    QLabel *m_moodBarWidget;
    QLabel *m_moodPercentLabel;
    QMap<QString, QPixmap> m_avatarImages;

    // Video player for Level 3
    QMediaPlayer *m_videoPlayer;
    QAudioOutput *m_audioOutput;
    QVideoWidget *m_videoWidget;
    QString m_currentVideoEmotion;

    QString m_currentEmotion;
    bool m_isSpeaking;
    double m_currentMood = 0.6;
    AvatarLevel m_currentLevel;

    // 状态锁 - 防止在关键操作期间发生冲突的状态切换
    bool m_stateLocked = false;  // true 时禁止情绪/视频切换
    QString m_pendingEmotion;     // 锁定期间暂存的情绪，解锁后应用
};

#endif // AVATARWIDGET_H