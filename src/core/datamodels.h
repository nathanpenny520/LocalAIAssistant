#ifndef DATAMODELS_H
#define DATAMODELS_H

#include <QString>
#include <QVector>
#include <QUuid>

struct FileAttachment
{
    QString path;           // 文件路径
    QString type;           // 类型标识: "text", "image", "binary"
    QString mimeType;       // MIME 类型: "text/plain", "image/png" 等
    QString content;        // 文本内容或 base64 编码数据
    qint64 size;            // 文件大小（字节）

    FileAttachment() : size(0) {}
    explicit FileAttachment(const QString &p) : path(p), size(0) {}
};

struct ChatMessage
{
    QString role;
    QString content;
    QVector<FileAttachment> attachments;

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