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

    // Parses emotion tag from the response, returns (emotion, cleaned text)
    struct EmotionResult {
        QString emotion;
        QString cleanText;
    };
    EmotionResult parseEmotionFromResponse(const QString& text) const;

    void setUserNickname(const QString& nickname);
    QString userNickname() const {
        return m_userNickname;
    }

    // Mood value (0.0 = worst, 1.0 = best)
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
    }

signals:
    void emotionDetected(const QString& emotion);
    void moodChanged(double newMood);

private:
    QString m_personalityPrompt;
    QString m_userNickname;
    double m_mood = 0.6;        // current mood (0-1)
    double m_moodDecay = 0.05;  // decay per message

    QString getMoodHint() const;
    void parseTemplateConfig();  // extract prompt config QMap from personality.md
    QString templateValue(const QString& key, const QString& fallback) const;

    void loadFromFile();
    QMap<QString, QString> m_templateConfig;  // key=value from <!-- CONFIG_START --> block
};

#endif  // PERSONALITYENGINE_H