#include "docimporter.h"

#include <QDebug>
#include <QFileInfo>

#include "embedder.h"
#include "fileparser.h"
#include "vectordb.h"

DocImporter::DocImporter(Embedder* embedder, VectorDB* vectorDB, QObject* parent)
        : m_embedder(embedder), m_vectorDB(vectorDB) {
    Q_UNUSED(parent);
}

DocImporter::~DocImporter() = default;

QStringList DocImporter::supportedExtensions() {
    QStringList exts = FileParser::textExtensions();
    // Add DOCX if available (it's not a standard text extension)
    if (!exts.contains(QStringLiteral("docx"))) exts.append(QStringLiteral("docx"));
    return exts;
}

bool DocImporter::isSupported(const QString& filePath) {
    QString ext = QFileInfo(filePath).suffix().toLower();
    return supportedExtensions().contains(ext);
}

ImportResult DocImporter::importDocument(const QString& filePath) {
    ImportResult result;
    result.documentPath = filePath;

    if (!QFileInfo::exists(filePath)) {
        result.errorMessage = tr("文件不存在: ") + filePath;
        return result;
    }

    if (!m_embedder || !m_vectorDB) {
        result.errorMessage = tr("Embedder 或 VectorDB 未初始化");
        return result;
    }

    QString text = FileParser::extractText(filePath);
    if (text.isEmpty()) {
        result.errorMessage = tr("无法提取文本内容: ") + filePath;
        return result;
    }

    QVector<TextChunk> chunks = m_chunker.chunkText(text, filePath);
    if (chunks.isEmpty()) {
        result.errorMessage = tr("文本分块结果为空: ") + filePath;
        return result;
    }

    QStringList chunkTexts;
    chunkTexts.reserve(chunks.size());
    for (const auto& chunk : chunks) chunkTexts.append(chunk.content);

    QVector<QVector<float>> vectors = m_embedder->embedBatch(chunkTexts);

    m_vectorDB->addVectors(vectors, chunks);
    m_vectorDB->save();

    result.success = true;
    result.chunkCount = chunks.size();
    return result;
}

QVector<ImportResult> DocImporter::importDocuments(const QStringList& filePaths) {
    QVector<ImportResult> results;
    results.reserve(filePaths.size());
    for (const auto& path : filePaths) results.append(importDocument(path));
    return results;
}
