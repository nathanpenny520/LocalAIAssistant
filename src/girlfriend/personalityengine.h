#pragma once

#ifndef PERSONALITYENGINE_H
#define PERSONALITYENGINE_H

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>

#include "girlfriendsettings.h"

class PersonalityEngine : public QObject {
    Q_OBJECT

public:
    explicit PersonalityEngine(QObject* parent = nullptr);

    QString loadPersonalityPrompt();
    QString buildSystemPrompt(const QString& memoryContent = {});

    QString detectEmotion(const QString& text, double mood = 0.6) const;
    QString emotionToDisplayName(const QString& emotion) const;

    // 解析回复中的情绪标记，返回 (情绪, 清理后的文本)
    struct EmotionResult {
        QString emotion;
        QString cleanText;
    };
    EmotionResult parseEmotionFromResponse(const QString& text) const;

    void setUserNickname(const QString& nickname);
    QString userNickname() const {
        return m_userNickname;
    }

    // 情绪值系统 (0.0 = 很差, 1.0 = 很好)
    double mood() const {
        return m_mood;
    }
    void setMood(double mood) {
        m_mood = qBound(0.0, mood, 1.0);
        emit moodChanged(m_mood);
    }
    void updateMood(const QString& userInput);
    void resetMood() {
        m_mood = 0.6;
        emit moodChanged(m_mood);
    }  // 触发信号更新UI

signals:
    void emotionDetected(const QString& emotion);
    void moodChanged(double newMood);

private:
    QString m_personalityPrompt;
    QString m_userNickname;
    double m_mood = 0.6;        // 当前心情值 (0-1)
    double m_moodDecay = 0.05;  // 每次对话衰减

    QString getMoodHint() const;
    void parseTemplateConfig();  // 从 personality.md 提取提示词配置 QMap
    QString templateValue(const QString& key, const QString& fallback) const;

    void loadFromFile();
    QMap<QString, QString> m_templateConfig;  // key=value from <!-- CONFIG_START --> block
};

#endif  // PERSONALITYENGINE_H