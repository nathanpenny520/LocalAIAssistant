#include "girlfriendsettings.h"

#include <QCoreApplication>
#include <QJsonDocument>

GirlfriendSettings* GirlfriendSettings::instance() {
    static GirlfriendSettings s_instance;
    return &s_instance;
}

GirlfriendSettings::GirlfriendSettings() : QObject(nullptr) {
    load();
}

QString GirlfriendSettings::settingsPath() const {
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString girlfriendDir = baseDir + "/girlfriend";
    QDir dir(girlfriendDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return girlfriendDir + "/settings.json";
}

QString GirlfriendSettings::avatarLevelPath() const {
    QString appDir = QCoreApplication::applicationDirPath();
    QString basePath;

#ifdef Q_OS_MACOS
    basePath = QDir::cleanPath(appDir + "/../Resources/girlfriend");
#else
    basePath = QDir::cleanPath(appDir + "/girlfriend");
#endif

    // Fallback paths
    if (!QDir(basePath).exists()) {
        basePath = "girlfriend";
    }
    if (!QDir(basePath).exists()) {
        basePath = "sourcecode-ai-assistant/resources/girlfriend";
    }

    switch (m_avatarLevel) {
        case AvatarLevel::Level1_Belle:
            return basePath + "/level-1-belle";
        case AvatarLevel::Level2_Hot:
            return basePath + "/level-2-hot";
        case AvatarLevel::Level3_Hotter:
            return basePath + "/level-3-hotter";
    }
    return basePath + "/level-1-belle";
}

void GirlfriendSettings::setAvatarLevel(AvatarLevel level) {
    if (m_avatarLevel != level) {
        m_avatarLevel = level;
        save();
        emit avatarLevelChanged(level);
    }
}

void GirlfriendSettings::setMoodInfluence(MoodInfluenceLevel level) {
    if (m_moodInfluence != level) {
        m_moodInfluence = level;
        save();
        emit moodInfluenceChanged(level);
    }
}

void GirlfriendSettings::setVideoSoundEnabled(bool enabled) {
    if (m_videoSoundEnabled != enabled) {
        m_videoSoundEnabled = enabled;
        save();
        emit videoSoundChanged(enabled);
    }
}

void GirlfriendSettings::setVoiceOutputEnabled(bool enabled) {
    if (m_voiceOutputEnabled != enabled) {
        m_voiceOutputEnabled = enabled;
        save();
        emit voiceOutputChanged(enabled);
    }
}

void GirlfriendSettings::setCurrentSessionId(const QString& id) {
    if (m_currentSessionId != id) {
        m_currentSessionId = id;
        save();
        emit currentSessionIdChanged(id);
    }
}

void GirlfriendSettings::setXfyunAppId(const QString& id) {
    if (m_xfyunAppId != id) {
        m_xfyunAppId = id;
        save();
        emit xfyunCredentialsChanged();
    }
}

void GirlfriendSettings::setXfyunApiKey(const QString& key) {
    if (m_xfyunApiKey != key) {
        m_xfyunApiKey = key;
        save();
        emit xfyunCredentialsChanged();
    }
}

void GirlfriendSettings::setXfyunApiSecret(const QString& secret) {
    if (m_xfyunApiSecret != secret) {
        m_xfyunApiSecret = secret;
        save();
        emit xfyunCredentialsChanged();
    }
}

void GirlfriendSettings::setXfyunAsrUrl(const QString& url) {
    if (m_xfyunAsrUrl != url) {
        m_xfyunAsrUrl = url;
        save();
        emit xfyunCredentialsChanged();
    }
}

void GirlfriendSettings::setXfyunTtsUrl(const QString& url) {
    if (m_xfyunTtsUrl != url) {
        m_xfyunTtsUrl = url;
        save();
        emit xfyunCredentialsChanged();
    }
}

void GirlfriendSettings::setXfyunVoiceType(const QString& type) {
    if (m_xfyunVoiceType != type) {
        m_xfyunVoiceType = type;
        save();
        emit xfyunCredentialsChanged();
    }
}

bool GirlfriendSettings::isXfyunConfigured() const {
    return !m_xfyunAppId.isEmpty() && !m_xfyunApiKey.isEmpty() && !m_xfyunApiSecret.isEmpty();
}

void GirlfriendSettings::save() {
    QJsonObject json = toJson();
    QJsonDocument doc(json);

    QString path = settingsPath();
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    } else {
        qWarning() << "GirlfriendSettings failed to open file for writing:" << path
                   << file.errorString();
    }
}

void GirlfriendSettings::load() {
    QString path = settingsPath();
    QFile file(path);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isObject()) {
            fromJson(doc.object());
        }
    }
}

QJsonObject GirlfriendSettings::toJson() const {
    QJsonObject json;
    json["avatarLevel"] = static_cast<int>(m_avatarLevel);
    json["moodInfluence"] = static_cast<int>(m_moodInfluence);
    json["videoSoundEnabled"] = m_videoSoundEnabled;
    json["voiceOutputEnabled"] = m_voiceOutputEnabled;
    json["currentSessionId"] = m_currentSessionId;

    // XFYUN credentials
    if (!m_xfyunAppId.isEmpty()) json["xfyun_app_id"] = m_xfyunAppId;
    if (!m_xfyunApiKey.isEmpty()) json["xfyun_api_key"] = m_xfyunApiKey;
    if (!m_xfyunApiSecret.isEmpty()) json["xfyun_api_secret"] = m_xfyunApiSecret;
    if (!m_xfyunAsrUrl.isEmpty()) json["xfyun_asr_url"] = m_xfyunAsrUrl;
    if (!m_xfyunTtsUrl.isEmpty()) json["xfyun_tts_url"] = m_xfyunTtsUrl;
    if (!m_xfyunVoiceType.isEmpty()) json["xfyun_voice_type"] = m_xfyunVoiceType;

    return json;
}

void GirlfriendSettings::fromJson(const QJsonObject& json) {
    int avatarLevelInt = json["avatarLevel"].toInt(0);
    if (avatarLevelInt < 0 || avatarLevelInt > 2) {
        qWarning() << "GirlfriendSettings: Invalid avatarLevel value" << avatarLevelInt
                   << ", defaulting to 0";
        avatarLevelInt = 0;
    }
    m_avatarLevel = static_cast<AvatarLevel>(avatarLevelInt);

    int moodInfluenceInt = json["moodInfluence"].toInt(1);
    if (moodInfluenceInt < 0 || moodInfluenceInt > 2) {
        qWarning() << "GirlfriendSettings: Invalid moodInfluence value" << moodInfluenceInt
                   << ", defaulting to 1";
        moodInfluenceInt = 1;
    }
    m_moodInfluence = static_cast<MoodInfluenceLevel>(moodInfluenceInt);

    m_videoSoundEnabled = json["videoSoundEnabled"].toBool(false);
    m_voiceOutputEnabled = json["voiceOutputEnabled"].toBool(true);
    m_currentSessionId = json["currentSessionId"].toString();

    // XFYUN credentials
    m_xfyunAppId = json["xfyun_app_id"].toString();
    m_xfyunApiKey = json["xfyun_api_key"].toString();
    m_xfyunApiSecret = json["xfyun_api_secret"].toString();
    m_xfyunAsrUrl = json["xfyun_asr_url"].toString();
    m_xfyunTtsUrl = json["xfyun_tts_url"].toString();
    m_xfyunVoiceType = json["xfyun_voice_type"].toString();
}
