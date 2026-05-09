#include "girlfriendsession.h"

#include <QDebug>
#include <QJsonDocument>

GirlfriendSession::GirlfriendSession()
        : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
        , m_currentEmotion("default")
        , m_mood(0.6)  // 默认心情值
        , m_messages() {
}

void GirlfriendSession::addMessage(const QString& role, const QString& content,
                                   const QString& emotion) {
    GirlfriendMessage msg(role, content, emotion);
    m_messages.append(msg);
}

void GirlfriendSession::setCurrentEmotion(const QString& emotion) {
    m_currentEmotion = emotion;
}

void GirlfriendSession::setMood(double mood) {
    m_mood = qBound(0.0, mood, 1.0);  // 确保在有效范围内
}

void GirlfriendSession::clearMessages() {
    m_messages.clear();
    m_currentEmotion = "default";
    // 注意：清除消息不重置心情值，保留心情状态
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
        qDebug() << "GirlfriendSession saved to:" << path;
    } else {
        qDebug() << "Failed to save GirlfriendSession:" << file.errorString();
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
            qDebug() << "GirlfriendSession loaded from:" << path;
        }
    }
}

QJsonObject GirlfriendSession::toJson() const {
    QJsonObject json;
    json["id"] = m_id;
    json["currentEmotion"] = m_currentEmotion;
    json["mood"] = m_mood;  // 保存心情值

    QJsonArray messagesArray;
    for (const GirlfriendMessage& msg : m_messages) {
        QJsonObject msgObj;
        msgObj["role"] = msg.role;
        msgObj["content"] = msg.content;
        msgObj["emotion"] = msg.emotion;
        messagesArray.append(msgObj);
    }
    json["messages"] = messagesArray;

    return json;
}

void GirlfriendSession::fromJson(const QJsonObject& json) {
    m_id = json["id"].toString();
    m_currentEmotion = json["currentEmotion"].toString("default");
    m_mood = json["mood"].toDouble(0.6);  // 加载心情值，默认0.6

    m_messages.clear();
    QJsonArray messagesArray = json["messages"].toArray();
    for (const QJsonValue& value : messagesArray) {
        QJsonObject msgObj = value.toObject();
        GirlfriendMessage msg;
        msg.role = msgObj["role"].toString();
        msg.content = msgObj["content"].toString();
        msg.emotion = msgObj["emotion"].toString("default");
        m_messages.append(msg);
    }
}