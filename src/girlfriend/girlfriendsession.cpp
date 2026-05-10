#include "girlfriendsession.h"

#include <QJsonDocument>
#include <QSettings>

GirlfriendSession::GirlfriendSession()
        : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
        , m_currentEmotion("default")
        , m_mood(0.6)
        , m_messages() {
    QSettings settings("LocalAIAssistant", "Settings");
    m_maxMessages = settings.value("sessionMaxMessages", 500).toInt();
    if (m_maxMessages > 0 && m_maxMessages < 10) {
        m_maxMessages = 10;
    }
}

void GirlfriendSession::addMessage(const QString& role, const QString& content,
                                   const QString& emotion) {
    GirlfriendMessage msg(role, content, emotion);
    m_messages.append(msg);
    truncateMessages();
}

void GirlfriendSession::setCurrentEmotion(const QString& emotion) {
    m_currentEmotion = emotion;
}

void GirlfriendSession::setMood(double mood) {
    m_mood = qBound(0.0, mood, 1.0);
}

void GirlfriendSession::setMaxMessages(int limit) {
    if (limit > 0 && limit < 10) {
        limit = 10;
    }
    m_maxMessages = limit;
    QSettings settings("LocalAIAssistant", "Settings");
    settings.setValue("sessionMaxMessages", limit);
    truncateMessages();
}

void GirlfriendSession::truncateMessages() {
    if (m_maxMessages <= 0) return;

    int removed = m_messages.size() - m_maxMessages;
    if (removed <= 0) return;

    m_messages.remove(0, removed);

    GirlfriendMessage info("system",
                           QString("Session auto-trimmed, keeping last %1 messages").arg(m_maxMessages));
    info.isSystemNotification = true;
    m_messages.prepend(info);
}

void GirlfriendSession::clearMessages() {
    m_messages.clear();
    m_currentEmotion = "default";
    // clearMessages preserves mood state — only messages and emotion are reset
}

QString GirlfriendSession::storagePath() {
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString girlfriendDir = baseDir + "/girlfriend";
    QDir dir(girlfriendDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return girlfriendDir + "/girlfriend_session.json";
}

void GirlfriendSession::saveToFile() {
    saveToFile(storagePath());
}

void GirlfriendSession::loadFromFile() {
    loadFromFile(storagePath());
}

void GirlfriendSession::saveToFile(const QString& path) {
    QJsonObject json = toJson();
    QJsonDocument doc(json);

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    } else {
    }
}

void GirlfriendSession::loadFromFile(const QString& path) {
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

QJsonObject GirlfriendSession::toJson() const {
    QJsonObject json;
    json["id"] = m_id;
    json["currentEmotion"] = m_currentEmotion;
    json["mood"] = m_mood;

    QJsonArray messagesArray;
    for (const GirlfriendMessage& msg : m_messages) {
        QJsonObject msgObj;
        msgObj["role"] = msg.role;
        msgObj["content"] = msg.content;
        msgObj["emotion"] = msg.emotion;
        if (msg.isSystemNotification)
            msgObj["isSystemNotification"] = true;
        messagesArray.append(msgObj);
    }
    json["messages"] = messagesArray;

    return json;
}

void GirlfriendSession::fromJson(const QJsonObject& json) {
    m_id = json["id"].toString();
    m_currentEmotion = json["currentEmotion"].toString("default");
    m_mood = json["mood"].toDouble(0.6);

    m_messages.clear();
    QJsonArray messagesArray = json["messages"].toArray();
    for (const QJsonValue& value : messagesArray) {
        QJsonObject msgObj = value.toObject();
        GirlfriendMessage msg;
        msg.role = msgObj["role"].toString();
        msg.content = msgObj["content"].toString();
        msg.emotion = msgObj["emotion"].toString("default");
        msg.isSystemNotification = msgObj["isSystemNotification"].toBool(false);
        m_messages.append(msg);
    }
}