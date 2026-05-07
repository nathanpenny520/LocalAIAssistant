#pragma once

#ifndef GIRLFRIENDSESSIONMANAGER_H
#define GIRLFRIENDSESSIONMANAGER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QJsonObject>

struct SessionMetadata {
    QString id;
    QString name;
    QString createdAt;
    QString lastUsedAt;
    bool pinned = false;
    bool autoNamed = false;
};

class GirlfriendSession;

class GirlfriendSessionManager : public QObject
{
    Q_OBJECT

public:
    static GirlfriendSessionManager* instance();

    // Session list access
    QVector<SessionMetadata> sessions() const { return m_sessions; }
    SessionMetadata currentSession() const;
    GirlfriendSession* currentSessionData();
    QString currentSessionId() const;
    int sessionCount() const { return m_sessions.size(); }

    // Session management
    QString createNewSession(const QString &name = "");
    bool switchSession(const QString &sessionId);
    bool deleteSession(const QString &sessionId);
    bool renameSession(const QString &sessionId, const QString &newName);
    bool setSessionPinned(const QString &sessionId, bool pinned);
    void markSessionAutoNamed(const QString &sessionId);

    // Persistence
    void saveAll();
    void loadAll();

signals:
    void sessionCreated(const QString &sessionId, const QString &name);
    void sessionSwitched(const QString &sessionId);
    void sessionDeleted(const QString &sessionId);
    void sessionRenamed(const QString &sessionId, const QString &newName);

private:
    GirlfriendSessionManager();
    ~GirlfriendSessionManager();

    // Prevent copying
    GirlfriendSessionManager(const GirlfriendSessionManager&) = delete;
    GirlfriendSessionManager& operator=(const GirlfriendSessionManager&) = delete;

    QString sessionsListPath() const;
    QString sessionDataPath(const QString &sessionId) const;
    QString baseDir() const;

    void saveSessionsList();
    void loadSessionsList();
    void updateLastUsed(const QString &sessionId);

    QVector<SessionMetadata> m_sessions;
    GirlfriendSession* m_currentSession = nullptr;
};

#endif // GIRLFRIENDSESSIONMANAGER_H