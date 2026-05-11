/**
 * @file operationplan.h
 * @brief Data structures for planned shell and file operations with safety metadata.
 */
#pragma once

#ifndef OPERATIONPLAN_H
#define OPERATIONPLAN_H

#include <QCoreApplication>
#include <QString>
#include <QVariantMap>
#include <QVector>

struct ShellOperation {
    static inline QString tr(const char* s, const char* c = nullptr, int n = -1) {
        return QCoreApplication::translate("ShellOperation", s, c, n);
    }

    enum Type {
        ShellCommand,
        ShellScript,
        WriteFile,
        SearchFiles,
        CreateDir,
        MoveFile,
        DeleteFile,
        CopyFile
    };
    Type type = ShellCommand;
    QString command;       // Command/script content (ShellCommand/ShellScript/WriteFile/SearchFiles)
    QString source;        // Source path (MoveFile/CopyFile/DeleteFile/SearchFiles)
    QString target;        // Target path (CreateDir/MoveFile/CopyFile/WriteFile)
    QString workingDir;    // Working directory
    QString description;   // Human-readable description (required)
    int timeoutSecs = 30;  // Timeout in seconds

    QString typeName() const;
    QString dangerLabel() const;  // Safe / Caution / Dangerous
};

struct OperationPlan {
    static inline QString tr(const char* s, const char* c = nullptr, int n = -1) {
        return QCoreApplication::translate("OperationPlan", s, c, n);
    }

    QString description;
    QVector<ShellOperation> operations;
    bool requiresConfirmation = true;

    QString generateSummary() const;
    QString generateShellPreview() const;
    bool isEmpty() const;
    int totalOperations() const;
};

#endif  // OPERATIONPLAN_H
