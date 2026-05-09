#include "textchunker.h"

#include <QRegularExpression>

TextChunker::TextChunker() = default;

void TextChunker::setMaxTokensPerChunk(int maxTokens) {
    m_maxTokensPerChunk = maxTokens;
}

int TextChunker::maxTokensPerChunk() const {
    return m_maxTokensPerChunk;
}

int TextChunker::estimateTokens(const QString& text) {
    int tokens = 0;
    for (int i = 0; i < text.length(); ++i) {
        QChar ch = text[i];
        // 中文字符和中文标点算 1 token
        if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF)
            tokens += 1;
        else if (ch.unicode() >= 0x3000 && ch.unicode() <= 0x303F)
            tokens += 1;  // CJK 标点
        else if (ch.unicode() >= 0xFF00 && ch.unicode() <= 0xFFEF)
            tokens += 1;  // 全角字符
        else if (ch.isLetterOrNumber())
            tokens += 1;
        // 其他字符（空格、标点等）不额外计数，归入相邻单词
    }

    // 英文按空格分词修正：粗略估算为字符数/4 + 中文字数
    int chineseChars = 0;
    int otherChars = 0;
    for (int i = 0; i < text.length(); ++i) {
        QChar ch = text[i];
        if ((ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF) ||
            (ch.unicode() >= 0x3000 && ch.unicode() <= 0x303F) ||
            (ch.unicode() >= 0xFF00 && ch.unicode() <= 0xFFEF)) {
            chineseChars++;
        } else if (!ch.isSpace()) {
            otherChars++;
        }
    }

    // 中文字符 ~1 token/字，英文 ~1 token/4字符
    return chineseChars + (otherChars / 4) + 1;
}

QVector<TextChunk> TextChunker::chunkText(const QString& text, const QString& documentPath) const {
    QStringList paragraphs = splitParagraphs(text);
    return mergeParagraphs(paragraphs, documentPath);
}

QStringList TextChunker::splitParagraphs(const QString& text) const {
    // 按双换行分段落
    QStringList paragraphs =
            text.split(QRegularExpression(QStringLiteral("\n\n+")), Qt::SkipEmptyParts);

    QStringList result;
    for (const auto& para : paragraphs) {
        QString trimmed = para.trimmed();
        if (!trimmed.isEmpty()) result.append(trimmed);
    }

    return result;
}

QVector<TextChunk> TextChunker::mergeParagraphs(const QStringList& paragraphs,
                                                const QString& documentPath) const {
    QVector<TextChunk> chunks;
    if (paragraphs.isEmpty()) return chunks;

    QString currentText;
    int currentTokens = 0;
    int chunkIndex = 0;
    int charOffset = 0;

    for (const auto& para : paragraphs) {
        int paraTokens = estimateTokens(para);

        // 如果单个段落超过上限，需要拆分
        if (paraTokens > m_maxTokensPerChunk) {
            // 先保存当前累积的 chunk
            if (!currentText.isEmpty()) {
                TextChunk chunk;
                chunk.content = currentText.trimmed();
                chunk.documentPath = documentPath;
                chunk.chunkIndex = chunkIndex++;
                chunk.charOffset = charOffset;
                chunk.estimatedTokens = currentTokens;
                chunks.append(chunk);
                charOffset += currentText.length() + 2;  // +2 for \n\n
                currentText.clear();
                currentTokens = 0;
            }

            // 对超长段落按句子拆分
            QStringList sentences =
                    para.split(QRegularExpression(QStringLiteral("(?<=[。！？.!?])\\s*")));
            for (const auto& sent : sentences) {
                int sentTokens = estimateTokens(sent);
                if (currentTokens + sentTokens > m_maxTokensPerChunk && !currentText.isEmpty()) {
                    TextChunk chunk;
                    chunk.content = currentText.trimmed();
                    chunk.documentPath = documentPath;
                    chunk.chunkIndex = chunkIndex++;
                    chunk.charOffset = charOffset;
                    chunk.estimatedTokens = currentTokens;
                    chunks.append(chunk);
                    charOffset += currentText.length() + 1;
                    currentText.clear();
                    currentTokens = 0;
                }
                if (!sent.trimmed().isEmpty()) {
                    if (!currentText.isEmpty()) currentText += QStringLiteral(" ");
                    currentText += sent.trimmed();
                    currentTokens += sentTokens;
                }
            }
        }
        // 如果加上当前段落会超出上限，保存当前 chunk
        else if (currentTokens + paraTokens > m_maxTokensPerChunk && !currentText.isEmpty()) {
            TextChunk chunk;
            chunk.content = currentText.trimmed();
            chunk.documentPath = documentPath;
            chunk.chunkIndex = chunkIndex++;
            chunk.charOffset = charOffset;
            chunk.estimatedTokens = currentTokens;
            chunks.append(chunk);

            charOffset += currentText.length() + 2;
            currentText = para;
            currentTokens = paraTokens;
        } else {
            if (!currentText.isEmpty()) currentText += QStringLiteral("\n\n");
            currentText += para;
            currentTokens += paraTokens;
        }
    }

    // 保存最后一个 chunk
    if (!currentText.isEmpty()) {
        TextChunk chunk;
        chunk.content = currentText.trimmed();
        chunk.documentPath = documentPath;
        chunk.chunkIndex = chunkIndex;
        chunk.charOffset = charOffset;
        chunk.estimatedTokens = currentTokens;
        chunks.append(chunk);
    }

    return chunks;
}
