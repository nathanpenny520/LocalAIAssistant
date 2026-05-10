#include "fileparser.h"

#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QRegularExpression>
#include <QTextStream>

#ifndef NO_PDF_SUPPORT
#include <poppler-document.h>
#include <poppler-page.h>
#endif

#if defined(HAS_LIBZIP) && defined(HAS_PUGIXML)
#include <pugixml.hpp>
#include <zip.h>
#endif

static const QStringList kTextExtensions = {"txt",
                                            "md",
                                            "markdown",
                                            "rst",
                                            "org",
                                            "cpp",
                                            "h",
                                            "hpp",
                                            "c",
                                            "cc",
                                            "cxx",
                                            "py",
                                            "js",
                                            "ts",
                                            "jsx",
                                            "tsx",
                                            "java",
                                            "kt",
                                            "kts",
                                            "go",
                                            "rs",
                                            "rb",
                                            "php",
                                            "cs",
                                            "swift",
                                            "scala",
                                            "json",
                                            "xml",
                                            "yaml",
                                            "yml",
                                            "toml",
                                            "ini",
                                            "cfg",
                                            "conf",
                                            "html",
                                            "htm",
                                            "css",
                                            "scss",
                                            "sass",
                                            "less",
                                            "sql",
                                            "sh",
                                            "bash",
                                            "zsh",
                                            "bat",
                                            "cmd",
                                            "ps1",
                                            "dockerfile",
                                            "makefile",
                                            "cmake",
                                            "gradle",
                                            "maven",
                                            "log",
                                            "csv",
                                            "tsv",
                                            "env",
                                            "gitignore",
                                            "dockerignore"
#ifndef NO_PDF_SUPPORT
                                            ,
                                            "pdf"
#endif
};

static const QStringList kImageExtensions = {"png", "jpg",  "jpeg", "gif",
                                             "bmp", "webp", "ico",  "svg"};

const QStringList& FileParser::textExtensions() {
    return kTextExtensions;
}

const QStringList& FileParser::imageExtensions() {
    return kImageExtensions;
}

bool FileParser::isTextFile(const QString& path) {
    QFileInfo info(path);
    QString ext = info.suffix().toLower();

    if (kTextExtensions.contains(ext)) return true;

    QString fileName = info.fileName().toLower();
    if (fileName == "dockerfile" || fileName == "makefile" || fileName == "cmakelists.txt" ||
        fileName.startsWith('.'))
        return true;

    return false;
}

bool FileParser::isImageFile(const QString& path) {
    QFileInfo info(path);
    return kImageExtensions.contains(info.suffix().toLower());
}

QString FileParser::mimeType(const QString& path) {
    QMimeDatabase db;
    return db.mimeTypeForFile(path).name();
}

QString FileParser::extractPlainText(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream.setAutoDetectUnicode(true);

    QString content = stream.readAll();
    file.close();
    return content;
}

QString FileParser::extractPdfText(const QString& path) {
#ifndef NO_PDF_SUPPORT
    std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(path.toStdString()));

    if (!doc) {
        qWarning() << "Failed to load PDF document:" << path;
        return {};
    }

    QString fullText;
    int totalPages = doc->pages();

    for (int i = 0; i < totalPages; ++i) {
        std::unique_ptr<poppler::page> page(doc->create_page(i));
        if (!page) continue;

        poppler::ustring pageText = page->text();
        std::vector<char> utf8Data = pageText.to_utf8();
        QString qText = QString::fromUtf8(utf8Data.data(), utf8Data.size());

        if (!qText.isEmpty()) {
            fullText += qText;
            if (i < totalPages - 1)
                fullText += QStringLiteral("\n\n--- Page %1 ---\n\n").arg(i + 2);
        }
    }

    fullText = fullText.trimmed();
    fullText.replace(QRegularExpression(QStringLiteral("[ \t]+")), QStringLiteral(" "));
    fullText.replace(QRegularExpression(QStringLiteral("\n{3,}")), QStringLiteral("\n\n"));

    return fullText;
#else
    Q_UNUSED(path);
    return {};
#endif
}

QString FileParser::extractDocxText(const QString& path) {
#if defined(HAS_LIBZIP) && defined(HAS_PUGIXML)
    int err = 0;
    zip_t* zip = zip_open(path.toUtf8().constData(), ZIP_RDONLY, &err);
    if (!zip) {
        qWarning() << "Failed to open DOCX as ZIP:" << path << "error:" << err;
        return {};
    }

    zip_stat_t stat;
    if (zip_stat(zip, "word/document.xml", 0, &stat) != 0) {
        qWarning() << "word/document.xml not found in DOCX:" << path;
        zip_close(zip);
        return {};
    }

    zip_file_t* zf = zip_fopen(zip, "word/document.xml", 0);
    if (!zf) {
        qWarning() << "Failed to open word/document.xml in:" << path;
        zip_close(zip);
        return {};
    }

    QByteArray xmlData(static_cast<int>(stat.size), Qt::Uninitialized);
    zip_int64_t bytesRead = zip_fread(zf, xmlData.data(), static_cast<zip_uint64_t>(stat.size));
    if (bytesRead < 0 || static_cast<zip_uint64_t>(bytesRead) != stat.size) {
        qWarning() << "Failed to read word/document.xml from:" << path;
        zip_fclose(zf);
        zip_close(zip);
        return {};
    }
    zip_fclose(zf);
    zip_close(zip);

    pugi::xml_document xmlDoc;
    pugi::xml_parse_result parseResult = xmlDoc.load_buffer_inplace(
            xmlData.data(), xmlData.size(), pugi::parse_default | pugi::parse_ws_pcdata);

    if (!parseResult) {
        qWarning() << "Failed to parse word/document.xml:" << parseResult.description();
        return {};
    }

    QString fullText;
    auto paragraphs = xmlDoc.select_nodes(
            "//*[local-name()='p' and "
            "namespace-uri()='http://schemas.openxmlformats.org/wordprocessingml/2006/main']");

    for (const auto& pNode : paragraphs) {
        QString paraText;
        auto textNodes = pNode.node().select_nodes(
                ".//*[local-name()='t' and "
                "namespace-uri()='http://schemas.openxmlformats.org/wordprocessingml/2006/main']");

        for (const auto& tNode : textNodes)
            paraText += QString::fromStdString(tNode.node().text().get());

        if (!paraText.isEmpty())
            fullText += paraText.trimmed() + QStringLiteral("\n");
        else
            fullText += QStringLiteral("\n");
    }

    fullText.replace(QRegularExpression(QStringLiteral("\n{3,}")), QStringLiteral("\n\n"));
    return fullText.trimmed();
#else
    Q_UNUSED(path);
    return {};
#endif
}

QString FileParser::encodeImageToBase64(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};

    QByteArray imageData = file.readAll();
    file.close();

    QString mime = mimeType(path);
    QString base64 = QString::fromUtf8(imageData.toBase64());

    return QStringLiteral("data:%1;base64,%2").arg(mime, base64);
}

QString FileParser::extractText(const QString& path) {
    QString ext = QFileInfo(path).suffix().toLower();

    if (ext == QStringLiteral("pdf")) return extractPdfText(path);

    if (ext == QStringLiteral("docx")) return extractDocxText(path);

    // Default: plain text
    return extractPlainText(path);
}
