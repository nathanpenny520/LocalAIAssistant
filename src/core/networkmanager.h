#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QCoreApplication>
#include <QFile>
#include <optional>
#include "datamodels.h"

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);

    bool isStreamingEnabled() const;
    void setStreamingEnabled(bool enabled);

signals:
    void responseReceived(const QString &content);
    void streamChunkReceived(const QString &chunk);
    void streamFinished(const QString &fullContent);
    void errorOccurred(const QString &error);

public slots:
    void sendChatRequest(const QString &userMessage);
    void sendChatRequestWithContext(const QVector<ChatMessage> &messages);
    void updateSettings(const QString &apiBaseUrl, const QString &apiKey,
                        const QString &modelName, bool isLocalMode);

private slots:
    void onReplyFinished();
    void onStreamReadyRead();
    void onStreamFinished();

private:
    void loadSettings();
    void saveSettings();
    QString loadSystemPrompt();
    QString extractContentFromResponse(const QByteArray &data);
    QString extractDeltaFromSSE(const QByteArray &data, bool isOllama = false);
    QJsonArray buildMessagesArray(const QVector<ChatMessage> &messages, int maxMessages);
    QJsonObject buildTextContentBlock(const QString &text);
    QJsonObject buildImageContentBlock(const QString &base64Data, const QString &mime);
    QJsonObject buildFileContentBlock(const FileAttachment &file);

    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply;

    QString m_apiBaseUrl;
    QString m_apiKey;
    QString m_modelName;
    bool m_isLocalMode;

    QString m_systemPrompt;
    double m_temperature;
    double m_topP;
    int m_maxContext;
    int m_maxTokens;
    double m_presencePenalty;
    double m_frequencyPenalty;
    std::optional<int> m_seed;

    bool m_streamingEnabled;
    QString m_streamBuffer;
};

#endif
