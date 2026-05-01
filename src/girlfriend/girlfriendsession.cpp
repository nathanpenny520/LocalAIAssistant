#include "girlfriendsession.h"
#include <QDebug>
#include <QJsonDocument>

GirlfriendSession::GirlfriendSession()
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_currentEmotion("default")
    , m_messages()
{
}

void GirlfriendSession::addMessage(const QString &role, const QString &content, const QString &emotion)
{
    GirlfriendMessage msg(role, content, emotion);
    m_messages.append(msg);
}

void GirlfriendSession::setCurrentEmotion(const QString &emotion)
{
    m_currentEmotion = emotion;
}

void GirlfriendSession::clearMessages()
{
    m_messages.clear();
    m_currentEmotion = "default";
}

QString GirlfriendSession::storagePath()
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString girlfriendDir = baseDir + "/girlfriend";
    QDir dir(girlfriendDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return girlfriendDir + "/girlfriend_session.json";
}

void GirlfriendSession::saveToFile()
{
    QJsonObject json = toJson();
    QJsonDocument doc(json);

    QString path = storagePath();
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "GirlfriendSession saved to:" << path;
    } else {
        qDebug() << "Failed to save GirlfriendSession:" << file.errorString();
    }
}

void GirlfriendSession::loadFromFile()
{
    QString path = storagePath();
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

QJsonObject GirlfriendSession::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["currentEmotion"] = m_currentEmotion;

    QJsonArray messagesArray;
    for (const GirlfriendMessage &msg : m_messages) {
        QJsonObject msgObj;
        msgObj["role"] = msg.role;
        msgObj["content"] = msg.content;
        msgObj["emotion"] = msg.emotion;
        messagesArray.append(msgObj);
    }
    json["messages"] = messagesArray;

    return json;
}

void GirlfriendSession::fromJson(const QJsonObject &json)
{
    m_id = json["id"].toString();
    m_currentEmotion = json["currentEmotion"].toString("default");

    m_messages.clear();
    QJsonArray messagesArray = json["messages"].toArray();
    for (const QJsonValue &value : messagesArray) {
        QJsonObject msgObj = value.toObject();
        GirlfriendMessage msg;
        msg.role = msgObj["role"].toString();
        msg.content = msgObj["content"].toString();
        msg.emotion = msgObj["emotion"].toString("default");
        m_messages.append(msg);
    }
}