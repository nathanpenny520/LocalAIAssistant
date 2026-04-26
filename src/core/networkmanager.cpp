#include "networkmanager.h"
#include "datamodels.h"
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

    m_systemPrompt = loadSystemPrompt();

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
    m_apiBaseUrl = settings.value("apiBaseUrl", "http://127.0.0.1:8080").toString().trimmed();
    m_apiKey = settings.value("apiKey", "").toString().trimmed();
    m_modelName = settings.value("modelName", "local-model").toString().trimmed();

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

QString NetworkManager::loadSystemPrompt()
{
    // 尝试多个可能的路径
    QStringList possiblePaths;

    // 1. 应用程序目录下的 core/soul.md
    QString appDir = QCoreApplication::applicationDirPath();
    possiblePaths << appDir + "/core/soul.md";

    // 2. macOS app bundle 结构
    possiblePaths << appDir + "/../Resources/core/soul.md";

    // 3. 开发时相对路径
    possiblePaths << "sourcecode-ai-assistant/src/core/soul.md";
    possiblePaths << "src/core/soul.md";

    for (const QString &path : possiblePaths) {
        QFile file(path);
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromUtf8(file.readAll());
            file.close();
            return content.trimmed();
        }
    }

    // 降级：内置默认 prompt
    return "You are a helpful AI assistant integrated into a desktop application.\n\n"
           "## Language\n\n"
           "Adapt to user's language. Reply in the same language the user uses.\n\n"
           "## Guidelines\n\n"
           "1. For technical problems, break down the logic before giving conclusions.\n"
           "2. For code suggestions, follow modern coding standards.\n"
           "3. Keep answers concise and avoid unnecessary verbosity.";
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
    m_apiBaseUrl = apiBaseUrl.trimmed();
    m_apiKey = apiKey.trimmed();
    m_modelName = modelName.trimmed();
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

    // 去除 URL 两端的空白字符
    QString fullUrl = m_apiBaseUrl.trimmed();

    if (fullUrl.isEmpty()) {
        emit errorOccurred("API URL is empty");
        return;
    }

    // 检查是否已有协议前缀，如果没有则添加 https://
    if (!fullUrl.startsWith("http://", Qt::CaseInsensitive) &&
        !fullUrl.startsWith("https://", Qt::CaseInsensitive)) {
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

    // 统一使用 messages 格式，传递 isOllama 参数以支持不同格式
    jsonPayload["messages"] = buildMessagesArray(messages, m_maxContext, isOllama);

    QJsonDocument doc(jsonPayload);
    QByteArray postData = doc.toJson();

    // Debug: 打印发送的 JSON（对于图片消息，显示前 500 字符）
    bool hasAttachments = false;
    for (const ChatMessage &msg : messages) {
        if (!msg.attachments.isEmpty()) {
            hasAttachments = true;
            break;
        }
    }
    if (hasAttachments) {
        qDebug() << "=== Multimodal request (with attachments) ===";
        qDebug() << "JSON payload (first 1000 chars):" << QString(postData).left(1000);
        qDebug() << "JSON payload size:" << postData.size() << "bytes";
    }

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

QJsonArray NetworkManager::buildMessagesArray(const QVector<ChatMessage> &messages, int maxMessages, bool isOllama)
{
    QJsonArray jsonMessages;

    // System message
    QJsonObject systemObj;
    systemObj["role"] = "system";
    systemObj["content"] = m_systemPrompt;
    jsonMessages.append(systemObj);

    int totalCount = messages.size();
    int startIndex = qMax(0, totalCount - maxMessages);

    for (int i = startIndex; i < totalCount; ++i) {
        const ChatMessage &msg = messages[i];
        QJsonObject msgObj;
        msgObj["role"] = msg.role;

        // Check for attachments
        if (msg.attachments.isEmpty()) {
            // No attachments, use simple string format
            msgObj["content"] = msg.content;
        } else if (isOllama) {
            // Ollama format: content as string, images as separate array
            msgObj["content"] = msg.content;

            QJsonArray imagesArray;
            for (const FileAttachment &file : msg.attachments) {
                if (file.type == "image") {
                    // Ollama needs base64 without data URL prefix
                    QString base64Data = file.content;
                    if (base64Data.startsWith("data:")) {
                        // Remove "data:image/png;base64," prefix
                        int commaPos = base64Data.indexOf(',');
                        if (commaPos > 0) {
                            base64Data = base64Data.mid(commaPos + 1);
                        }
                    }
                    imagesArray.append(base64Data);
                } else {
                    // Non-image files: append as text content
                    QString currentContent = msgObj["content"].toString();
                    currentContent += "\n\n" + file.content;
                    msgObj["content"] = currentContent;
                }
            }
            if (!imagesArray.isEmpty()) {
                msgObj["images"] = imagesArray;
            }
        } else {
            // OpenAI format: content as array with text and image_url types
            QJsonArray contentArray;

            // Add user text first (if any)
            if (!msg.content.isEmpty()) {
                contentArray.append(buildTextContentBlock(msg.content));
            }

            // Add all attachments
            for (const FileAttachment &file : msg.attachments) {
                if (file.type == "image") {
                    contentArray.append(buildImageContentBlock(file.content, file.mimeType));
                } else {
                    contentArray.append(buildFileContentBlock(file));
                }
            }

            msgObj["content"] = contentArray;
        }

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

QJsonObject NetworkManager::buildTextContentBlock(const QString &text)
{
    QJsonObject block;
    block["type"] = "text";
    block["text"] = text;
    return block;
}

QJsonObject NetworkManager::buildImageContentBlock(const QString &base64Data, const QString &mime)
{
    QJsonObject block;
    block["type"] = "image_url";

    QJsonObject imageUrl;
    imageUrl["url"] = base64Data;  // Already in data:mime;base64,... format
    block["image_url"] = imageUrl;

    return block;
}

QJsonObject NetworkManager::buildFileContentBlock(const FileAttachment &file)
{
    QJsonObject block;
    block["type"] = "text";

    // File content already formatted in FileManager
    block["text"] = file.content;

    return block;
}
