#pragma once

#ifndef OPENAIPROVIDER_H
#define OPENAIPROVIDER_H

#include "apiprovider.h"

/// Provider for OpenAI-compatible APIs (/v1/chat/completions).
/// Handles SSE streaming (data: prefix) and content-array message format.
class OpenAIProvider : public ApiProvider
{
    Q_OBJECT

public:
    explicit OpenAIProvider(QObject *parent = nullptr);

protected:
    QString endpointPath() const override;
    QJsonArray buildMessagesArray(const QVector<ChatMessage> &messages) const override;
    QString extractDeltaFromSSE(const QByteArray &data) override;
    QString extractContentFromResponse(const QByteArray &data) override;
    void configureRequest(QNetworkRequest &request) const override;
};

#endif
