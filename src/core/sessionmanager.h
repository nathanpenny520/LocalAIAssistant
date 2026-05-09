/**
 * @file sessionmanager.h
 * @brief Chat session CRUD, JSON persistence, and session size limits with auto-truncation.
 */
#pragma once

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QJsonArray>
#include <QMap>
#include <QObject>

#include "datamodels.h"

class SessionManager : public QObject {
    Q_OBJECT

public:
    static SessionManager* instance();

    ChatSession& currentSession();
    const QString& currentSessionId() const {
        return m_currentSessionId;
    }

    void createNewSession(const QString& title = QString());
    void switchToSession(const QString& sessionId);
    void addMessageToCurrentSession(const QString& role, const QString& content);
    void addMessageToCurrentSession(const QString& role, const QString& content,
                                    const QVector<FileAttachment>& attachments);
    void addMessageToSession(const QString& sessionId, const QString& role,
                             const QString& content);  // Add a message directly to a specific session
    void updateSessionTitle(const QString& sessionId, const QString& title);
    void setSessionPinned(const QString& sessionId, bool pinned);
    void removeSession(const QString& sessionId);

    const QMap<QString, ChatSession>& allSessions() const {
        return m_sessions;
    }

    void saveSessionsToFile();
    void loadSessionsFromFile();

    int maxMessages() const {
        return m_maxMessages;
    }
    void setMaxMessages(int limit);

signals:
    void sessionChanged(const QString& sessionId);

private:
    SessionManager(QObject* parent = nullptr);

    static SessionManager* m_instance;
    QMap<QString, ChatSession> m_sessions;
    QString m_currentSessionId;
    int m_maxMessages;
    QString getStorageFilePath() const;
    void truncateSession(const QString& sessionId);
};

#endif