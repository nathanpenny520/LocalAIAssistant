#ifndef GIRLFRIENDSETTINGS_H
#define GIRLFRIENDSETTINGS_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QFile>
#include <QStandardPaths>
#include <QDir>

// Mood influence level
enum class MoodInfluenceLevel {
    Low,
    Medium,
    High
};

// Avatar level
enum class AvatarLevel {
    Level1_Belle,   // PNG images, normal style
    Level2_Hot,     // PNG images, hot style
    Level3_Hotter   // MP4 videos, hotter style
};

class GirlfriendSettings : public QObject
{
    Q_OBJECT

public:
    static GirlfriendSettings* instance();

    // Avatar Level
    AvatarLevel avatarLevel() const { return m_avatarLevel; }
    void setAvatarLevel(AvatarLevel level);
    QString avatarLevelPath() const;

    // Mood Influence
    MoodInfluenceLevel moodInfluence() const { return m_moodInfluence; }
    void setMoodInfluence(MoodInfluenceLevel level);

    // Video Sound (only for Level 3)
    bool videoSoundEnabled() const { return m_videoSoundEnabled; }
    void setVideoSoundEnabled(bool enabled);

    // Voice Output
    bool voiceOutputEnabled() const { return m_voiceOutputEnabled; }
    void setVoiceOutputEnabled(bool enabled);

    // Current Session ID
    QString currentSessionId() const { return m_currentSessionId; }
    void setCurrentSessionId(const QString &id);

    // Persistence
    void save();
    void load();

signals:
    void avatarLevelChanged(AvatarLevel level);
    void moodInfluenceChanged(MoodInfluenceLevel level);
    void videoSoundChanged(bool enabled);
    void voiceOutputChanged(bool enabled);

private:
    GirlfriendSettings();
    QString settingsPath() const;
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

    AvatarLevel m_avatarLevel = AvatarLevel::Level1_Belle;
    MoodInfluenceLevel m_moodInfluence = MoodInfluenceLevel::Medium;
    bool m_videoSoundEnabled = false;
    bool m_voiceOutputEnabled = true;
    QString m_currentSessionId;
};

#endif // GIRLFRIENDSETTINGS_H