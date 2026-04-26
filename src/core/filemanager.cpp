#include "filemanager.h"
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDebug>

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

    if (isTextFile(path)) {
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