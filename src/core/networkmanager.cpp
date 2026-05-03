#include "networkmanager.h"
#include "datamodels.h"
#include <QDebug>
#include <QBuffer>
#include <QDir>
#include <QStandardPaths>

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

void NetworkManager::setSystemPrompt(const QString &prompt)
{
    m_systemPrompt = prompt;
}

void NetworkManager::abortCurrentRequest()
{
    if (m_currentReply) {
        // Disconnect signals first to prevent abort-triggered finished signal from causing crash
        m_currentReply->disconnect(this);
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    m_streamBuffer.clear();
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

    // seed handling: -1 means not set (null)
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
    // Try multiple possible paths
    QStringList possiblePaths;

    // 1. Application directory
    QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_MACOS
    // macOS app bundle structure
    possiblePaths << QDir::cleanPath(appDir + "/../Resources/core/soul.md");
#elif defined(Q_OS_WIN)
    // Windows: 资源在可执行文件同级目录
    possiblePaths << QDir::cleanPath(appDir + "/core/soul.md");
#else
    // Linux
    possiblePaths << QDir::cleanPath(appDir + "/core/soul.md");
    possiblePaths << "/usr/share/localaiassistant/core/soul.md";
    possiblePaths << "/usr/local/share/localaiassistant/core/soul.md";
#endif

    // 用户数据目录
    possiblePaths << QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/core/soul.md");

    // 开发时的相对路径
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

    // Fallback: built-in default prompt
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

    // seed save: -1 means null
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

    // Trim whitespace from URL
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
    jsonPayload["temperature"] = m_temperature;                  // Use member variable instead of hardcoded
    jsonPayload["top_p"] = m_topP;                               // Added
    jsonPayload["max_tokens"] = m_maxTokens;
    jsonPayload["presence_penalty"] = m_presencePenalty;
    jsonPayload["frequency_penalty"] = m_frequencyPenalty;       // Added
    jsonPayload["stream"] = m_streamingEnabled;

    // seed only passed when has value
    if (m_seed.has_value()) {
        jsonPayload["seed"] = m_seed.value();
    }

    // Use messages format, pass isOllama parameter to support different formats
    jsonPayload["messages"] = buildMessagesArray(messages, m_maxContext, isOllama);

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

    // Check if it's Ollama
    bool isOllama = m_apiBaseUrl.contains("11434");

    QByteArray newData = m_currentReply->readAll();

    if (isOllama) {
        // For Ollama, read data line by line
        QString chunk = extractDeltaFromSSE(newData, true);
        if (!chunk.isEmpty()) {
            m_streamBuffer += chunk;
            emit streamChunkReceived(chunk);
        }
    } else {
        // For OpenAI format, extract content
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

    // 读取剩余数据
    QByteArray remainingData = m_currentReply->readAll();
    if (!remainingData.isEmpty()) {
        QString chunk = extractDeltaFromSSE(remainingData);
        if (!chunk.isEmpty()) {
            m_streamBuffer += chunk;
        }
    }

    if (!m_streamBuffer.isEmpty()) {
        emit streamFinished(m_streamBuffer);
        m_streamBuffer.clear();
    }

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
        // For Ollama, parse JSON directly
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

        // Handle Ollama /api/generate streaming response format
        if (root.contains("response")) {
            result += root["response"].toString();
        }
        // Handle Ollama /api/chat response format
        else if (root.contains("message")) {
            QJsonObject message = root["message"].toObject();
            if (message.contains("content")) {
                result += message["content"].toString();
            }
        }
    } else {
        // For other servers, keep the original processing
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

            // Handle OpenAI response format
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
        return "API Error: " + errorMessage;
    }

    // Handle Ollama /api/generate response format
    if (root.contains("response")) {
        QString content = root["response"].toString();
        content = content.trimmed();
        return content;
    }
    // Handle Ollama /api/chat response format
    else if (root.contains("message")) {
        QJsonObject message = root["message"].toObject();
        if (message.contains("content")) {
            QString content = message["content"].toString();
            content = content.trimmed();
            return content;
        }
    }
    // Handle OpenAI response format
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
                    return "Warning: Response truncated due to length limit";
                }
            }
        }
    }

    return "Failed to parse valid content";
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
