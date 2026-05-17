/**
 * @file knowledgebase.h
 * @brief Document indexing, chunked knowledge retrieval, and AI context injection.
 */
#pragma once

#ifndef KNOWLEDGEBASE_H
#define KNOWLEDGEBASE_H

#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "textchunker.h"
#include "vectordb.h"

class Embedder;
class DocImporter;
struct ImportResult;

class KnowledgeBase : public QObject {
    Q_OBJECT

public:
    static KnowledgeBase* instance();

    bool init();

    // Document management
    bool importDocument(const QString& filePath);
    QVector<ImportResult> importDocuments(const QStringList& filePaths);
    void importDocumentAsync(const QString& filePath);
    void importDocumentsAsync(const QStringList& filePaths);
    bool removeDocument(const QString& filePath);
    QStringList allDocuments() const;

    // Semantic search
    QVector<SearchResult> search(const QString& query, int topK = 5) const;

    // Build AI context injection text
    QString generateContext(const QString& query, int topK = 5) const;

    // Detect whether a message is a knowledge-base query
    static bool isKnowledgeQuery(const QString& message);

    bool isReady() const;
    int totalChunks() const;
    int totalDocuments() const;

    Embedder* embedder() const;
    VectorDB* vectorDB() const;

    static QString storageDir();

signals:
    void documentImported(const QString& path, int chunks);
    void documentRemoved(const QString& path);
    void importFailed(const QString& path, const QString& error);

private:
    explicit KnowledgeBase(QObject* parent = nullptr);
    ~KnowledgeBase() override;

    static KnowledgeBase* s_instance;

    Embedder* m_embedder;
    VectorDB* m_vectorDB;
    DocImporter* m_importer;
    bool m_ready = false;
    QMutex m_mutex;
};

#endif  // KNOWLEDGEBASE_H
