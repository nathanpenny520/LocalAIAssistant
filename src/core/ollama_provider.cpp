#include "ollama_provider.h"

OllamaProvider::OllamaProvider(QObject *parent)
    : ApiProvider(parent)
{
    m_isLocalMode = true;
}

QString OllamaProvider::endpointPath() const
{
    return QStringLiteral("api/chat");
}

QJsonArray OllamaProvider::buildMessagesArray(const QVector<ChatMessage> &messages) const
{
    QJsonArray jsonMessages;

    QJsonObject systemObj;
    systemObj["role"] = "system";
    systemObj["content"] = m_systemPrompt;
    jsonMessages.append(systemObj);

    int totalCount = messages.size();
    int startIndex = qMax(0, totalCount - m_maxContext);

    for (int i = startIndex; i < totalCount; ++i) {
        const ChatMessage &msg = messages[i];
        QJsonObject msgObj;
        msgObj["role"] = msg.role;
        msgObj["content"] = msg.content;

        if (!msg.attachments.isEmpty()) {
            QJsonArray imagesArray;

            for (const FileAttachment &file : msg.attachments) {
                if (file.type == "image") {
                    QString base64Data = file.content;
                    if (base64Data.startsWith("data:")) {
                        int commaPos = base64Data.indexOf(',');
                        if (commaPos > 0)
                            base64Data = base64Data.mid(commaPos + 1);
                    }
                    imagesArray.append(base64Data);
                } else {
                    QString currentContent = msgObj["content"].toString();
                    currentContent += "\n\n" + file.content;
                    msgObj["content"] = currentContent;
                }
            }

            if (!imagesArray.isEmpty())
                msgObj["images"] = imagesArray;
        }

        jsonMessages.append(msgObj);
    }

    return jsonMessages;
}

QString OllamaProvider::extractDeltaFromSSE(const QByteArray &data)
{
    QString result;
    QString text = QString::fromUtf8(data).trimmed();

    if (text.isEmpty())
        return result;

    if (text == "[DONE]")
        return result;

    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
    if (doc.isNull() || !doc.isObject())
        return result;

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

    if (root.contains("response")) {
        result += root["response"].toString();
    } else if (root.contains("message")) {
        QJsonObject message = root["message"].toObject();
        if (message.contains("content"))
            result += message["content"].toString();
    }

    return result;
}

QString OllamaProvider::extractContentFromResponse(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject())
        return "Response format error";

    QJsonObject root = doc.object();

    if (root.contains("error")) {
        QJsonObject errorObj = root["error"].toObject();
        return "API Error: " + errorObj["message"].toString("Unknown error");
    }

    if (root.contains("response"))
        return root["response"].toString().trimmed();

    if (root.contains("message")) {
        QJsonObject message = root["message"].toObject();
        if (message.contains("content"))
            return message["content"].toString().trimmed();
    }

    return "Failed to parse valid content";
}
