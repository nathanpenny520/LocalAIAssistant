#ifndef GIRLFRIENDSESSION_H
#define GIRLFRIENDSESSION_H

#include <QString>
#include <QVector>
#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QStandardPaths>
#include <QDir>

struct GirlfriendMessage
{
    QString role;       // "user" 或 "girlfriend"
    QString content;
    QString emotion;    // 当前情绪状态：happy, shy, love, hate, sad, angry, afraid, awaiting, studying, default

    GirlfriendMessage() : emotion("default") {}
    GirlfriendMessage(const QString &r, const QString &c, const QString &e = "default")
        : role(r), content(c), emotion(e) {}
};

class GirlfriendSession
{
public:
    GirlfriendSession();

    QString id() const { return m_id; }
    QString currentEmotion() const { return m_currentEmotion; }
    QVector<GirlfriendMessage> messages() const { return m_messages; }

    void addMessage(const QString &role, const QString &content, const QString &emotion = "default");
    void setCurrentEmotion(const QString &emotion);
    void clearMessages();

    // 持久化
    void saveToFile();
    void loadFromFile();
    static QString storagePath();

private:
    QString m_id;
    QString m_currentEmotion;
    QVector<GirlfriendMessage> m_messages;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
};

#endif // GIRLFRIENDSESSION_H