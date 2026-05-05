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
    double mood() const { return m_mood; }  // 获取心情值
    QVector<GirlfriendMessage> messages() const { return m_messages; }

    void addMessage(const QString &role, const QString &content, const QString &emotion = "default");
    void setCurrentEmotion(const QString &emotion);
    void setMood(double mood);  // 设置心情值
    void clearMessages();

    // 持久化
    void saveToFile();
    void loadFromFile();
    void saveToFile(const QString &path);
    void loadFromFile(const QString &path);
    static QString storagePath();

private:
    QString m_id;
    QString m_currentEmotion;
    double m_mood = 0.6;  // 心情值 (0.0-1.0)，默认0.6
    QVector<GirlfriendMessage> m_messages;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
};

#endif // GIRLFRIENDSESSION_H