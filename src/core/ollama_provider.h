#pragma once

#ifndef OLLAMAPROVIDER_H
#define OLLAMAPROVIDER_H

#include "apiprovider.h"

/// Provider for Ollama API (/api/chat).
/// Handles raw JSON streaming and images-array message format.
class OllamaProvider : public ApiProvider
{
    Q_OBJECT

public:
    explicit OllamaProvider(QObject *parent = nullptr);

protected:
    QString endpointPath() const override;
    QJsonArray buildMessagesArray(const QVector<ChatMessage> &messages) const override;
    QString extractDeltaFromSSE(const QByteArray &data) override;
    QString extractContentFromResponse(const QByteArray &data) override;
};

#endif
