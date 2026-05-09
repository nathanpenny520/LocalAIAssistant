/**
 * @file networkmanager.h
 * @brief Thin facade over ApiProvider subclasses (OpenAI, Ollama, LlamaCpp, Anthropic) for LLM API
 * calls.
 */
#pragma once

#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <optional>

#include <QObject>

#include "datamodels.h"

enum class ApiType { OpenAI, Ollama, LlamaCpp, Anthropic };

class ApiProvider;

class NetworkManager : public QObject {
    Q_OBJECT

public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager() override;

    bool isStreamingEnabled() const;
    void setStreamingEnabled(bool enabled);
    void setSystemPrompt(const QString& prompt);
    void setKnowledgeContext(const QString& context);
    void abortCurrentRequest();

    ApiType apiType() const {
        return m_apiType;
    }
    void setApiType(ApiType type);

signals:
    void responseReceived(const QString& content);
    void streamChunkReceived(const QString& chunk);
    void streamFinished(const QString& fullContent);
    void errorOccurred(const QString& error);

public slots:
    void sendChatRequest(const QString& userMessage);
    void sendChatRequestWithContext(const QVector<ChatMessage>& messages);
    void updateSettings(const QString& apiBaseUrl, const QString& apiKey, const QString& modelName,
                        ApiType apiType = ApiType::OpenAI);

private:
    void loadSettings();
    void saveSettings();
    void applySettingsToProvider();
    void ensureProvider(ApiType type);
    void connectProviderSignals();

    ApiProvider* m_provider;
    ApiType m_apiType;

    // Cached settings (loaded/saved via QSettings)
    QString m_apiBaseUrl;
    QString m_apiKey;
    QString m_modelName;
    double m_temperature;
    double m_topP;
    int m_maxContext;
    int m_maxTokens;
    double m_presencePenalty;
    double m_frequencyPenalty;
    std::optional<int> m_seed;
    bool m_streamingEnabled;
};

#endif
