#pragma once

#ifndef DOCIMPORTER_H
#define DOCIMPORTER_H

#include <QCoreApplication>
#include <QString>
#include <QStringList>

#include "textchunker.h"

class Embedder;
class VectorDB;

struct ImportResult {
    bool success = false;
    QString documentPath;
    int chunkCount = 0;
    QString errorMessage;
};

class DocImporter {
    Q_DECLARE_TR_FUNCTIONS(DocImporter)

public:
    DocImporter(Embedder* embedder, VectorDB* vectorDB, QObject* parent = nullptr);
    ~DocImporter();

    ImportResult importDocument(const QString& filePath);
    QVector<ImportResult> importDocuments(const QStringList& filePaths);

    static QStringList supportedExtensions();
    static bool isSupported(const QString& filePath);

private:
    Embedder* m_embedder;
    VectorDB* m_vectorDB;
    TextChunker m_chunker;
};

#endif  // DOCIMPORTER_H
