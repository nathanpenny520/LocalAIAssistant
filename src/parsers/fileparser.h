#pragma once

#ifndef FILEPARSER_H
#define FILEPARSER_H

#include <QString>
#include <QStringList>

class FileParser {
public:
    FileParser() = delete;

    // Type detection
    static bool isTextFile(const QString& path);
    static bool isImageFile(const QString& path);
    static QString mimeType(const QString& path);

    // Raw text extraction (no headers/wrappers; returns empty QString on failure)
    static QString extractPlainText(const QString& path);
    static QString extractPdfText(const QString& path);
    static QString extractDocxText(const QString& path);

    // Image encoding (returns data: URL or empty QString on failure)
    static QString encodeImageToBase64(const QString& path);

    // Auto-detect type and extract text; falls back to plain text read
    static QString extractText(const QString& path);

    // Extension lists
    static const QStringList& textExtensions();
    static const QStringList& imageExtensions();
};

#endif  // FILEPARSER_H
