#pragma once

#ifndef TEXTCHUNKER_H
#define TEXTCHUNKER_H

#include <QString>
#include <QVector>

struct TextChunk {
    QString content;
    QString documentPath;  // source document path
    int chunkIndex;        // sequence number within the document
    int charOffset;        // character offset within the document
    int estimatedTokens;   // estimated token count
};

class TextChunker {
public:
    TextChunker();

    void setMaxTokensPerChunk(int maxTokens);
    int maxTokensPerChunk() const;

    // Split text into paragraph-based chunks, each under the maxTokens limit
    QVector<TextChunk> chunkText(const QString& text, const QString& documentPath) const;

    // Rough token count estimate (Chinese: 1 token/char, English: ~1 token per 4 chars)
    static int estimateTokens(const QString& text);

private:
    QStringList splitParagraphs(const QString& text) const;
    QVector<TextChunk> mergeParagraphs(const QStringList& paragraphs,
                                       const QString& documentPath) const;

    int m_maxTokensPerChunk = 512;
};

#endif  // TEXTCHUNKER_H
