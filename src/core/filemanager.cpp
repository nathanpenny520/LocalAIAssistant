#include "filemanager.h"

#include <QFileInfo>

#include "fileparser.h"

namespace {
const qint64 kMaxFileSize = 10 * 1024 * 1024;
}

FileManager::FileManager(QObject* parent) : QObject(parent), m_pendingFiles() {
}

bool FileManager::isTextFile(const QString& path) {
    return FileParser::isTextFile(path);
}

bool FileManager::isImageFile(const QString& path) {
    return FileParser::isImageFile(path);
}

bool FileManager::addFile(const QString& path) {
    QFileInfo info(path);
    if (!info.exists()) {
        return false;
    }

    if (info.size() > kMaxFileSize) {
        return false;
    }

    m_pendingFiles.append(processFile(path));
    return true;
}

QVector<FileAttachment> FileManager::pendingFiles() const {
    return m_pendingFiles;
}

void FileManager::clearPendingFiles() {
    m_pendingFiles.clear();
}

QString FileManager::fileListSummary() const {
    if (m_pendingFiles.isEmpty()) return QStringLiteral("待发送文件列表为空");

    QString summary = QStringLiteral("待发送文件列表:\n");
    summary += QStringLiteral("----------------------------------------\n");

    for (int i = 0; i < m_pendingFiles.size(); ++i) {
        const FileAttachment& file = m_pendingFiles[i];
        QString typeStr;
        if (file.type == QStringLiteral("text"))
            typeStr = QStringLiteral("文本");
        else if (file.type == QStringLiteral("image"))
            typeStr = QStringLiteral("图片");
        else
            typeStr = QStringLiteral("二进制");

        summary += QString("  %1. %2 (%3, %4 bytes)\n")
                           .arg(i + 1)
                           .arg(file.path)
                           .arg(typeStr)
                           .arg(file.size);
    }

    summary += QStringLiteral("----------------------------------------\n");
    summary += QString("共 %1 个文件\n").arg(m_pendingFiles.size());

    return summary;
}

int FileManager::pendingFileCount() const {
    return m_pendingFiles.size();
}

FileAttachment FileManager::processFile(const QString& path) {
    FileAttachment attachment;
    attachment.path = path;

    QFileInfo info(path);
    attachment.size = info.size();
    attachment.mimeType = FileParser::mimeType(path);

    QString ext = info.suffix().toLower();

    if (ext == QStringLiteral("pdf")) {
        attachment.type = QStringLiteral("text");
        QString rawText = FileParser::extractPdfText(path);
        if (rawText.isEmpty()) {
            attachment.content = QStringLiteral("[无法读取 PDF 文件: %1]").arg(info.fileName());
        } else {
            attachment.content =
                    QStringLiteral("[PDF 文件: %1]\n\n%2").arg(info.fileName()).arg(rawText);
        }
    } else if (ext == QStringLiteral("docx")) {
        attachment.type = QStringLiteral("text");
        QString rawText = FileParser::extractDocxText(path);
        if (rawText.isEmpty()) {
            attachment.content = QStringLiteral("[无法读取 DOCX 文件: %1]").arg(info.fileName());
        } else {
            attachment.content =
                    QStringLiteral("[DOCX 文件: %1]\n\n%2").arg(info.fileName()).arg(rawText);
        }
    } else if (FileParser::isTextFile(path)) {
        attachment.type = QStringLiteral("text");
        QString rawText = FileParser::extractPlainText(path);
        if (rawText.isEmpty()) {
            attachment.content = QStringLiteral("[无法读取文件: %1]").arg(path);
        } else {
            attachment.content =
                    QStringLiteral("[文件: %1]\n\n%2").arg(info.fileName()).arg(rawText);
        }
    } else if (FileParser::isImageFile(path)) {
        attachment.type = QStringLiteral("image");
        attachment.content = FileParser::encodeImageToBase64(path);
    } else {
        attachment.type = QStringLiteral("binary");
        attachment.content = QStringLiteral("[二进制文件: %1 (%2 bytes, %3)]")
                                     .arg(info.fileName())
                                     .arg(info.size())
                                     .arg(attachment.mimeType);
    }

    return attachment;
}
