#include "sessionmanager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

SessionManager* SessionManager::m_instance = nullptr;

SessionManager::SessionManager(QObject* parent) : QObject(parent) {
    QSettings settings("LocalAIAssistant", "Settings");
    m_maxMessages = settings.value("sessionMaxMessages", 500).toInt();
    if (m_maxMessages > 0 && m_maxMessages < 10) {
        m_maxMessages = 10;
    }

    m_saveDebounceTimer = new QTimer(this);
    m_saveDebounceTimer->setSingleShot(true);
    m_saveDebounceTimer->setInterval(2000);
    connect(m_saveDebounceTimer, &QTimer::timeout, this, &SessionManager::saveSessionsToFile);

    createNewSession();
}

SessionManager* SessionManager::instance() {
    if (!m_instance) {
        m_instance = new SessionManager();
    }
    return m_instance;
}

ChatSession& SessionManager::currentSession() {
    return m_sessions[m_currentSessionId];
}

void SessionManager::createNewSession(const QString& title) {
    QString sessionTitle = title.isEmpty() ? QStringLiteral("新对话") : title;
    ChatSession newSession(sessionTitle);
    m_sessions[newSession.id] = newSession;
    m_currentSessionId = newSession.id;

    // Save last session ID for CLI/GUI sync
    QSettings settings("LocalAIAssistant", "Settings");
    settings.setValue("lastSessionId", m_currentSessionId);

    emit sessionChanged(m_currentSessionId);
}

void SessionManager::switchToSession(const QString& sessionId) {
    if (m_sessions.contains(sessionId)) {
        m_currentSessionId = sessionId;

        // Save last session ID for CLI/GUI sync
        QSettings settings("LocalAIAssistant", "Settings");
        settings.setValue("lastSessionId", m_currentSessionId);

        emit sessionChanged(m_currentSessionId);
    }
}

void SessionManager::addMessageToCurrentSession(const QString& role, const QString& content) {
    m_sessions[m_currentSessionId].messages.append(ChatMessage(role, content));
    truncateSession(m_currentSessionId);
    m_saveDebounceTimer->start();
    emit sessionChanged(m_currentSessionId);
}

void SessionManager::addMessageToCurrentSession(const QString& role, const QString& content,
                                                const QVector<FileAttachment>& attachments) {
    ChatMessage msg(role, content);
    msg.attachments = attachments;
    m_sessions[m_currentSessionId].messages.append(msg);
    truncateSession(m_currentSessionId);
    m_saveDebounceTimer->start();
    emit sessionChanged(m_currentSessionId);
}

void SessionManager::addMessageToSession(const QString& sessionId, const QString& role,
                                         const QString& content) {
    if (m_sessions.contains(sessionId)) {
        m_sessions[sessionId].messages.append(ChatMessage(role, content));
        truncateSession(sessionId);
        m_saveDebounceTimer->start();
        emit sessionChanged(sessionId);
    }
}

void SessionManager::addMessageToSession(const QString& sessionId, const ChatMessage& message) {
    if (m_sessions.contains(sessionId)) {
        m_sessions[sessionId].messages.append(message);
        truncateSession(sessionId);
        m_saveDebounceTimer->start();
        emit sessionChanged(sessionId);
    }
}

void SessionManager::updateSessionTitle(const QString& sessionId, const QString& title) {
    if (m_sessions.contains(sessionId)) {
        m_sessions[sessionId].title = title;
        m_sessions[sessionId].autoNamed = true;
        emit sessionChanged(sessionId);
    }
}

void SessionManager::setSessionPinned(const QString& sessionId, bool pinned) {
    if (m_sessions.contains(sessionId)) {
        m_sessions[sessionId].pinned = pinned;
        emit sessionChanged(sessionId);
    }
}

void SessionManager::removeSession(const QString& sessionId) {
    m_sessions.remove(sessionId);
}

void SessionManager::truncateSession(const QString& sessionId) {
    if (m_maxMessages <= 0 || !m_sessions.contains(sessionId)) {
        return;
    }

    ChatSession& session = m_sessions[sessionId];
    int removed = session.messages.size() - m_maxMessages;
    if (removed <= 0) {
        return;
    }

    session.messages.remove(0, removed);

    // Prepend system message to inform user about truncation
    QString info = QString("Session auto-trimmed, keeping last %1 messages").arg(m_maxMessages);
    ChatMessage truncMsg("system", info);
    truncMsg.messageType = ChatMessage::SystemNotification;
    session.messages.prepend(truncMsg);
}

void SessionManager::setMaxMessages(int limit) {
    if (limit > 0 && limit < 10) {
        limit = 10;
    }
    m_maxMessages = limit;

    QSettings settings("LocalAIAssistant", "Settings");
    settings.setValue("sessionMaxMessages", limit);

    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        truncateSession(it.key());
    }
}

QString SessionManager::getStorageFilePath() const {
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);
    if (!dir.exists()) {
        dir.mkpath(dataPath);
    }
    return dataPath + "/chat_history.json";
}

void SessionManager::saveSessionsToFile() {
    QString filePath = getStorageFilePath();
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QJsonArray sessionsArray;
    for (auto it = m_sessions.constBegin(); it != m_sessions.constEnd(); ++it) {
        const ChatSession& session = it.value();

        QJsonObject sessionObj;
        sessionObj["id"] = session.id;
        sessionObj["title"] = session.title;
        sessionObj["pinned"] = session.pinned;
        sessionObj["autoNamed"] = session.autoNamed;

        QJsonArray messagesArray;
        for (const auto& msg : session.messages) {
            QJsonObject msgObj;
            msgObj["role"] = msg.role;
            msgObj["content"] = msg.content;
            if (msg.isAgentLoopInjected)
                msgObj["isAgentLoopInjected"] = true;
            if (msg.messageType != ChatMessage::Normal)
                msgObj["messageType"] = (msg.messageType == ChatMessage::SystemNotification)
                        ? QStringLiteral("SystemNotification") : QStringLiteral("Normal");

            // Serialize attachments
            if (!msg.attachments.isEmpty()) {
                QJsonArray attachmentsArray;
                for (const auto& attachment : msg.attachments) {
                    QJsonObject attachObj;
                    attachObj["path"] = attachment.path;
                    attachObj["type"] = attachment.type;
                    attachObj["mimeType"] = attachment.mimeType;
                    attachObj["content"] = attachment.content;
                    attachObj["size"] = attachment.size;
                    attachmentsArray.append(attachObj);
                }
                msgObj["attachments"] = attachmentsArray;
            }

            messagesArray.append(msgObj);
        }
        sessionObj["messages"] = messagesArray;

        sessionsArray.append(sessionObj);
    }

    QJsonDocument doc(sessionsArray);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

void SessionManager::loadSessionsFromFile() {
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
    for (const auto& sessionVal : sessionsArray) {
        if (!sessionVal.isObject()) {
            continue;
        }

        QJsonObject sessionObj = sessionVal.toObject();

        ChatSession session;
        session.id = sessionObj["id"].toString();
        session.title = sessionObj["title"].toString(QStringLiteral("新对话"));
        session.pinned = sessionObj["pinned"].toBool(false);
        session.autoNamed = sessionObj["autoNamed"].toBool(false);

        QJsonArray messagesArray = sessionObj["messages"].toArray();
        for (const auto& msgVal : messagesArray) {
            if (!msgVal.isObject()) {
                continue;
            }
            QJsonObject msgObj = msgVal.toObject();
            ChatMessage msg(msgObj["role"].toString(), msgObj["content"].toString());
            msg.isAgentLoopInjected = msgObj.value("isAgentLoopInjected").toBool(false);
            QString mtStr = msgObj.value("messageType").toString();
            if (mtStr == QStringLiteral("SystemNotification"))
                msg.messageType = ChatMessage::SystemNotification;

            // Deserialize attachments
            if (msgObj.contains("attachments")) {
                QJsonArray attachmentsArray = msgObj["attachments"].toArray();
                for (const auto& attachVal : attachmentsArray) {
                    if (!attachVal.isObject()) {
                        continue;
                    }
                    QJsonObject attachObj = attachVal.toObject();
                    FileAttachment attachment;
                    attachment.path = attachObj["path"].toString();
                    attachment.type = attachObj["type"].toString();
                    attachment.mimeType = attachObj["mimeType"].toString();
                    attachment.content = attachObj["content"].toString();
                    attachment.size = attachObj["size"].toVariant().toLongLong();
                    msg.attachments.append(attachment);
                }
            }

            session.messages.append(msg);
        }

        m_sessions[session.id] = session;
    }

    // Restore last session from settings (CLI/GUI sync)
    QSettings settings("LocalAIAssistant", "Settings");
    QString lastSessionId = settings.value("lastSessionId").toString();

    if (!m_sessions.isEmpty()) {
        // Prefer last session if it exists
        if (m_sessions.contains(lastSessionId)) {
            m_currentSessionId = lastSessionId;
        } else {
            // Fallback to first session
            m_currentSessionId = m_sessions.constBegin().key();
        }
    } else {
        createNewSession();
    }
}