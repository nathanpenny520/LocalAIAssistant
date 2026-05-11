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
        // Chinese characters and CJK punctuation count as 1 token each
        if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF)
            tokens += 1;
        else if (ch.unicode() >= 0x3000 && ch.unicode() <= 0x303F)
            tokens += 1;  // CJK punctuation
        else if (ch.unicode() >= 0xFF00 && ch.unicode() <= 0xFFEF)
            tokens += 1;  // fullwidth characters
        else if (ch.isLetterOrNumber())
            tokens += 1;
        // Other characters (spaces, punctuation) not counted individually; absorbed into adjacent words
    }

    // Refine estimate using word-based heuristic: ~chars/4 for English + ~1/char for Chinese
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

    // Chinese ~1 token/char, English ~1 token per 4 characters
    return chineseChars + (otherChars / 4) + 1;
}

QVector<TextChunk> TextChunker::chunkText(const QString& text, const QString& documentPath) const {
    QStringList paragraphs = splitParagraphs(text);
    return mergeParagraphs(paragraphs, documentPath);
}

QStringList TextChunker::splitParagraphs(const QString& text) const {
    // Split on double (or more) newlines into paragraphs
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

        // If a single paragraph exceeds the token limit, split it
        if (paraTokens > m_maxTokensPerChunk) {
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

            // Split the oversized paragraph by sentence boundaries
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
        // Adding this paragraph would exceed the limit — flush current chunk first
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
