#ifndef PERSONALITYENGINE_H
#define PERSONALITYENGINE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

class PersonalityEngine : public QObject
{
    Q_OBJECT

public:
    explicit PersonalityEngine(QObject *parent = nullptr);

    QString loadPersonalityPrompt();
    QString buildSystemPrompt();

    QString detectEmotion(const QString &text) const;
    QString emotionToDisplayName(const QString &emotion) const;

    // 解析回复中的情绪标记，返回 (情绪, 清理后的文本)
    struct EmotionResult {
        QString emotion;
        QString cleanText;
    };
    EmotionResult parseEmotionFromResponse(const QString &text) const;

    void setUserNickname(const QString &nickname);
    QString userNickname() const { return m_userNickname; }

    // 情绪值系统 (0.0 = 很差, 1.0 = 很好)
    double mood() const { return m_mood; }
    void updateMood(const QString &userInput);
    void resetMood() { m_mood = 0.6; }

signals:
    void emotionDetected(const QString &emotion);
    void moodChanged(double newMood);

private:
    QString m_personalityPrompt;
    QString m_userNickname;
    double m_mood = 0.6;           // 当前心情值 (0-1)
    double m_moodDecay = 0.05;     // 每次对话衰减

    QString getMoodHint() const;   // 根据心情生成提示语

    void loadFromFile();
    QStringList findPossiblePaths() const;
};

#endif // PERSONALITYENGINE_H