#include "vectordb.h"

#include <algorithm>

#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

#include "embedder.h"

#ifdef HNSWLIB_AVAILABLE
#include "hnswlib/hnswlib.h"
#endif

// ============================================================
// VectorDB — vector database for semantic search
// hnswlib: approximate nearest neighbor index (HNSW algorithm)
// Fallback: flat index with brute-force search
// Chunk metadata: SQLite (documents + chunks tables)
// ============================================================

VectorDB::VectorDB() = default;

VectorDB::~VectorDB() {
#ifdef HNSWLIB_AVAILABLE
    delete m_index;
    delete m_space;
#endif
}

bool VectorDB::init(int dimension, const QString& storageDir) {
    m_dimension = dimension;
    m_storageDir = storageDir;
    return initStorage(storageDir);
}

bool VectorDB::initStorage(const QString& dir) {
    if (!QDir().mkpath(dir)) {
        qWarning() << "VectorDB: Cannot create storage dir:" << dir;
        return false;
    }

    QString dbPath = dir + QStringLiteral("/chunks.db");

    const QString connName = QStringLiteral("vectordb_conn");
    if (QSqlDatabase::contains(connName)) QSqlDatabase::removeDatabase(connName);

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qWarning() << "VectorDB: Cannot open database:" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);

    query.exec(
            QStringLiteral("CREATE TABLE IF NOT EXISTS documents ("
                           "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "  path TEXT UNIQUE NOT NULL,"
                           "  import_time TEXT NOT NULL"
                           ")"));

    query.exec(
            QStringLiteral("CREATE TABLE IF NOT EXISTS chunks ("
                           "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "  doc_id INTEGER NOT NULL,"
                           "  content TEXT NOT NULL,"
                           "  chunk_index INTEGER NOT NULL,"
                           "  char_offset INTEGER NOT NULL,"
                           "  estimated_tokens INTEGER NOT NULL,"
                           "  FOREIGN KEY (doc_id) REFERENCES documents(id) ON DELETE CASCADE"
                           ")"));

    // Metadata table (dimension, index type)
    query.exec(
            QStringLiteral("CREATE TABLE IF NOT EXISTS metadata ("
                           "  key TEXT PRIMARY KEY,"
                           "  value TEXT NOT NULL"
                           ")"));

    if (query.lastError().isValid()) {
        qWarning() << "VectorDB: Cannot create tables:" << query.lastError().text();
        return false;
    }

    // Store dimension
    query.prepare(
            QStringLiteral("INSERT OR REPLACE INTO metadata (key, value) VALUES ('dimension', ?)"));
    query.addBindValue(QString::number(m_dimension));
    query.exec();

    db.close();
    return true;
}

#ifdef HNSWLIB_AVAILABLE
bool VectorDB::ensureIndex() {
    if (m_index) return true;

    if (m_dimension <= 0) return false;

    m_space = new hnswlib::InnerProductSpace(m_dimension);
    // Start with moderate capacity; hnswlib resizes as needed
    m_index = new hnswlib::HierarchicalNSW<float>(m_space, 1000, 16, 200, 100, false);
    return true;
}
#endif

void VectorDB::addVectors(const QVector<QVector<float>>& vectors,
                          const QVector<TextChunk>& chunks) {
    if (vectors.size() != chunks.size()) {
        qWarning() << "VectorDB: vectors and chunks size mismatch";
        return;
    }

    // Verify dimension consistency
    for (int i = 0; i < vectors.size(); ++i) {
        if (vectors[i].size() != m_dimension) {
            qWarning() << "VectorDB: vector" << i << "has wrong dimension" << vectors[i].size()
                       << "(expected" << m_dimension << ")";
            return;
        }
    }

    const QString connName = QStringLiteral("vectordb_conn");
    if (!QSqlDatabase::contains(connName)) {
        qWarning() << "VectorDB: database connection not found";
        return;
    }

    QSqlDatabase db = QSqlDatabase::database(connName);
    if (!db.isOpen()) db.open();

    // Cache doc_id per path
    QMap<QString, int> pathToDocId;
    QSqlQuery query(db);

    for (int i = 0; i < vectors.size(); ++i) {
        const auto& chunk = chunks[i];

        // Resolve doc_id (lookup once per document)
        int docId = -1;
        const auto it = pathToDocId.constFind(chunk.documentPath);
        if (it != pathToDocId.constEnd()) {
            docId = it.value();
        } else {
            query.prepare(
                    QStringLiteral("INSERT OR IGNORE INTO documents (path, import_time) VALUES (?, "
                                   "?)"));
            query.addBindValue(chunk.documentPath);
            query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
            if (!query.exec()) {
                qWarning() << "VectorDB: insert doc failed:" << query.lastError().text();
                continue;
            }

            query.prepare(QStringLiteral("SELECT id FROM documents WHERE path = ?"));
            query.addBindValue(chunk.documentPath);
            if (!query.exec() || !query.next()) {
                qWarning() << "VectorDB: failed to get doc_id for" << chunk.documentPath;
                continue;
            }
            docId = query.value(0).toInt();
            pathToDocId[chunk.documentPath] = docId;
        }

        // Insert chunk record
        query.prepare(QStringLiteral(
                "INSERT INTO chunks (doc_id, content, chunk_index, char_offset, estimated_tokens) "
                "VALUES (?, ?, ?, ?, ?)"));
        query.addBindValue(docId);
        query.addBindValue(chunk.content);
        query.addBindValue(chunk.chunkIndex);
        query.addBindValue(chunk.charOffset);
        query.addBindValue(chunk.estimatedTokens);
        if (!query.exec()) {
            qWarning() << "VectorDB: insert chunk failed:" << query.lastError().text();
            continue;
        }

        // Memory index
#ifdef HNSWLIB_AVAILABLE
        if (!ensureIndex()) continue;
        hnswlib::labeltype label = static_cast<hnswlib::labeltype>(m_chunks.size());
        m_index->addPoint(vectors[i].constData(), label);
        m_chunks.append(chunk);
        ++m_activeChunks;
#else
        m_vectors.append(vectors[i]);
        m_chunks.append(chunk);
#endif
    }

    // Persist HNSW index after adding vectors so data survives restarts
    save();
}

QVector<SearchResult> VectorDB::search(const QVector<float>& queryVector, int topK) const {
    QVector<SearchResult> results;

    if (queryVector.size() != m_dimension) return results;

#ifdef HNSWLIB_AVAILABLE
    if (!m_index || m_activeChunks == 0) return results;

    // ef must be >= topK for correct recall
    m_index->setEf(static_cast<size_t>(qMax(50, topK)));

    auto knn = m_index->searchKnnCloserFirst(queryVector.constData(), static_cast<size_t>(topK));

    results.reserve(static_cast<int>(knn.size()));
    for (const auto& [dist, label] : knn) {
        int idx = static_cast<int>(label);
        if (idx < 0 || idx >= m_chunks.size()) continue;
        const TextChunk& chunk = m_chunks[idx];
        if (chunk.content.isEmpty())  // deleted marker
            continue;

        SearchResult sr;
        sr.chunk = chunk;
        // Inner product on L2-normalized vectors = cosine similarity
        sr.similarity = qBound(0.0f, dist, 1.0f);
        results.append(sr);
    }
#else
    if (m_vectors.isEmpty()) return results;

    struct ScoredIndex {
        int index;
        float score;
    };

    QVector<ScoredIndex> scored;
    scored.reserve(m_vectors.size());

    for (int i = 0; i < m_vectors.size(); ++i) {
        float sim = Embedder::cosineSimilarity(queryVector, m_vectors[i]);
        scored.append({i, sim});
    }

    std::sort(scored.begin(), scored.end(),
              [](const ScoredIndex& a, const ScoredIndex& b) { return a.score > b.score; });

    int count = qMin(topK, scored.size());
    results.reserve(count);
    for (int i = 0; i < count; ++i) {
        SearchResult sr;
        sr.chunk = m_chunks[scored[i].index];
        sr.similarity = scored[i].score;
        results.append(sr);
    }
#endif

    return results;
}

int VectorDB::removeDocument(const QString& documentPath) {
    const QString connName = QStringLiteral("vectordb_conn");
    if (!QSqlDatabase::contains(connName)) return 0;

    QSqlDatabase db = QSqlDatabase::database(connName);
    if (!db.isOpen()) db.open();

    QSqlQuery query(db);

    query.prepare(QStringLiteral("SELECT id FROM documents WHERE path = ?"));
    query.addBindValue(documentPath);
    if (!query.exec() || !query.next()) {
        qWarning() << "VectorDB: document not found for removal:" << documentPath;
        return 0;
    }

    int docId = query.value(0).toInt();

    query.prepare(QStringLiteral("DELETE FROM chunks WHERE doc_id = ?"));
    query.addBindValue(docId);
    if (!query.exec()) {
        qWarning() << "VectorDB: failed to delete chunks:" << query.lastError().text();
        return 0;
    }
    int removedChunks = query.numRowsAffected();

    query.prepare(QStringLiteral("DELETE FROM documents WHERE id = ?"));
    query.addBindValue(docId);
    if (!query.exec()) {
        qWarning() << "VectorDB: failed to delete document record:" << query.lastError().text();
        return 0;
    }

#ifdef HNSWLIB_AVAILABLE
    // Mark labels as deleted in hnswlib (labels = chunk indices in m_chunks)
    for (int i = 0; i < m_chunks.size(); ++i) {
        if (m_chunks[i].documentPath == documentPath && !m_chunks[i].content.isEmpty()) {
            m_index->markDelete(static_cast<hnswlib::labeltype>(i));
            m_chunks[i].content.clear();  // mark as deleted
            --m_activeChunks;
        }
    }
#else
    // Remove from flat index (iterate backwards for safe removal)
    for (int i = m_chunks.size() - 1; i >= 0; --i) {
        if (m_chunks[i].documentPath == documentPath) {
            m_chunks.removeAt(i);
            m_vectors.removeAt(i);
        }
    }

    if (m_vectors.size() != m_chunks.size()) {
        qWarning() << "VectorDB: vectors/chunks mismatch after removeDocument — clearing vectors";
        m_vectors.clear();
    }
#endif

    if (!save()) qWarning() << "VectorDB: failed to save after document removal";
    return removedChunks;
}

int VectorDB::totalChunks() const {
#ifdef HNSWLIB_AVAILABLE
    return m_activeChunks;
#else
    return m_chunks.size();
#endif
}

int VectorDB::totalDocuments() const {
    return allDocuments().size();
}

QStringList VectorDB::allDocuments() const {
    const QString connName = QStringLiteral("vectordb_conn");
    if (!QSqlDatabase::contains(connName)) return {};

    QSqlDatabase db = QSqlDatabase::database(connName);
    if (!db.isOpen()) db.open();

    QSqlQuery query(db);
    query.exec(QStringLiteral("SELECT path FROM documents ORDER BY import_time DESC"));

    QStringList docs;
    while (query.next()) docs.append(query.value(0).toString());
    return docs;
}

bool VectorDB::save() {
    if (m_storageDir.isEmpty()) return false;

#ifdef HNSWLIB_AVAILABLE
    if (m_index) {
        try {
            QString indexPath = m_storageDir + QStringLiteral("/hnsw.index");
            m_index->saveIndex(indexPath.toStdString());
        } catch (const std::exception& e) {
            qWarning() << "VectorDB: saveIndex failed:" << e.what();
            return false;
        }
    }
#else
    QString indexPath = m_storageDir + QStringLiteral("/vectors.bin");
    QFile file(indexPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "VectorDB: Cannot write vectors file:" << indexPath;
        return false;
    }

    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_6_0);

    stream << m_dimension << static_cast<qint64>(m_vectors.size());

    for (const auto& vec : m_vectors) {
        for (float v : vec) stream << v;
    }

    file.close();
#endif

    return true;
}

bool VectorDB::load() {
    if (m_storageDir.isEmpty()) return false;

    // Load chunk metadata from SQLite
    const QString connName = QStringLiteral("vectordb_conn");
    QSqlDatabase db = QSqlDatabase::database(connName);
    if (!db.isOpen()) db.open();

    // Verify stored dimension matches
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT value FROM metadata WHERE key = 'dimension'"));
    if (query.exec() && query.next()) {
        int storedDim = query.value(0).toInt();
        if (storedDim != m_dimension && m_dimension > 0) {
            qWarning() << "VectorDB: dimension mismatch (stored:" << storedDim
                       << "current:" << m_dimension << "), re-indexing";
        }
    }

    // Load chunks from SQLite
    query.exec(QStringLiteral(
            "SELECT c.content, c.chunk_index, c.char_offset, c.estimated_tokens, d.path "
            "FROM chunks c JOIN documents d ON c.doc_id = d.id "
            "ORDER BY d.id, c.chunk_index"));

    m_chunks.clear();
    while (query.next()) {
        TextChunk chunk;
        chunk.content = query.value(0).toString();
        chunk.chunkIndex = query.value(1).toInt();
        chunk.charOffset = query.value(2).toInt();
        chunk.estimatedTokens = query.value(3).toInt();
        chunk.documentPath = query.value(4).toString();
        m_chunks.append(chunk);
    }

#ifdef HNSWLIB_AVAILABLE
    // Clean up any existing index
    delete m_index;
    m_index = nullptr;
    delete m_space;
    m_space = nullptr;
    m_activeChunks = 0;

    if (m_dimension <= 0) return !m_chunks.isEmpty();

    m_space = new hnswlib::InnerProductSpace(m_dimension);

    QString indexPath = m_storageDir + QStringLiteral("/hnsw.index");

    if (QFile::exists(indexPath)) {
        try {
            // Use the file-based constructor
            m_index = new hnswlib::HierarchicalNSW<float>(m_space, indexPath.toStdString(),
                                                          false,   // nmslib format
                                                          0,       // max_elements (0 = use file
                                                                   // contents)
                                                          false);  // allow_replace_deleted

            m_activeChunks = static_cast<int>(m_index->getCurrentElementCount());

            // Detect stale HNSW index: vector count != SQLite chunk count
            if (m_activeChunks != m_chunks.size()) {
                qWarning() << "VectorDB: HNSW/chunk mismatch detected —" << m_activeChunks
                           << "vectors vs" << m_chunks.size() << "chunks, reinitializing...";

                delete m_index;
                m_index = nullptr;
                delete m_space;
                m_space = nullptr;
                m_chunks.clear();
                m_activeChunks = 0;

                // Transactionally clear SQLite
                {
                    QSqlDatabase db = QSqlDatabase::database(connName);
                    if (db.isOpen()) {
                        db.transaction();
                        QSqlQuery q(db);
                        bool ok = q.exec(QStringLiteral("DELETE FROM chunks"));
                        ok &= q.exec(QStringLiteral("DELETE FROM documents"));
                        if (ok) {
                            db.commit();
                            qInfo() << "VectorDB: database cleared successfully";
                        } else {
                            db.rollback();
                            qCritical() << "VectorDB: failed to clear database:"
                                        << q.lastError().text();
                        }
                    }
                }

                // Delete stale HNSW index file from disk
                QFile hnswFile(m_storageDir + QStringLiteral("/hnsw.index"));
                if (hnswFile.exists()) hnswFile.remove();

                // Create fresh empty index, let flow continue
                if (m_dimension > 0) {
                    m_space = new hnswlib::InnerProductSpace(m_dimension);
                    m_index =
                            new hnswlib::HierarchicalNSW<float>(m_space, 1000, 16, 200, 100, false);
                }
            } else {
                qInfo() << "VectorDB: loaded hnsw index," << m_activeChunks
                        << "vectors, dimension:" << m_dimension;
            }
        } catch (const std::exception& e) {
            qWarning() << "VectorDB: failed to load hnsw index:" << e.what()
                       << "— creating new index, re-embedding needed";
            delete m_space;
            m_space = nullptr;
            m_index = nullptr;
            m_chunks.clear();
            ensureIndex();
        }
    } else {
        // No saved index — create fresh
        int maxElements = qMax(m_chunks.size(), 1000);
        m_index = new hnswlib::HierarchicalNSW<float>(m_space, maxElements, 16, 200, 100, false);
        // If there are chunks in SQLite but no hnsw index, vectors need re-embedding.
        // Clear chunks so caller can detect this and re-index.
        if (!m_chunks.isEmpty()) {
            qWarning() << "VectorDB: chunks exist in SQLite but hnsw index missing"
                       << "— re-embedding needed";
            m_chunks.clear();
        }
    }

    return true;
#else
    // Load vectors from binary file
    QString indexPath = m_storageDir + QStringLiteral("/vectors.bin");
    QFile file(indexPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "VectorDB: Cannot read vectors file (may be empty):" << indexPath;
        return !m_chunks.isEmpty();
    }

    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_6_0);

    int dim;
    qint64 count64;
    stream >> dim >> count64;

    if (stream.status() != QDataStream::Ok) {
        qWarning() << "VectorDB: failed to read vectors header";
        return !m_chunks.isEmpty();
    }

    m_dimension = dim;
    int count = static_cast<int>(count64);

    m_vectors.clear();
    m_vectors.reserve(count);

    for (int i = 0; i < count; ++i) {
        QVector<float> vec(dim);
        for (int j = 0; j < dim; ++j) stream >> vec[j];
        m_vectors.append(vec);
    }

    if (m_vectors.size() != count) {
        qWarning() << "VectorDB: loaded" << m_vectors.size() << "vectors but expected" << count
                   << "— discarding cached vectors (re-embedding needed)";
        m_vectors.clear();
        return !m_chunks.isEmpty();
    }
    if (m_vectors.size() != m_chunks.size()) {
        qWarning() << "VectorDB: vectors/chunks count mismatch (" << m_vectors.size() << "vs"
                   << m_chunks.size() << "), re-embedding needed";
        m_vectors.clear();
        return !m_chunks.isEmpty();
    }

    file.close();
    return true;
#endif
}
