#pragma once

#ifndef GIRLFRIENDSESSION_H
#define GIRLFRIENDSESSION_H

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QString>
#include <QUuid>
#include <QVector>

struct GirlfriendMessage {
    QString role;     // "user" or "girlfriend"
    QString content;
    QString emotion;  // current emotion: happy, shy, love, hate, sad, angry, afraid, awaiting,
                      // studying, default
    bool isSystemNotification = false;

    GirlfriendMessage() : emotion("default") {
    }
    GirlfriendMessage(const QString& r, const QString& c, const QString& e = "default")
            : role(r), content(c), emotion(e) {
    }
};

class GirlfriendSession {
public:
    GirlfriendSession();

    QString id() const {
        return m_id;
    }
    QString currentEmotion() const {
        return m_currentEmotion;
    }
    double mood() const {
        return m_mood;
    }
    QVector<GirlfriendMessage> messages() const {
        return m_messages;
    }

    void addMessage(const QString& role, const QString& content,
                    const QString& emotion = "default");
    void setCurrentEmotion(const QString& emotion);
    void setMood(double mood);
    void clearMessages();

    int maxMessages() const { return m_maxMessages; }
    void setMaxMessages(int limit);

    // 持久化
    void saveToFile();
    void loadFromFile();
    void saveToFile(const QString& path);
    void loadFromFile(const QString& path);
    static QString storagePath();

private:
    QString m_id;
    QString m_currentEmotion;
    double m_mood = 0.6;  // mood value (0.0-1.0), default 0.6
    int m_maxMessages = 500;
    QVector<GirlfriendMessage> m_messages;

    void truncateMessages();

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
};

#endif  // GIRLFRIENDSESSION_H