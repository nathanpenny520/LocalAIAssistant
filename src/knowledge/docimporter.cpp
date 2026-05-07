#include "docimporter.h"
#include "embedder.h"
#include "vectordb.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>
#include <memory>
#ifdef POPPLER_FOUND
#include <poppler-document.h>
#include <poppler-page.h>
#endif
#ifdef LIBZIP_AVAILABLE
#include <zip.h>
#endif
#ifdef PUGIXML_FOUND
#define PUGIXML_HEADER_ONLY
#include <pugixml.hpp>
#endif

DocImporter::DocImporter(Embedder *embedder, VectorDB *vectorDB, QObject *parent)
    : m_embedder(embedder), m_vectorDB(vectorDB)
{
    Q_UNUSED(parent);
}

DocImporter::~DocImporter() = default;

QStringList DocImporter::supportedExtensions()
{
    return {
        QStringLiteral("txt"), QStringLiteral("md"), QStringLiteral("markdown"),
        QStringLiteral("rst"), QStringLiteral("org"),
#ifdef POPPLER_FOUND
        QStringLiteral("pdf"),
#endif
#if defined(LIBZIP_AVAILABLE) && defined(PUGIXML_FOUND)
        QStringLiteral("docx"),
#endif
    };
}

bool DocImporter::isSupported(const QString &filePath)
{
    QString ext = QFileInfo(filePath).suffix().toLower();
    return supportedExtensions().contains(ext);
}

ImportResult DocImporter::importDocument(const QString &filePath)
{
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

    // 提取文本
    QString text = extractText(filePath);
    if (text.isEmpty()) {
        result.errorMessage = tr("无法提取文本内容: ") + filePath;
        return result;
    }

    // 分块
    QVector<TextChunk> chunks = m_chunker.chunkText(text, filePath);
    if (chunks.isEmpty()) {
        result.errorMessage = tr("文本分块结果为空: ") + filePath;
        return result;
    }

    // 向量化
    QStringList chunkTexts;
    chunkTexts.reserve(chunks.size());
    for (const auto &chunk : chunks)
        chunkTexts.append(chunk.content);

    QVector<QVector<float>> vectors = m_embedder->embedBatch(chunkTexts);

    // 存储
    m_vectorDB->addVectors(vectors, chunks);
    m_vectorDB->save();

    result.success = true;
    result.chunkCount = chunks.size();
    return result;
}

QVector<ImportResult> DocImporter::importDocuments(const QStringList &filePaths)
{
    QVector<ImportResult> results;
    results.reserve(filePaths.size());
    for (const auto &path : filePaths) {
        results.append(importDocument(path));
    }
    return results;
}

QString DocImporter::extractText(const QString &filePath) const
{
    QString ext = QFileInfo(filePath).suffix().toLower();

    if (ext == QStringLiteral("txt") || ext == QStringLiteral("md")
        || ext == QStringLiteral("markdown") || ext == QStringLiteral("rst")
        || ext == QStringLiteral("org")) {
        return extractPlainText(filePath);
    }

    if (ext == QStringLiteral("pdf")) {
        return extractPdfText(filePath);
    }

    if (ext == QStringLiteral("docx")) {
        return extractDocxText(filePath);
    }

    return {};
}

QString DocImporter::extractPlainText(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    // 尝试 UTF-8，失败则用系统编码
    stream.setAutoDetectUnicode(true);

    QString content = stream.readAll();
    file.close();
    return content;
}

QString DocImporter::extractPdfText(const QString &filePath) const
{
#ifdef POPPLER_FOUND
    std::unique_ptr<poppler::document> doc(
        poppler::document::load_from_file(filePath.toStdString())
    );
    if (!doc) {
        qWarning() << "Failed to load PDF document:" << filePath;
        return {};
    }

    QString fullText;
    int totalPages = doc->pages();

    for (int i = 0; i < totalPages; ++i) {
        std::unique_ptr<poppler::page> page(doc->create_page(i));
        if (!page) {
            continue;
        }

        // poppler::page::text() returns poppler::ustring
        poppler::ustring pageText = page->text(poppler::text_layout_enum::physical_layout);
        // Convert to QString via UTF-8
        std::string utf8Text = pageText.to_string();
        QString qText = QString::fromUtf8(utf8Text.c_str(), static_cast<int>(utf8Text.size()));

        if (!qText.isEmpty()) {
            fullText += qText;
            // Add page separator for multi-page documents
            if (i < totalPages - 1) {
                fullText += QStringLiteral("\n\n--- Page %1 ---\n\n").arg(i + 2);
            }
        }
    }

    // Clean up common PDF extraction artifacts
    fullText = fullText.trimmed();
    // Remove excessive whitespace while preserving paragraph structure
    fullText.replace(QRegularExpression(QStringLiteral("[ \t]+")), QStringLiteral(" "));
    fullText.replace(QRegularExpression(QStringLiteral("\n{3,}")), QStringLiteral("\n\n"));

    return fullText;
#else
    Q_UNUSED(filePath);
    return {};
#endif
}

QString DocImporter::extractDocxText(const QString &filePath) const
{
#if defined(LIBZIP_AVAILABLE) && defined(PUGIXML_FOUND)
    // Step 1: Open DOCX as ZIP archive
    int err = 0;
    zip_t *zip = zip_open(filePath.toUtf8().constData(), ZIP_RDONLY, &err);
    if (!zip) {
        qWarning() << "Failed to open DOCX as ZIP:" << filePath << "error:" << err;
        return {};
    }

    // Step 2: Locate word/document.xml in the archive
    zip_stat_t stat;
    if (zip_stat(zip, "word/document.xml", 0, &stat) != 0) {
        qWarning() << "word/document.xml not found in DOCX:" << filePath;
        zip_close(zip);
        return {};
    }

    zip_file_t *zf = zip_fopen(zip, "word/document.xml", 0);
    if (!zf) {
        qWarning() << "Failed to open word/document.xml in:" << filePath;
        zip_close(zip);
        return {};
    }

    QByteArray xmlData(static_cast<int>(stat.size), Qt::Uninitialized);
    zip_int64_t bytesRead = zip_fread(zf, xmlData.data(), static_cast<zip_uint64_t>(stat.size));
    if (bytesRead < 0 || static_cast<zip_uint64_t>(bytesRead) != stat.size) {
        qWarning() << "Failed to read word/document.xml from:" << filePath;
        zip_fclose(zf);
        zip_close(zip);
        return {};
    }
    zip_fclose(zf);
    zip_close(zip);

    // Step 3: Parse XML with pugixml
    pugi::xml_document xmlDoc;
    pugi::xml_parse_result parseResult = xmlDoc.load_buffer_inplace(
        xmlData.data(), xmlData.size(),
        pugi::parse_default | pugi::parse_ws_pcdata
    );

    if (!parseResult) {
        qWarning() << "Failed to parse word/document.xml:" << parseResult.description();
        return {};
    }

    // Step 4: Extract text from <w:t> elements organized by paragraph <w:p>
    QString fullText;
    auto paragraphs = xmlDoc.select_nodes(
        "//*[local-name()='p' and namespace-uri()='http://schemas.openxmlformats.org/wordprocessingml/2006/main']"
    );

    for (const auto &pNode : paragraphs) {
        QString paraText;
        auto textNodes = pNode.node().select_nodes(
            ".//*[local-name()='t' and namespace-uri()='http://schemas.openxmlformats.org/wordprocessingml/2006/main']"
        );

        for (const auto &tNode : textNodes) {
            paraText += QString::fromStdString(tNode.node().text().get());
        }

        if (!paraText.isEmpty()) {
            fullText += paraText.trimmed() + QStringLiteral("\n");
        } else {
            fullText += QStringLiteral("\n");
        }
    }

    fullText.replace(QRegularExpression(QStringLiteral("\n{3,}")), QStringLiteral("\n\n"));
    return fullText.trimmed();

#else
    Q_UNUSED(filePath);
    return {};
#endif
}
