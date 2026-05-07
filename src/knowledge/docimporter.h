#pragma once

#ifndef DOCIMPORTER_H
#define DOCIMPORTER_H

#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include "textchunker.h"

class Embedder;
class VectorDB;

struct ImportResult
{
    bool success = false;
    QString documentPath;
    int chunkCount = 0;
    QString errorMessage;
};

class DocImporter
{
    Q_DECLARE_TR_FUNCTIONS(DocImporter)

public:
    DocImporter(Embedder *embedder, VectorDB *vectorDB, QObject *parent = nullptr);
    ~DocImporter();

    // 导入单个文档
    ImportResult importDocument(const QString &filePath);

    // 批量导入
    QVector<ImportResult> importDocuments(const QStringList &filePaths);

    // 支持的扩展名
    static QStringList supportedExtensions();
    static bool isSupported(const QString &filePath);

private:
    QString extractText(const QString &filePath) const;

    // TXT/MD 文本提取
    QString extractPlainText(const QString &filePath) const;
    // PDF 文本提取（需要 Poppler）
    QString extractPdfText(const QString &filePath) const;
    // Word DOCX 文本提取（需要 pugixml）
    QString extractDocxText(const QString &filePath) const;

    Embedder *m_embedder;
    VectorDB *m_vectorDB;
    TextChunker m_chunker;
};

#endif // DOCIMPORTER_H
