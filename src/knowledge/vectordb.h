#pragma once

#ifndef VECTORDB_H
#define VECTORDB_H

#include <QString>
#include <QStringList>
#include <QVector>

#include "textchunker.h"

#ifdef HNSWLIB_AVAILABLE
namespace hnswlib {
template <typename dist_t>
class SpaceInterface;
template <typename dist_t>
class HierarchicalNSW;
typedef size_t labeltype;
}  // namespace hnswlib
#endif

struct SearchResult {
    TextChunk chunk;
    float similarity;  // cosine similarity, range 0~1
};

class VectorDB {
public:
    VectorDB();
    ~VectorDB();

    bool init(int dimension, const QString& storageDir);
    void addVectors(const QVector<QVector<float>>& vectors, const QVector<TextChunk>& chunks);
    QVector<SearchResult> search(const QVector<float>& queryVector, int topK = 5) const;
    int removeDocument(const QString& documentPath);
    int totalChunks() const;
    int totalDocuments() const;
    QStringList allDocuments() const;
    bool save();
    bool load();

private:
    bool initStorage(const QString& dir);

    int m_dimension = 0;
    QString m_storageDir;

#ifdef HNSWLIB_AVAILABLE
    bool ensureIndex();
    // L2-normalized vectors → inner product = cosine similarity
    hnswlib::SpaceInterface<float>* m_space = nullptr;
    mutable hnswlib::HierarchicalNSW<float>* m_index = nullptr;
    int m_activeChunks = 0;
#else
    QVector<QVector<float>> m_vectors;
#endif
    QVector<TextChunk> m_chunks;
};

#endif  // VECTORDB_H
