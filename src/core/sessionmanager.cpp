#include "sessionmanager.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

SessionManager* SessionManager::m_instance = nullptr;

SessionManager::SessionManager(QObject *parent)
    : QObject(parent)
{
    createNewSession("新对话");
}

SessionManager* SessionManager::instance()
{
    if (!m_instance) {
        m_instance = new SessionManager();
    }
    return m_instance;
}

ChatSession& SessionManager::currentSession()
{
    return m_sessions[m_currentSessionId];
}

void SessionManager::createNewSession(const QString &title)
{
    ChatSession newSession(title);
    m_sessions[newSession.id] = newSession;
    m_currentSessionId = newSession.id;
    emit sessionChanged(m_currentSessionId);
}

void SessionManager::switchToSession(const QString &sessionId)
{
    if (m_sessions.contains(sessionId)) {
        m_currentSessionId = sessionId;
        emit sessionChanged(m_currentSessionId);
    }
}

void SessionManager::addMessageToCurrentSession(const QString &role, const QString &content)
{
    m_sessions[m_currentSessionId].messages.append(ChatMessage(role, content));
    emit sessionChanged(m_currentSessionId);
}

void SessionManager::removeSession(const QString &sessionId)
{
    m_sessions.remove(sessionId);
}

QString SessionManager::getStorageFilePath() const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);
    if (!dir.exists()) {
        dir.mkpath(dataPath);
    }
    return dataPath + "/chat_history.json";
}

void SessionManager::saveSessionsToFile()
{
    QString filePath = getStorageFilePath();
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QJsonArray sessionsArray;
    for (auto it = m_sessions.constBegin(); it != m_sessions.constEnd(); ++it) {
        const ChatSession &session = it.value();

        QJsonObject sessionObj;
        sessionObj["id"] = session.id;
        sessionObj["title"] = session.title;

        QJsonArray messagesArray;
        for (const auto &msg : session.messages) {
            QJsonObject msgObj;
            msgObj["role"] = msg.role;
            msgObj["content"] = msg.content;
            messagesArray.append(msgObj);
        }
        sessionObj["messages"] = messagesArray;

        sessionsArray.append(sessionObj);
    }

    QJsonDocument doc(sessionsArray);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

void SessionManager::loadSessionsFromFile()
{
    QString filePath = getStorageFilePath();
    QFile file(filePath);

    if (!file.exists()) {
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isArray()) {
        return;
    }

    m_sessions.clear();

    QJsonArray sessionsArray = doc.array();
    for (const auto &sessionVal : sessionsArray) {
        if (!sessionVal.isObject()) {
            continue;
        }

        QJsonObject sessionObj = sessionVal.toObject();

        ChatSession session;
        session.id = sessionObj["id"].toString();
        session.title = sessionObj["title"].toString("新对话");

        QJsonArray messagesArray = sessionObj["messages"].toArray();
        for (const auto &msgVal : messagesArray) {
            if (!msgVal.isObject()) {
                continue;
            }
            QJsonObject msgObj = msgVal.toObject();
            session.messages.append(ChatMessage(
                msgObj["role"].toString(),
                msgObj["content"].toString()
            ));
        }

        m_sessions[session.id] = session;
    }

    if (!m_sessions.isEmpty()) {
        m_currentSessionId = m_sessions.constBegin().key();
    } else {
        createNewSession("新对话");
    }
}