#pragma once

#ifndef APIPROVIDER_H
#define APIPROVIDER_H

#include <optional>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QSettings>
#include <QStandardPaths>

#include "datamodels.h"

/// Abstract base for LLM API providers (OpenAI, Ollama, LlamaCpp).
/// Subclasses override endpoint, message format, SSE parsing, and response parsing.
class ApiProvider : public QObject {
    Q_OBJECT

public:
    explicit ApiProvider(QObject* parent = nullptr);
    ~ApiProvider() override;

    // Configuration
    void setBaseUrl(const QString& url);
    void setApiKey(const QString& key);
    void setModelName(const QString& name);
    void setSystemPrompt(const QString& prompt);
    void setKnowledgeContext(const QString& context);
    void setIsLocalMode(bool local);
    void setStreamingEnabled(bool enabled);
    bool isStreamingEnabled() const;

    // Model parameters
    void setTemperature(double temp);
    void setTopP(double topP);
    void setMaxTokens(int tokens);
    void setMaxContext(int context);
    void setPresencePenalty(double penalty);
    void setFrequencyPenalty(double penalty);
    void setSeed(std::optional<int> seed);

    // Operations
    void sendChatRequest(const QVector<ChatMessage>& messages);
    void abortCurrentRequest();

signals:
    void responseReceived(const QString& content);
    void streamChunkReceived(const QString& chunk);
    void streamFinished(const QString& fullContent);
    void errorOccurred(const QString& error);

protected:
    // Provider-specific overrides
    virtual QString endpointPath() const = 0;
    virtual QJsonArray buildMessagesArray(const QVector<ChatMessage>& messages) const = 0;
    virtual QString extractDeltaFromSSE(const QByteArray& data) = 0;
    virtual QString extractContentFromResponse(const QByteArray& data) = 0;
    virtual void configureRequest(QNetworkRequest& request) const;
    virtual QJsonObject buildBasePayload() const;

    // Shared helpers
    QString resolveFullUrl() const;
    QJsonObject buildTextContentBlock(const QString& text) const;
    QJsonObject buildImageContentBlock(const QString& base64Data, const QString& mime) const;
    QJsonObject buildFileContentBlock(const FileAttachment& file) const;
    QString loadSystemPrompt() const;

    // Shared state
    QNetworkAccessManager* m_network;
    QNetworkReply* m_currentReply;
    QString m_streamBuffer;

    QString m_baseUrl;
    QString m_apiKey;
    QString m_modelName;
    QString m_systemPrompt;
    QString m_knowledgeContext;
    bool m_isLocalMode;
    bool m_streamingEnabled;
    double m_temperature;
    double m_topP;
    int m_maxTokens;
    int m_maxContext;
    double m_presencePenalty;
    double m_frequencyPenalty;
    std::optional<int> m_seed;

private slots:
    void onReplyFinished();
    void onStreamReadyRead();
    void onStreamFinished();
};

#endif
