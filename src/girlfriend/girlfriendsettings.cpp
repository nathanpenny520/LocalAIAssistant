#include "girlfriendsettings.h"
#include <QDebug>
#include <QJsonDocument>
#include <QCoreApplication>

GirlfriendSettings* GirlfriendSettings::instance()
{
    static GirlfriendSettings s_instance;
    return &s_instance;
}

GirlfriendSettings::GirlfriendSettings()
    : QObject(nullptr)
{
    load();
}

QString GirlfriendSettings::settingsPath() const
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString girlfriendDir = baseDir + "/girlfriend";
    QDir dir(girlfriendDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return girlfriendDir + "/settings.json";
}

QString GirlfriendSettings::avatarLevelPath() const
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString basePath;

#ifdef Q_OS_MACOS
    basePath = QDir::cleanPath(appDir + "/../Resources/AIGirlfriend");
#else
    basePath = QDir::cleanPath(appDir + "/AIGirlfriend");
#endif

    // Fallback paths
    if (!QDir(basePath).exists()) {
        basePath = "AIGirlfriend";
    }
    if (!QDir(basePath).exists()) {
        basePath = "sourcecode-ai-assistant/AIGirlfriend";
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

void GirlfriendSettings::setAvatarLevel(AvatarLevel level)
{
    if (m_avatarLevel != level) {
        m_avatarLevel = level;
        save();
        emit avatarLevelChanged(level);
    }
}

void GirlfriendSettings::setMoodInfluence(MoodInfluenceLevel level)
{
    if (m_moodInfluence != level) {
        m_moodInfluence = level;
        save();
        emit moodInfluenceChanged(level);
    }
}

void GirlfriendSettings::setVideoSoundEnabled(bool enabled)
{
    if (m_videoSoundEnabled != enabled) {
        m_videoSoundEnabled = enabled;
        save();
        emit videoSoundChanged(enabled);
    }
}

void GirlfriendSettings::setVoiceOutputEnabled(bool enabled)
{
    if (m_voiceOutputEnabled != enabled) {
        m_voiceOutputEnabled = enabled;
        save();
        emit voiceOutputChanged(enabled);
    }
}

void GirlfriendSettings::setCurrentSessionId(const QString &id)
{
    if (m_currentSessionId != id) {
        m_currentSessionId = id;
        save();
        emit currentSessionIdChanged(id);
    }
}

void GirlfriendSettings::save()
{
    QJsonObject json = toJson();
    QJsonDocument doc(json);

    QString path = settingsPath();
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "GirlfriendSettings saved to:" << path;
    } else {
        qWarning() << "GirlfriendSettings failed to open file for writing:" << path << file.errorString();
    }
}

void GirlfriendSettings::load()
{
    QString path = settingsPath();
    QFile file(path);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isObject()) {
            fromJson(doc.object());
            qDebug() << "GirlfriendSettings loaded from:" << path;
        }
    }
}

QJsonObject GirlfriendSettings::toJson() const
{
    QJsonObject json;
    json["avatarLevel"] = static_cast<int>(m_avatarLevel);
    json["moodInfluence"] = static_cast<int>(m_moodInfluence);
    json["videoSoundEnabled"] = m_videoSoundEnabled;
    json["voiceOutputEnabled"] = m_voiceOutputEnabled;
    json["currentSessionId"] = m_currentSessionId;
    return json;
}

void GirlfriendSettings::fromJson(const QJsonObject &json)
{
    int avatarLevelInt = json["avatarLevel"].toInt(0);
    if (avatarLevelInt < 0 || avatarLevelInt > 2) {
        qWarning() << "GirlfriendSettings: Invalid avatarLevel value" << avatarLevelInt << ", defaulting to 0";
        avatarLevelInt = 0;
    }
    m_avatarLevel = static_cast<AvatarLevel>(avatarLevelInt);

    int moodInfluenceInt = json["moodInfluence"].toInt(1);
    if (moodInfluenceInt < 0 || moodInfluenceInt > 2) {
        qWarning() << "GirlfriendSettings: Invalid moodInfluence value" << moodInfluenceInt << ", defaulting to 1";
        moodInfluenceInt = 1;
    }
    m_moodInfluence = static_cast<MoodInfluenceLevel>(moodInfluenceInt);

    m_videoSoundEnabled = json["videoSoundEnabled"].toBool(false);
    m_voiceOutputEnabled = json["voiceOutputEnabled"].toBool(true);
    m_currentSessionId = json["currentSessionId"].toString();
}