#include "openai_provider.h"

OpenAIProvider::OpenAIProvider(QObject* parent) : ApiProvider(parent) {
    m_isLocalMode = false;
}

QString OpenAIProvider::endpointPath() const {
    return QStringLiteral("v1/chat/completions");
}

void OpenAIProvider::configureRequest(QNetworkRequest& request) const {
    if (!m_isLocalMode && !m_apiKey.isEmpty()) {
        QString authHeader = QString("Bearer %1").arg(m_apiKey);
        request.setRawHeader("Authorization", authHeader.toUtf8());
    }
}

QJsonArray OpenAIProvider::buildMessagesArray(const QVector<ChatMessage>& messages) const {
    QJsonArray jsonMessages;

    QJsonObject systemObj;
    systemObj["role"] = "system";
    systemObj["content"] = m_systemPrompt;
    jsonMessages.append(systemObj);

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
                    contentArray.append(buildImageContentBlock(file.content, file.mimeType));
                else
                    contentArray.append(buildFileContentBlock(file));
            }

            msgObj["content"] = contentArray;
        }

        jsonMessages.append(msgObj);
    }

    return jsonMessages;
}

QString OpenAIProvider::extractDeltaFromSSE(const QByteArray& data) {
    QString result;
    QString text = QString::fromUtf8(data).trimmed();

    if (text.isEmpty()) return result;

    if (text == "[DONE]") return result;

    QStringList lines = text.split('\n');
    for (const QString& line : lines) {
        if (!line.startsWith("data: ")) continue;

        QString jsonStr = line.mid(6).trimmed();
        if (jsonStr == "[DONE]") break;

        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (doc.isNull() || !doc.isObject()) continue;

        QJsonObject root = doc.object();
        if (root.contains("error")) {
            QJsonObject errorObj = root["error"].toObject();
            emit errorOccurred(errorObj["message"].toString("Unknown error"));
            break;
        }

        if (root.contains("choices") && root["choices"].isArray()) {
            QJsonArray choices = root["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject firstChoice = choices[0].toObject();
                QJsonObject delta = firstChoice["delta"].toObject();
                if (delta.contains("content")) result += delta["content"].toString();
            }
        }
    }

    return result;
}

QString OpenAIProvider::extractContentFromResponse(const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) return "Response format error";

    QJsonObject root = doc.object();

    if (root.contains("error")) {
        QJsonObject errorObj = root["error"].toObject();
        return "API Error: " + errorObj["message"].toString("Unknown error");
    }

    if (root.contains("choices") && root["choices"].isArray()) {
        QJsonArray choices = root["choices"].toArray();
        if (!choices.isEmpty()) {
            QJsonObject firstChoice = choices[0].toObject();

            if (firstChoice.contains("message")) {
                QJsonObject message = firstChoice["message"].toObject();
                if (message.contains("content")) return message["content"].toString().trimmed();
            }

            if (firstChoice.contains("finish_reason")) {
                QString finishReason = firstChoice["finish_reason"].toString();
                if (finishReason == "length")
                    return "Warning: Response truncated due to length limit";
            }
        }
    }

    return "Failed to parse valid content";
}
