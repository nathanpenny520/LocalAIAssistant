#pragma once

#ifndef KNOWLEDGEBASE_H
#define KNOWLEDGEBASE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include "textchunker.h"
#include "vectordb.h"

class Embedder;
class DocImporter;
struct ImportResult;

class KnowledgeBase : public QObject
{
    Q_OBJECT

public:
    static KnowledgeBase *instance();

    // 初始化知识库
    bool init();

    // 文档管理
    bool importDocument(const QString &filePath);
    QVector<ImportResult> importDocuments(const QStringList &filePaths);
    void importDocumentAsync(const QString &filePath);
    void importDocumentsAsync(const QStringList &filePaths);
    bool removeDocument(const QString &filePath);
    QStringList allDocuments() const;

    // 语义搜索
    QVector<SearchResult> search(const QString &query, int topK = 5) const;

    // 生成 AI 上下文注入文本
    QString generateContext(const QString &query, int topK = 5) const;

    // 检测是否为知识库查询
    static bool isKnowledgeQuery(const QString &message);

    // 状态
    bool isReady() const;
    int totalChunks() const;
    int totalDocuments() const;

    // 获取组件
    Embedder *embedder() const;
    VectorDB *vectorDB() const;

    static QString storageDir();

signals:
    void documentImported(const QString &path, int chunks);
    void documentRemoved(const QString &path);
    void importFailed(const QString &path, const QString &error);

private:
    explicit KnowledgeBase(QObject *parent = nullptr);
    ~KnowledgeBase() override;

    static KnowledgeBase *s_instance;

    Embedder *m_embedder;
    VectorDB *m_vectorDB;
    DocImporter *m_importer;
    bool m_ready = false;
};

#endif // KNOWLEDGEBASE_H
