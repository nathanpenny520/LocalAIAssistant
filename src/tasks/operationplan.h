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
    QString command;       // 命令/脚本内容 (ShellCommand/ShellScript/WriteFile/SearchFiles)
    QString source;        // 源路径 (MoveFile/CopyFile/DeleteFile/SearchFiles)
    QString target;        // 目标路径 (CreateDir/MoveFile/CopyFile/WriteFile)
    QString workingDir;    // 工作目录
    QString description;   // 人类可读描述（必须填写）
    int timeoutSecs = 30;  // 超时秒数

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
