#pragma once

#ifndef TEXTCHUNKER_H
#define TEXTCHUNKER_H

#include <QString>
#include <QVector>

struct TextChunk {
    QString content;
    QString documentPath;  // 源文档路径
    int chunkIndex;        // 在文档中的序号
    int charOffset;        // 在文档中的字符偏移
    int estimatedTokens;   // 估算 token 数
};

class TextChunker {
public:
    TextChunker();

    void setMaxTokensPerChunk(int maxTokens);
    int maxTokensPerChunk() const;

    // 将文本按段落分块，保证每个chunk不超过 maxTokens
    QVector<TextChunk> chunkText(const QString& text, const QString& documentPath) const;

    // 估算文本的 token 数（中文字数 + 英文单词数）
    static int estimateTokens(const QString& text);

private:
    QStringList splitParagraphs(const QString& text) const;
    QVector<TextChunk> mergeParagraphs(const QStringList& paragraphs,
                                       const QString& documentPath) const;

    int m_maxTokensPerChunk = 512;
};

#endif  // TEXTCHUNKER_H
