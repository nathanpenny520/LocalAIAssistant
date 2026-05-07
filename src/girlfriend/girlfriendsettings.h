#pragma once

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

    // XFYUN Voice Credentials (user-editable via settings UI)
    QString xfyunAppId() const { return m_xfyunAppId; }
    void setXfyunAppId(const QString &id);

    QString xfyunApiKey() const { return m_xfyunApiKey; }
    void setXfyunApiKey(const QString &key);

    QString xfyunApiSecret() const { return m_xfyunApiSecret; }
    void setXfyunApiSecret(const QString &secret);

    QString xfyunAsrUrl() const { return m_xfyunAsrUrl; }
    void setXfyunAsrUrl(const QString &url);

    QString xfyunTtsUrl() const { return m_xfyunTtsUrl; }
    void setXfyunTtsUrl(const QString &url);

    QString xfyunVoiceType() const { return m_xfyunVoiceType; }
    void setXfyunVoiceType(const QString &type);

    // Check if credentials are configured
    bool isXfyunConfigured() const;

    // Persistence
    void save();
    void load();

signals:
    void avatarLevelChanged(AvatarLevel level);
    void moodInfluenceChanged(MoodInfluenceLevel level);
    void videoSoundChanged(bool enabled);
    void voiceOutputChanged(bool enabled);
    void currentSessionIdChanged(const QString &id);
    void xfyunCredentialsChanged();

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

    // XFYUN credentials
    QString m_xfyunAppId;
    QString m_xfyunApiKey;
    QString m_xfyunApiSecret;
    QString m_xfyunAsrUrl;
    QString m_xfyunTtsUrl;
    QString m_xfyunVoiceType;
};

#endif // GIRLFRIENDSETTINGS_H
