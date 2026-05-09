/**
 * @file datamodels.h
 * @brief Shared data structures for chat messages, sessions, file attachments, and shell operations.
 */
#pragma once

#ifndef DATAMODELS_H
#define DATAMODELS_H

#include <QString>
#include <QUuid>
#include <QVector>

struct FileAttachment {
    QString path;      // file path
    QString type;      // type identifier: "text", "image", "binary"
    QString mimeType;  // MIME type, e.g. "text/plain", "image/png"
    QString content;   // text content or base64-encoded data
    qint64 size;       // file size in bytes

    FileAttachment() : size(0) {
    }
    explicit FileAttachment(const QString& p) : path(p), size(0) {
    }
};

struct ChatMessage {
    enum MessageType { Normal, SystemNotification };
    QString role;
    QString content;
    QVector<FileAttachment> attachments;
    bool isAgentLoopInjected = false;
    MessageType messageType = Normal;

    ChatMessage() = default;
    ChatMessage(const QString& r, const QString& c) : role(r), content(c) {
    }
};

struct ChatSession {
    QString id;
    QString title;
    QVector<ChatMessage> messages;
    bool pinned = false;
    bool autoNamed = false;

    ChatSession() : id(QUuid::createUuid().toString(QUuid::WithoutBraces)) {
    }
    explicit ChatSession(const QString& sessionTitle)
            : id(QUuid::createUuid().toString(QUuid::WithoutBraces)), title(sessionTitle) {
    }
};

#endif