#include "girlfriendsessionmanager.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QUuid>

#include "girlfriendsession.h"
#include "girlfriendsettings.h"

GirlfriendSessionManager* GirlfriendSessionManager::instance() {
    static GirlfriendSessionManager instance;
    return &instance;
}

GirlfriendSessionManager::GirlfriendSessionManager() : m_currentSession(nullptr) {
    loadAll();
}

GirlfriendSessionManager::~GirlfriendSessionManager() {
    saveAll();
    delete m_currentSession;
}

QString GirlfriendSessionManager::baseDir() const {
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return baseDir + "/girlfriend";
}

QString GirlfriendSessionManager::sessionsListPath() const {
    return baseDir() + "/sessions.json";
}

QString GirlfriendSessionManager::sessionDataPath(const QString& sessionId) const {
    return baseDir() + "/session_" + sessionId + ".json";
}

SessionMetadata GirlfriendSessionManager::currentSession() const {
    QString currentId = currentSessionId();
    for (const SessionMetadata& meta : m_sessions) {
        if (meta.id == currentId) {
            return meta;
        }
    }
    return SessionMetadata();
}

GirlfriendSession* GirlfriendSessionManager::currentSessionData() {
    return m_currentSession;
}

QString GirlfriendSessionManager::currentSessionId() const {
    return GirlfriendSettings::instance()->currentSessionId();
}

QString GirlfriendSessionManager::createNewSession(const QString& name) {
    // Ensure directory exists
    QDir dir(baseDir());
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Generate new session ID
    QString sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // Generate name if empty
    QString sessionName = name;
    if (sessionName.isEmpty()) {
        int sessionNum = m_sessions.size() + 1;
        sessionName = QString("Session %1").arg(sessionNum);
        // Ensure unique name
        bool nameExists = true;
        while (nameExists) {
            nameExists = false;
            for (const SessionMetadata& meta : m_sessions) {
                if (meta.name == sessionName) {
                    sessionNum++;
                    sessionName = QString("Session %1").arg(sessionNum);
                    nameExists = true;
                    break;
                }
            }
        }
    }

    // Create metadata
    SessionMetadata meta;
    meta.id = sessionId;
    meta.name = sessionName;
    QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    meta.createdAt = now;
    meta.lastUsedAt = now;

    // Add to sessions list
    m_sessions.append(meta);

    // Create empty session file
    GirlfriendSession session;
    session.saveToFile(sessionDataPath(sessionId));

    // Save sessions list
    saveSessionsList();

    // Set as current session
    GirlfriendSettings::instance()->setCurrentSessionId(sessionId);

    // Load the new session as current
    delete m_currentSession;
    m_currentSession = new GirlfriendSession();
    m_currentSession->loadFromFile(sessionDataPath(sessionId));

    emit sessionCreated(sessionId, sessionName);

    qDebug() << "Created new session:" << sessionId << "named:" << sessionName;
    return sessionId;
}

bool GirlfriendSessionManager::switchSession(const QString& sessionId) {
    // Check if session exists
    bool found = false;
    for (const SessionMetadata& meta : m_sessions) {
        if (meta.id == sessionId) {
            found = true;
            break;
        }
    }

    if (!found) {
        qDebug() << "Cannot switch to non-existent session:" << sessionId;
        return false;
    }

    // Save current session
    if (m_currentSession) {
        m_currentSession->saveToFile(sessionDataPath(currentSessionId()));
    }

    // Update last used timestamp
    updateLastUsed(sessionId);

    // Load new session
    delete m_currentSession;
    m_currentSession = new GirlfriendSession();
    m_currentSession->loadFromFile(sessionDataPath(sessionId));

    // Update settings
    GirlfriendSettings::instance()->setCurrentSessionId(sessionId);

    emit sessionSwitched(sessionId);

    qDebug() << "Switched to session:" << sessionId;
    return true;
}

bool GirlfriendSessionManager::deleteSession(const QString& sessionId) {
    // Cannot delete current session
    if (sessionId == currentSessionId()) {
        qDebug() << "Cannot delete current session:" << sessionId;
        return false;
    }

    // Find and remove from list
    int index = -1;
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions[i].id == sessionId) {
            index = i;
            break;
        }
    }

    if (index < 0) {
        qDebug() << "Cannot find session to delete:" << sessionId;
        return false;
    }

    // Remove from list
    m_sessions.removeAt(index);

    // Delete session file
    QString path = sessionDataPath(sessionId);
    QFile::remove(path);

    // Save sessions list
    saveSessionsList();

    emit sessionDeleted(sessionId);

    qDebug() << "Deleted session:" << sessionId;
    return true;
}

bool GirlfriendSessionManager::renameSession(const QString& sessionId, const QString& newName) {
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions[i].id == sessionId) {
            m_sessions[i].name = newName;
            saveSessionsList();
            emit sessionRenamed(sessionId, newName);
            qDebug() << "Renamed session:" << sessionId << "to:" << newName;
            return true;
        }
    }

    qDebug() << "Cannot find session to rename:" << sessionId;
    return false;
}

bool GirlfriendSessionManager::setSessionPinned(const QString& sessionId, bool pinned) {
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions[i].id == sessionId) {
            m_sessions[i].pinned = pinned;
            saveSessionsList();
            return true;
        }
    }
    return false;
}

void GirlfriendSessionManager::markSessionAutoNamed(const QString& sessionId) {
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions[i].id == sessionId) {
            m_sessions[i].autoNamed = true;
            saveSessionsList();
            return;
        }
    }
}

void GirlfriendSessionManager::saveAll() {
    saveSessionsList();
    if (m_currentSession) {
        m_currentSession->saveToFile(sessionDataPath(currentSessionId()));
    }
}

void GirlfriendSessionManager::loadAll() {
    loadSessionsList();

    QString currentId = currentSessionId();

    // If no current session ID, check if there are existing sessions
    if (currentId.isEmpty()) {
        if (m_sessions.isEmpty()) {
            // Create default session
            createNewSession("Default");
            return;
        } else {
            // Use first session
            currentId = m_sessions.first().id;
            GirlfriendSettings::instance()->setCurrentSessionId(currentId);
        }
    }

    // Verify current session exists
    bool found = false;
    for (const SessionMetadata& meta : m_sessions) {
        if (meta.id == currentId) {
            found = true;
            break;
        }
    }

    if (!found) {
        // Current session ID invalid, use first or create
        if (m_sessions.isEmpty()) {
            createNewSession("Default");
            return;
        } else {
            currentId = m_sessions.first().id;
            GirlfriendSettings::instance()->setCurrentSessionId(currentId);
        }
    }

    // Load current session
    delete m_currentSession;
    m_currentSession = new GirlfriendSession();
    m_currentSession->loadFromFile(sessionDataPath(currentId));

    qDebug() << "Loaded session manager with" << m_sessions.size()
             << "sessions, current:" << currentId;
}

void GirlfriendSessionManager::saveSessionsList() {
    QJsonArray sessionsArray;
    for (const SessionMetadata& meta : m_sessions) {
        QJsonObject obj;
        obj["id"] = meta.id;
        obj["name"] = meta.name;
        obj["createdAt"] = meta.createdAt;
        obj["lastUsedAt"] = meta.lastUsedAt;
        obj["pinned"] = meta.pinned;
        obj["autoNamed"] = meta.autoNamed;
        sessionsArray.append(obj);
    }

    QJsonObject root;
    root["sessions"] = sessionsArray;

    QJsonDocument doc(root);

    // Ensure directory exists
    QDir dir(baseDir());
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(sessionsListPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "Saved sessions list to:" << sessionsListPath();
    } else {
        qDebug() << "Failed to save sessions list:" << file.errorString();
    }
}

void GirlfriendSessionManager::loadSessionsList() {
    m_sessions.clear();

    QFile file(sessionsListPath());
    if (!file.exists()) {
        qDebug() << "Sessions list file does not exist, will create on save";
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open sessions list:" << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "Invalid sessions list JSON";
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray sessionsArray = root["sessions"].toArray();

    for (const QJsonValue& value : sessionsArray) {
        QJsonObject obj = value.toObject();
        SessionMetadata meta;
        meta.id = obj["id"].toString();
        meta.name = obj["name"].toString();
        meta.createdAt = obj["createdAt"].toString();
        meta.lastUsedAt = obj["lastUsedAt"].toString();
        meta.pinned = obj["pinned"].toBool(false);
        meta.autoNamed = obj["autoNamed"].toBool(false);
        m_sessions.append(meta);
    }

    qDebug() << "Loaded" << m_sessions.size() << "sessions from list";
}

void GirlfriendSessionManager::updateLastUsed(const QString& sessionId) {
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions[i].id == sessionId) {
            m_sessions[i].lastUsedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
            saveSessionsList();
            return;
        }
    }
}