#pragma once

#ifndef COMMANDEXECUTOR_H
#define COMMANDEXECUTOR_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QElapsedTimer>
#include <QCoreApplication>
#include "operationplan.h"

struct CommandResult
{
    bool success = false;
    int exitCode = -1;
    QString stdoutOutput;
    QString stderrOutput;
    QString errorMessage;
    qint64 elapsedMs = 0;
};

class CommandExecutor : public QObject
{
    Q_OBJECT

public:
    explicit CommandExecutor(QObject *parent = nullptr);
    ~CommandExecutor() override;

    // 单条命令执行
    CommandResult execute(const ShellOperation &op);

    // 批量执行（遇错即停）
    QVector<CommandResult> executePlan(const OperationPlan &plan);

    // 取消当前执行
    void cancel();

    // 路径展开
    static QString expandPath(const QString &path);

signals:
    void stdoutLineReceived(const QString &line, int operationIndex);
    void stderrLineReceived(const QString &line, int operationIndex);
    void operationStarted(int index, const QString &command);
    void operationFinished(int index, const CommandResult &result);

private:
    CommandResult runCommand(const QString &command,
                             const QString &workingDir,
                             int timeoutSecs,
                             const QStringList &extraEnv);

    QString buildShell() const;

    QProcess *m_currentProcess = nullptr;
    bool m_cancelled = false;
    int m_currentOpIndex = 0;
};

#endif // COMMANDEXECUTOR_H
