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

    void setUserNickname(const QString &nickname);
    QString userNickname() const { return m_userNickname; }

signals:
    void emotionDetected(const QString &emotion);

private:
    QString m_personalityPrompt;
    QString m_userNickname;

    void loadFromFile();
    QStringList findPossiblePaths() const;
};

#endif // PERSONALITYENGINE_H