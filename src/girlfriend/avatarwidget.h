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
    double currentMood() const { return m_currentMood; }
    void retranslateUi();  // 更新情绪标签文字
    void setAvatarLevel(AvatarLevel level);

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
};

#endif // AVATARWIDGET_H