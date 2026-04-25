#ifndef DATAMODELS_H
#define DATAMODELS_H

#include <QString>
#include <QVector>
#include <QUuid>

struct ChatMessage
{
    QString role;
    QString content;

    ChatMessage() = default;
    ChatMessage(const QString &r, const QString &c) : role(r), content(c) {}
};

struct ChatSession
{
    QString id;
    QString title;
    QVector<ChatMessage> messages;

    ChatSession() : id(QUuid::createUuid().toString(QUuid::WithoutBraces)) {}
    explicit ChatSession(const QString &sessionTitle) : id(QUuid::createUuid().toString(QUuid::WithoutBraces)), title(sessionTitle) {}
};

#endif