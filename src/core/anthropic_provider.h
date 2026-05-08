#pragma once

#ifndef ANTHROPICPROVIDER_H
#define ANTHROPICPROVIDER_H

#include "apiprovider.h"

/// Provider for Anthropic Messages API (/v1/messages).
/// Handles SSE streaming (event:/data: pairs), anthropic-version header,
/// system prompt at top level, and Anthropic-specific image format.
class AnthropicProvider : public ApiProvider
{
    Q_OBJECT

public:
    explicit AnthropicProvider(QObject *parent = nullptr);

protected:
    QString endpointPath() const override;
    QJsonObject buildBasePayload() const override;
    QJsonArray buildMessagesArray(const QVector<ChatMessage> &messages) const override;
    QString extractDeltaFromSSE(const QByteArray &data) override;
    QString extractContentFromResponse(const QByteArray &data) override;
    void configureRequest(QNetworkRequest &request) const override;

private:
    QJsonObject buildAnthropicImageBlock(const QString &base64Data, const QString &mime) const;
};

#endif
