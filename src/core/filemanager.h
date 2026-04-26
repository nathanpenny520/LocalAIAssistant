#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include <QVector>
#include "datamodels.h"

class FileManager : public QObject
{
    Q_OBJECT

public:
    explicit FileManager(QObject *parent = nullptr);

    // 添加文件到待发送列表，返回是否成功
    bool addFile(const QString &path);

    // 获取待发送文件列表（用于发送时取走）
    QVector<FileAttachment> pendingFiles() const;

    // 清空待发送列表
    void clearPendingFiles();

    // 获取文件列表摘要（用于 /listfiles 显示）
    QString fileListSummary() const;

    // 待发送文件数量
    int pendingFileCount() const;

    // 判断文件是否为文本类型
    static bool isTextFile(const QString &path);

    // 判断文件是否为图片类型
    static bool isImageFile(const QString &path);

private:
    FileAttachment processFile(const QString &path);
    QString getMimeType(const QString &path);
    QString readTextFile(const QString &path);
    QString encodeImageToBase64(const QString &path);

    QVector<FileAttachment> m_pendingFiles;
};

#endif