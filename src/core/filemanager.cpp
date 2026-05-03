#include "filemanager.h"
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDebug>

#ifndef NO_PDF_SUPPORT
#include <poppler-document.h>
#include <poppler-page.h>
#endif

namespace {
    // 文本文件扩展名列表
    const QStringList TEXT_EXTENSIONS = {
        "txt", "md", "rst", "cpp", "h", "hpp", "c", "cc", "cxx",
        "py", "js", "ts", "jsx", "tsx", "java", "kt", "kts",
        "go", "rs", "rb", "php", "cs", "swift", "scala",
        "json", "xml", "yaml", "yml", "toml", "ini", "cfg", "conf",
        "html", "htm", "css", "scss", "sass", "less",
        "sql", "sh", "bash", "zsh", "bat", "cmd", "ps1",
        "dockerfile", "makefile", "cmake", "gradle", "maven",
        "log", "csv", "tsv", "env", "gitignore", "dockerignore"
#ifndef NO_PDF_SUPPORT
        , "pdf"  // PDF 文件作为特殊文本文件处理（需要 Poppler）
#endif
    };

    // 图片文件扩展名列表
    const QStringList IMAGE_EXTENSIONS = {
        "png", "jpg", "jpeg", "gif", "bmp", "webp", "ico", "svg"
    };

    // 最大文件大小限制 (10MB)
    const qint64 MAX_FILE_SIZE = 10 * 1024 * 1024;
}

FileManager::FileManager(QObject *parent)
    : QObject(parent)
    , m_pendingFiles()
{
}

bool FileManager::isTextFile(const QString &path)
{
    QFileInfo info(path);
    QString ext = info.suffix().toLower();

    // 检查扩展名
    if (TEXT_EXTENSIONS.contains(ext)) {
        return true;
    }

    // 特殊文件名（无扩展名）
    QString fileName = info.fileName().toLower();
    if (fileName == "dockerfile" || fileName == "makefile" ||
        fileName == "cmakelists.txt" || fileName.startsWith(".")) {
        return true;
    }

    return false;
}

bool FileManager::isImageFile(const QString &path)
{
    QFileInfo info(path);
    QString ext = info.suffix().toLower();
    return IMAGE_EXTENSIONS.contains(ext);
}

bool FileManager::addFile(const QString &path)
{
    // 检查文件是否存在
    QFileInfo info(path);
    if (!info.exists()) {
        qDebug() << "File does not exist:" << path;
        return false;
    }

    // 检查文件大小
    if (info.size() > MAX_FILE_SIZE) {
        qDebug() << "File too large:" << path << info.size() << "bytes";
        return false;
    }

    // 处理文件
    FileAttachment attachment = processFile(path);
    m_pendingFiles.append(attachment);

    return true;
}

QVector<FileAttachment> FileManager::pendingFiles() const
{
    return m_pendingFiles;
}

void FileManager::clearPendingFiles()
{
    m_pendingFiles.clear();
}

QString FileManager::fileListSummary() const
{
    if (m_pendingFiles.isEmpty()) {
        return "待发送文件列表为空";
    }

    QString summary = "待发送文件列表:\n";
    summary += "----------------------------------------\n";

    for (int i = 0; i < m_pendingFiles.size(); ++i) {
        const FileAttachment &file = m_pendingFiles[i];
        QString typeStr;
        if (file.type == "text") {
            typeStr = "文本";
        } else if (file.type == "image") {
            typeStr = "图片";
        } else {
            typeStr = "二进制";
        }

        summary += QString("  %1. %2 (%3, %4 bytes)\n")
            .arg(i + 1)
            .arg(file.path)
            .arg(typeStr)
            .arg(file.size);
    }

    summary += "----------------------------------------\n";
    summary += QString("共 %1 个文件\n").arg(m_pendingFiles.size());

    return summary;
}

int FileManager::pendingFileCount() const
{
    return m_pendingFiles.size();
}

FileAttachment FileManager::processFile(const QString &path)
{
    FileAttachment attachment;
    attachment.path = path;

    QFileInfo info(path);
    attachment.size = info.size();
    attachment.mimeType = getMimeType(path);

    QString ext = info.suffix().toLower();

    if (ext == "pdf") {
        // PDF 文件：使用 Poppler 提取文本
        attachment.type = "text";
        attachment.content = extractPdfText(path);
    } else if (isTextFile(path)) {
        attachment.type = "text";
        attachment.content = readTextFile(path);
    } else if (isImageFile(path)) {
        attachment.type = "image";
        attachment.content = encodeImageToBase64(path);
    } else {
        attachment.type = "binary";
        // 二进制文件不读取内容，只保留元信息
        attachment.content = QString("[二进制文件: %1 (%2 bytes, %3)]")
            .arg(info.fileName())
            .arg(info.size())
            .arg(attachment.mimeType);
    }

    return attachment;
}

QString FileManager::extractPdfText(const QString &path)
{
#ifndef NO_PDF_SUPPORT
    poppler::document *doc = poppler::document::load_from_file(path.toStdString());

    if (!doc) {
        qDebug() << "Failed to load PDF:" << path;
        return QString("[无法读取 PDF 文件: %1]").arg(QFileInfo(path).fileName());
    }

    QString content;
    QFileInfo info(path);
    content = QString("[PDF 文件: %1]\n\n").arg(info.fileName());

    int numPages = doc->pages();

    for (int i = 0; i < numPages; ++i) {
        poppler::page *page = doc->create_page(i);
        if (page) {
            poppler::ustring pageText = page->text();
            std::vector<char> utf8Data = pageText.to_utf8();
            QString qText = QString::fromUtf8(utf8Data.data(), utf8Data.size());
            content += QString("--- Page %1 ---\n").arg(i + 1);
            content += qText;
            content += "\n\n";
            delete page;
        }
    }

    delete doc;

    return content;
#else
    // 没有 Poppler 支持，返回提示信息
    QFileInfo info(path);
    return QString("[PDF 文件: %1 (%2 bytes)]\n\n注意: PDF 支持未启用，需要安装 Poppler 库")
        .arg(info.fileName())
        .arg(info.size());
#endif
}

QString FileManager::getMimeType(const QString &path)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(path);
    return mime.name();
}

QString FileManager::readTextFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open text file:" << path << file.errorString();
        return QString("[无法读取文件: %1]").arg(file.errorString());
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    // 添加文件头标识
    QFileInfo info(path);
    return QString("[文件: %1]\n\n%2").arg(info.fileName()).arg(content);
}

QString FileManager::encodeImageToBase64(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open image file:" << path << file.errorString();
        return QString();
    }

    QByteArray imageData = file.readAll();
    file.close();

    // 获取 MIME 类型用于 data URL
    QString mime = getMimeType(path);

    // Base64 编码
    QString base64 = imageData.toBase64();

    // 返回 data URL 格式
    return QString("data:%1;base64,%2").arg(mime).arg(base64);
}