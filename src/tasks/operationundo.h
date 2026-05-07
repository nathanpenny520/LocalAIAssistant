#pragma once

#ifndef OPERATIONUNDO_H
#define OPERATIONUNDO_H

#include <QString>
#include <QVector>
#include <QCoreApplication>
#include "operationplan.h"
#include "commandexecutor.h"

struct UndoEntry
{
    enum Strategy { AutoReverse, ScriptReverse, NotUndoable };

    Strategy strategy = NotUndoable;
    QString reverseCommand;   // 逆向命令（AutoReverse 时）
    QString undoHint;         // 撤销提示（ScriptReverse 时）
    QString description;      // 原始操作描述
};

class OperationUndo
{
    Q_DECLARE_TR_FUNCTIONS(OperationUndo)

public:
    OperationUndo();

    void recordBefore(const ShellOperation &op);
    void recordBefore(const OperationPlan &plan);

    QVector<CommandResult> undoLastPlan();
    bool canUndo() const;

    void saveLog();
    void loadLog();

    static QString logFilePath();

private:
    QString generateReverse(const ShellOperation &op);

    QVector<UndoEntry> m_undoStack;
};

#endif // OPERATIONUNDO_H
