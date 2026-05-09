#pragma once

#ifndef OPERATIONUNDO_H
#define OPERATIONUNDO_H

#include <QCoreApplication>
#include <QString>
#include <QVector>

#include "commandexecutor.h"
#include "operationplan.h"

struct UndoEntry {
    enum Strategy { AutoReverse, ScriptReverse, NotUndoable };

    Strategy strategy = NotUndoable;
    QString reverseCommand;  // Reverse command (for AutoReverse strategy)
    QString undoHint;        // Undo hint (for ScriptReverse strategy)
    QString description;     // Original operation description
};

class OperationUndo {
    Q_DECLARE_TR_FUNCTIONS(OperationUndo)

public:
    OperationUndo();

    void recordBefore(const ShellOperation& op);
    void recordBefore(const OperationPlan& plan);

    QVector<CommandResult> undoLastPlan();
    bool canUndo() const;

    void saveLog();
    void loadLog();

    static QString logFilePath();

private:
    QString generateReverse(const ShellOperation& op);

    QVector<UndoEntry> m_undoStack;
};

#endif  // OPERATIONUNDO_H
