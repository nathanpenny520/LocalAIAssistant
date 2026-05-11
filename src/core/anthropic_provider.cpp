#include "anthropic_provider.h"

AnthropicProvider::AnthropicProvider(QObject* parent) : ApiProvider(parent) {
    m_isLocalMode = false;
}

QString AnthropicProvider::endpointPath() const {
    return QStringLiteral("v1/messages");
}

void AnthropicProvider::configureRequest(QNetworkRequest& request) const {
    if (!m_apiKey.isEmpty()) {
        request.setRawHeader("x-api-key", m_apiKey.toUtf8());
    }
    request.setRawHeader("anthropic-version", "2023-06-01");
}

QJsonObject AnthropicProvider::buildBasePayload() const {
    QJsonObject payload;
    payload["model"] = m_modelName;
    payload["max_tokens"] = m_maxTokens;
    payload["stream"] = m_streamingEnabled;
    // Anthropic: system prompt is top-level, not a message
    payload["system"] = m_systemPrompt;

    // Anthropic does not support presence_penalty or frequency_penalty.
    // temperature and top_p are mutually exclusive; we send temperature.
    payload["temperature"] = m_temperature;

    if (m_seed.has_value()) payload["seed"] = m_seed.value();

    return payload;
}

QJsonArray AnthropicProvider::buildMessagesArray(const QVector<ChatMessage>& messages) const {
    QJsonArray jsonMessages;
    // No system message — system prompt is at top level via buildBasePayload()

    int totalCount = messages.size();
    int startIndex = computeContextStartIndex(messages);

    for (int i = startIndex; i < totalCount; ++i) {
        const ChatMessage& msg = messages[i];
        QJsonObject msgObj;
        msgObj["role"] = msg.role;

        if (msg.attachments.isEmpty()) {
            msgObj["content"] = msg.content;
        } else {
            QJsonArray contentArray;

            if (!msg.content.isEmpty()) contentArray.append(buildTextContentBlock(msg.content));

            for (const FileAttachment& file : msg.attachments) {
                if (file.type == "image")
                    contentArray.append(buildAnthropicImageBlock(file.content, file.mimeType));
                else
                    contentArray.append(buildFileContentBlock(file));
            }

            msgObj["content"] = contentArray;
        }

        jsonMessages.append(msgObj);
    }

    return jsonMessages;
}

QJsonObject AnthropicProvider::buildAnthropicImageBlock(const QString& base64Data,
                                                        const QString& mime) const {
    QJsonObject block;
    block["type"] = "image";

    QJsonObject source;
    source["type"] = "base64";
    source["media_type"] = mime.isEmpty() ? QStringLiteral("image/png") : mime;

    // Strip data URL prefix if present (OpenAI format uses data:image/png;base64,...)
    QString data = base64Data;
    if (data.startsWith("data:")) {
        int commaPos = data.indexOf(',');
        if (commaPos > 0) data = data.mid(commaPos + 1);
    }
    source["data"] = data;

    block["source"] = source;
    return block;
}

QString AnthropicProvider::extractDeltaFromSSE(const QByteArray& data) {
    QString result;
    QString text = QString::fromUtf8(data).trimmed();

    if (text.isEmpty()) return result;

    // Anthropic SSE: event: <type>\ndata: <json>
    QStringList lines = text.split('\n');
    QString currentEvent;

    for (const QString& line : lines) {
        if (line.startsWith("event: ")) {
            currentEvent = line.mid(7).trimmed();
        } else if (line.startsWith("data: ")) {
            QString jsonStr = line.mid(6).trimmed();
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
            if (doc.isNull() || !doc.isObject()) continue;

            QJsonObject root = doc.object();

            if (root.contains("error")) {
                QJsonObject errorObj = root["error"].toObject();
                emit errorOccurred(errorObj["message"].toString("Unknown error"));
                return result;
            }

            // Extract text from content_block_delta events
            QString eventType = currentEvent.isEmpty() ? root["type"].toString() : currentEvent;
            if (eventType == "content_block_delta") {
                QJsonObject delta = root["delta"].toObject();
                if (delta["type"].toString() == "text_delta" && delta.contains("text"))
                    result += delta["text"].toString();
            }
        }
    }

    return result;
}

QString AnthropicProvider::extractContentFromResponse(const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) return "Response format error";

    QJsonObject root = doc.object();

    if (root.contains("error")) {
        QJsonObject errorObj = root["error"].toObject();
        return "API Error: " + errorObj["message"].toString("Unknown error");
    }

    // Anthropic response: content is an array of blocks
    if (root.contains("content") && root["content"].isArray()) {
        QJsonArray content = root["content"].toArray();
        QString result;
        for (const QJsonValue& val : content) {
            QJsonObject block = val.toObject();
            if (block["type"].toString() == "text") result += block["text"].toString();
        }
        return result.trimmed();
    }

    return "Failed to parse valid content";
}
