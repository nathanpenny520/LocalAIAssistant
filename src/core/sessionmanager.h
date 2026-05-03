#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "datamodels.h"
#include <QMap>
#include <QObject>
#include <QJsonArray>

class SessionManager : public QObject
{
    Q_OBJECT

public:
    static SessionManager* instance();

    ChatSession& currentSession();
    const QString& currentSessionId() const { return m_currentSessionId; }

    void createNewSession(const QString &title = QString());
    void switchToSession(const QString &sessionId);
    void addMessageToCurrentSession(const QString &role, const QString &content);
    void addMessageToCurrentSession(const QString &role, const QString &content, const QVector<FileAttachment> &attachments);
    void addMessageToSession(const QString &sessionId, const QString &role, const QString &content);  // 直接向指定会话添加消息
    void updateSessionTitle(const QString &sessionId, const QString &title);
    void removeSession(const QString &sessionId);

    const QMap<QString, ChatSession>& allSessions() const { return m_sessions; }

    void saveSessionsToFile();
    void loadSessionsFromFile();

signals:
    void sessionChanged(const QString &sessionId);

private:
    SessionManager(QObject *parent = nullptr);

    static SessionManager *m_instance;
    QMap<QString, ChatSession> m_sessions;
    QString m_currentSessionId;
    QString getStorageFilePath() const;
};

#endif