#include "networkmanager.h"
#include <QDebug>
#include <QBuffer>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_apiBaseUrl()
    , m_apiKey()
    , m_modelName()
    , m_isLocalMode(true)
    , m_systemPrompt()
    , m_temperature(0.4)
    , m_topP(1.0)
    , m_maxContext(10)
    , m_maxTokens(8192)
    , m_presencePenalty(0.2)
    , m_frequencyPenalty(0.0)
    , m_seed(std::nullopt)
    , m_streamingEnabled(true)
    , m_streamBuffer()
{
    m_apiBaseUrl = "http://127.0.0.1:8080";
    m_apiKey = "";
    m_modelName = "local-model";

    m_systemPrompt = "你是一个集成在电脑桌面端的专业 AI 助手。"
                     "1. 遇到技术问题，请先进行逻辑拆解（Thought），再给出结论。"
                     "2. 如果是代码建议，请确保符合现代代码规范。"
                     "3. 保持回答简洁有力，避免过度废话。";

    loadSettings();
}

bool NetworkManager::isStreamingEnabled() const
{
    return m_streamingEnabled;
}

void NetworkManager::setStreamingEnabled(bool enabled)
{
    m_streamingEnabled = enabled;
    QSettings settings("LocalAIAssistant", "Settings");
    settings.setValue("streamingEnabled", m_streamingEnabled);
}

void NetworkManager::loadSettings()
{
    QSettings settings("LocalAIAssistant", "Settings");

    m_isLocalMode = settings.value("localMode", true).toBool();
    m_apiBaseUrl = settings.value("apiBaseUrl", "http://127.0.0.1:8080").toString();
    m_apiKey = settings.value("apiKey", "").toString();
    m_modelName = settings.value("modelName", "local-model").toString();

    m_temperature = settings.value("temperature", 0.4).toDouble();
    m_maxContext = settings.value("maxContext", 10).toInt();
    m_maxTokens = settings.value("maxTokens", 2048).toInt();
    m_presencePenalty = settings.value("presencePenalty", 0.2).toDouble();
    m_topP = settings.value("topP", 1.0).toDouble();
    m_frequencyPenalty = settings.value("frequencyPenalty", 0.0).toDouble();

    // seed 处理：-1 表示未设置（null）
    int seedValue = settings.value("seed", -1).toInt();
    if (seedValue >= 0) {
        m_seed = seedValue;
    } else {
        m_seed = std::nullopt;
    }

    m_streamingEnabled = settings.value("streamingEnabled", true).toBool();
}

void NetworkManager::saveSettings()
{
    QSettings settings("LocalAIAssistant", "Settings");

    settings.setValue("localMode", m_isLocalMode);
    settings.setValue("apiBaseUrl", m_apiBaseUrl);
    settings.setValue("apiKey", m_apiKey);
    settings.setValue("modelName", m_modelName);
    settings.setValue("temperature", m_temperature);
    settings.setValue("maxContext", m_maxContext);
    settings.setValue("maxTokens", m_maxTokens);
    settings.setValue("presencePenalty", m_presencePenalty);
    settings.setValue("topP", m_topP);
    settings.setValue("frequencyPenalty", m_frequencyPenalty);

    // seed 保存：-1 表示 null
    settings.setValue("seed", m_seed.has_value() ? m_seed.value() : -1);
    settings.setValue("streamingEnabled", m_streamingEnabled);
}

void NetworkManager::updateSettings(const QString &apiBaseUrl, const QString &apiKey,
                                    const QString &modelName, bool isLocalMode)
{
    m_apiBaseUrl = apiBaseUrl;
    m_apiKey = apiKey;
    m_modelName = modelName;
    m_isLocalMode = isLocalMode;

    saveSettings();
}

void NetworkManager::sendChatRequest(const QString &userMessage)
{
    QVector<ChatMessage> singleMessage;
    singleMessage.append(ChatMessage("user", userMessage));
    sendChatRequestWithContext(singleMessage);
}

void NetworkManager::sendChatRequestWithContext(const QVector<ChatMessage> &messages)
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    QString fullUrl = m_apiBaseUrl;
    
    if (fullUrl.isEmpty()) {
        emit errorOccurred("API URL is empty");
        return;
    }
    
    if (!fullUrl.startsWith("http://") && !fullUrl.startsWith("https://")) {
        fullUrl = "https://" + fullUrl;
    }
    
    if (!fullUrl.endsWith("/")) {
        fullUrl += "/";
    }
    
    // 检查是否是 Ollama
    bool isOllama = fullUrl.contains("11434");

    // Ollama 使用 /api/chat 接口（支持 messages 格式和参数）
    // 其他使用 OpenAI 格式 /v1/chat/completions
    if (isOllama) {
        fullUrl += "api/chat";
    } else {
        fullUrl += "v1/chat/completions";
    }

    QUrl url(fullUrl);
    
    if (!url.isValid()) {
        emit errorOccurred(QString("Invalid API URL: %1").arg(fullUrl));
        return;
    }

    QJsonObject jsonPayload;
    jsonPayload["model"] = m_modelName;
    jsonPayload["temperature"] = m_temperature;                  // 使用成员变量而非硬编码
    jsonPayload["top_p"] = m_topP;                               // 新增
    jsonPayload["max_tokens"] = m_maxTokens;
    jsonPayload["presence_penalty"] = m_presencePenalty;
    jsonPayload["frequency_penalty"] = m_frequencyPenalty;       // 新增
    jsonPayload["stream"] = m_streamingEnabled;

    // seed 仅在有值时传递
    if (m_seed.has_value()) {
        jsonPayload["seed"] = m_seed.value();
    }

    // 统一使用 messages 格式（Ollama /api/chat 也支持）
    jsonPayload["messages"] = buildMessagesArray(messages, m_maxContext);

    QJsonDocument doc(jsonPayload);
    QByteArray postData = doc.toJson();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (!m_isLocalMode && !m_apiKey.isEmpty()) {
        QString authHeader = QString("Bearer %1").arg(m_apiKey);
        request.setRawHeader("Authorization", authHeader.toUtf8());
    }

    m_streamBuffer.clear();
    m_currentReply = m_networkManager->post(request, postData);

    if (m_streamingEnabled) {
        connect(m_currentReply, &QNetworkReply::readyRead, this, &NetworkManager::onStreamReadyRead);
        connect(m_currentReply, &QNetworkReply::finished, this, &NetworkManager::onStreamFinished);
    } else {
        connect(m_currentReply, &QNetworkReply::finished, this, &NetworkManager::onReplyFinished);
    }
}

QJsonArray NetworkManager::buildMessagesArray(const QVector<ChatMessage> &messages, int maxMessages)
{
    QJsonArray jsonMessages;

    QJsonObject systemObj;
    systemObj["role"] = "system";
    systemObj["content"] = m_systemPrompt;
    jsonMessages.append(systemObj);

    int totalCount = messages.size();
    int startIndex = qMax(0, totalCount - maxMessages);

    for (int i = startIndex; i < totalCount; ++i) {
        QJsonObject msgObj;
        msgObj["role"] = messages[i].role;
        msgObj["content"] = messages[i].content;
        jsonMessages.append(msgObj);
    }

    return jsonMessages;
}

void NetworkManager::onReplyFinished()
{
    if (!m_currentReply) {
        return;
    }

    if (m_currentReply->error() != QNetworkReply::NoError) {
        QString errorMsg = m_currentReply->errorString();
        emit errorOccurred(errorMsg);
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

void NetworkManager::onStreamReadyRead()
{
    if (!m_currentReply) {
        return;
    }

    // 检查是否是 Ollama
    bool isOllama = m_apiBaseUrl.contains("11434");

    if (isOllama) {
        // 对于 Ollama，按行读取数据
        while (m_currentReply->canReadLine()) {
            QByteArray line = m_currentReply->readLine();
            QString chunk = extractDeltaFromSSE(line, true);

            if (!chunk.isEmpty()) {
                m_streamBuffer += chunk;
                emit streamChunkReceived(chunk);
            }
        }
    } else {
        // 对于其他服务器，保持原来的处理方式
        QByteArray newData = m_currentReply->readAll();
        QString chunk = extractDeltaFromSSE(newData, false);

        if (!chunk.isEmpty()) {
            m_streamBuffer += chunk;
            emit streamChunkReceived(chunk);
        }
    }
}

void NetworkManager::onStreamFinished()
{
    if (!m_currentReply) {
        return;
    }

    if (m_currentReply->error() != QNetworkReply::NoError
        && m_currentReply->error() != QNetworkReply::OperationCanceledError) {
        QString errorMsg = m_currentReply->errorString();
        emit errorOccurred(errorMsg);
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        m_streamBuffer.clear();
        return;
    }

    QByteArray remainingData = m_currentReply->readAll();
    if (!remainingData.isEmpty()) {
        QString chunk = extractDeltaFromSSE(remainingData);
        if (!chunk.isEmpty()) {
            m_streamBuffer += chunk;
        }
    }

    if (m_streamBuffer.isEmpty()) {
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }

    emit streamFinished(m_streamBuffer);

    m_streamBuffer.clear();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

QString NetworkManager::extractDeltaFromSSE(const QByteArray &data, bool isOllama)
{
    QString result;
    QString text = QString::fromUtf8(data).trimmed();

    if (text.isEmpty()) {
        return result;
    }

    // 检查是否是 [DONE] 消息
    if (text == "[DONE]") {
        return result;
    }

    if (isOllama) {
        // 对于 Ollama，直接解析 JSON
        QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
        if (doc.isNull() || !doc.isObject()) {
            return result;
        }

        QJsonObject root = doc.object();
        if (root.contains("error")) {
            QString errorMessage = root["error"].toString();
            if (errorMessage.isEmpty()) {
                QJsonObject errorObj = root["error"].toObject();
                errorMessage = errorObj["message"].toString("Unknown error");
            }
            emit errorOccurred(errorMessage);
            return result;
        }

        // 处理 Ollama /api/generate 的流式响应格式
        if (root.contains("response")) {
            result += root["response"].toString();
        }
        // 处理 Ollama /api/chat 的响应格式
        else if (root.contains("message")) {
            QJsonObject message = root["message"].toObject();
            if (message.contains("content")) {
                result += message["content"].toString();
            }
        }
    } else {
        // 对于其他服务器，保持原来的处理方式
        QStringList lines = text.split('\n');
        for (const QString &line : lines) {
            if (!line.startsWith("data: ")) {
                continue;
            }

            QString jsonStr = line.mid(6).trimmed();
            if (jsonStr == "[DONE]") {
                break;
            }

            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
            if (doc.isNull() || !doc.isObject()) {
                continue;
            }

            QJsonObject root = doc.object();
            if (root.contains("error")) {
                QJsonObject errorObj = root["error"].toObject();
                emit errorOccurred(errorObj["message"].toString("Unknown error"));
                break;
            }

            // 处理 OpenAI 的响应格式
            if (root.contains("choices") && root["choices"].isArray()) {
                QJsonArray choices = root["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject firstChoice = choices[0].toObject();
                    QJsonObject delta = firstChoice["delta"].toObject();
                    if (delta.contains("content")) {
                        result += delta["content"].toString();
                    }
                }
            }
        }
    }

    return result;
}

QString NetworkManager::extractContentFromResponse(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return "响应格式错误";
    }

    QJsonObject root = doc.object();

    if (root.contains("error")) {
        QJsonObject errorObj = root["error"].toObject();
        QString errorMessage = errorObj["message"].toString("Unknown error");
        return "API 错误: " + errorMessage;
    }

    // 处理 Ollama /api/generate 的响应格式
    if (root.contains("response")) {
        QString content = root["response"].toString();
        content = content.trimmed();
        return content;
    }
    // 处理 Ollama /api/chat 的响应格式
    else if (root.contains("message")) {
        QJsonObject message = root["message"].toObject();
        if (message.contains("content")) {
            QString content = message["content"].toString();
            content = content.trimmed();
            return content;
        }
    }
    // 处理 OpenAI 的响应格式
    else if (root.contains("choices") && root["choices"].isArray()) {
        QJsonArray choices = root["choices"].toArray();
        if (!choices.isEmpty()) {
            QJsonObject firstChoice = choices[0].toObject();

            if (firstChoice.contains("message")) {
                QJsonObject message = firstChoice["message"].toObject();
                if (message.contains("content")) {
                    QString content = message["content"].toString();
                    content = content.trimmed();
                    return content;
                }
            }

            if (firstChoice.contains("finish_reason")) {
                QString finishReason = firstChoice["finish_reason"].toString();
                if (finishReason == "length") {
                    return "警告: 回复因长度限制被截断";
                }
            }
        }
    }

    return "未能解析到有效内容";
}
