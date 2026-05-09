#pragma once

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include <QVector>

#include "datamodels.h"

class FileManager : public QObject {
    Q_OBJECT

public:
    explicit FileManager(QObject* parent = nullptr);

    bool addFile(const QString& path);
    QVector<FileAttachment> pendingFiles() const;
    void clearPendingFiles();
    QString fileListSummary() const;
    int pendingFileCount() const;

    static bool isTextFile(const QString& path);
    static bool isImageFile(const QString& path);

private:
    FileAttachment processFile(const QString& path);

    QVector<FileAttachment> m_pendingFiles;
};

#endif
