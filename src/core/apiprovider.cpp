#include "apiprovider.h"
#include "../prompts/promptmanager.h"
#include <QDebug>
#include <QUrl>

ApiProvider::ApiProvider(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_baseUrl("http://127.0.0.1:8080")
    , m_apiKey()
    , m_modelName("local-model")
    , m_systemPrompt()
    , m_isLocalMode(true)
    , m_streamingEnabled(true)
    , m_temperature(0.4)
    , m_topP(1.0)
    , m_maxTokens(8192)
    , m_maxContext(20)
    , m_presencePenalty(0.2)
    , m_frequencyPenalty(0.0)
    , m_seed(std::nullopt)
{
    m_systemPrompt = loadSystemPrompt();
}

ApiProvider::~ApiProvider()
{
    abortCurrentRequest();
}

// --- Configuration setters ---

void ApiProvider::setBaseUrl(const QString &url) { m_baseUrl = url.trimmed(); }
void ApiProvider::setApiKey(const QString &key) { m_apiKey = key.trimmed(); }
void ApiProvider::setModelName(const QString &name) { m_modelName = name.trimmed(); }
void ApiProvider::setSystemPrompt(const QString &prompt) { m_systemPrompt = prompt; }
void ApiProvider::setIsLocalMode(bool local) { m_isLocalMode = local; }

void ApiProvider::setStreamingEnabled(bool enabled)
{
    m_streamingEnabled = enabled;
}

bool ApiProvider::isStreamingEnabled() const
{
    return m_streamingEnabled;
}

void ApiProvider::setTemperature(double temp) { m_temperature = temp; }
void ApiProvider::setTopP(double topP) { m_topP = topP; }
void ApiProvider::setMaxTokens(int tokens) { m_maxTokens = tokens; }
void ApiProvider::setMaxContext(int context) { m_maxContext = context; }
void ApiProvider::setPresencePenalty(double penalty) { m_presencePenalty = penalty; }
void ApiProvider::setFrequencyPenalty(double penalty) { m_frequencyPenalty = penalty; }
void ApiProvider::setSeed(std::optional<int> seed) { m_seed = seed; }

// --- Operations ---

void ApiProvider::abortCurrentRequest()
{
    if (m_currentReply) {
        m_currentReply->disconnect(this);
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    m_streamBuffer.clear();
}

void ApiProvider::sendChatRequest(const QVector<ChatMessage> &messages)
{
    abortCurrentRequest();

    QString fullUrl = resolveFullUrl();
    if (fullUrl.isEmpty()) {
        emit errorOccurred("API URL is empty");
        return;
    }

    QUrl url(fullUrl);
    if (!url.isValid()) {
        emit errorOccurred(QString("Invalid API URL: %1").arg(fullUrl));
        return;
    }

    QJsonObject jsonPayload = buildBasePayload();

    // Inject user memory into system prompt if available
    {
        QString memoryPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                             + QStringLiteral("/girlfriend/memory.md");
        QFile memoryFile(memoryPath);
        QString memoryContent;
        if (memoryFile.exists() && memoryFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            memoryContent = QString::fromUtf8(memoryFile.readAll());
            memoryFile.close();
        }
        if (!memoryContent.isEmpty() && !memoryContent.contains(QStringLiteral("待记录"))) {
            m_systemPrompt = loadSystemPrompt()
                + QStringLiteral("\n\n## 用户记忆档案\n\n")
                + memoryContent;
        } else {
            m_systemPrompt = loadSystemPrompt();
        }
    }

    jsonPayload["messages"] = buildMessagesArray(messages);

    QJsonDocument doc(jsonPayload);
    QByteArray postData = doc.toJson();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    configureRequest(request);

    m_streamBuffer.clear();
    m_currentReply = m_network->post(request, postData);

    if (m_streamingEnabled) {
        connect(m_currentReply, &QNetworkReply::readyRead, this, &ApiProvider::onStreamReadyRead);
        connect(m_currentReply, &QNetworkReply::finished, this, &ApiProvider::onStreamFinished);
    } else {
        connect(m_currentReply, &QNetworkReply::finished, this, &ApiProvider::onReplyFinished);
    }
}

// --- Private slots ---

void ApiProvider::onReplyFinished()
{
    if (!m_currentReply)
        return;

    if (m_currentReply->error() != QNetworkReply::NoError) {
        emit errorOccurred(m_currentReply->errorString());
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }

    QByteArray responseData = m_currentReply->readAll();
    QString content = extractContentFromResponse(responseData);
    emit responseReceived(content);

    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

void ApiProvider::onStreamReadyRead()
{
    if (!m_currentReply)
        return;

    QByteArray newData = m_currentReply->readAll();
    QString chunk = extractDeltaFromSSE(newData);
    if (!chunk.isEmpty()) {
        m_streamBuffer += chunk;
        emit streamChunkReceived(chunk);
    }
}

void ApiProvider::onStreamFinished()
{
    if (!m_currentReply)
        return;

    if (m_currentReply->error() != QNetworkReply::NoError
        && m_currentReply->error() != QNetworkReply::OperationCanceledError) {
        emit errorOccurred(m_currentReply->errorString());
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        m_streamBuffer.clear();
        return;
    }

    QByteArray remainingData = m_currentReply->readAll();
    if (!remainingData.isEmpty()) {
        QString chunk = extractDeltaFromSSE(remainingData);
        if (!chunk.isEmpty())
            m_streamBuffer += chunk;
    }

    if (!m_streamBuffer.isEmpty()) {
        emit streamFinished(m_streamBuffer);
        m_streamBuffer.clear();
    }

    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

// --- Default configureRequest (no auth) ---

void ApiProvider::configureRequest(QNetworkRequest &request) const
{
    Q_UNUSED(request);
}

// --- Shared helpers ---

QString ApiProvider::resolveFullUrl() const
{
    QString fullUrl = m_baseUrl;

    if (fullUrl.isEmpty())
        return {};

    if (!fullUrl.startsWith("http://", Qt::CaseInsensitive)
        && !fullUrl.startsWith("https://", Qt::CaseInsensitive)) {
        fullUrl = (m_isLocalMode ? QStringLiteral("http://") : QStringLiteral("https://")) + fullUrl;
    }

    if (!fullUrl.endsWith("/"))
        fullUrl += "/";

    fullUrl += endpointPath();
    return fullUrl;
}

QJsonObject ApiProvider::buildBasePayload() const
{
    QJsonObject payload;
    payload["model"] = m_modelName;
    payload["temperature"] = m_temperature;
    payload["top_p"] = m_topP;
    payload["max_tokens"] = m_maxTokens;
    payload["presence_penalty"] = m_presencePenalty;
    payload["frequency_penalty"] = m_frequencyPenalty;
    payload["stream"] = m_streamingEnabled;

    if (m_seed.has_value())
        payload["seed"] = m_seed.value();

    return payload;
}

QString ApiProvider::loadSystemPrompt() const
{
    QString prompt = PromptManager::instance()->systemPrompt();
    QString task = PromptManager::instance()->taskPrompt();
    if (!task.isEmpty())
        prompt += QStringLiteral("\n\n") + task;
    return prompt;
}

QJsonObject ApiProvider::buildTextContentBlock(const QString &text) const
{
    QJsonObject block;
    block["type"] = "text";
    block["text"] = text;
    return block;
}

QJsonObject ApiProvider::buildImageContentBlock(const QString &base64Data, const QString &mime) const
{
    Q_UNUSED(mime);
    QJsonObject block;
    block["type"] = "image_url";

    QJsonObject imageUrl;
    imageUrl["url"] = base64Data;
    block["image_url"] = imageUrl;

    return block;
}

QJsonObject ApiProvider::buildFileContentBlock(const FileAttachment &file) const
{
    QJsonObject block;
    block["type"] = "text";
    block["text"] = file.content;
    return block;
}
