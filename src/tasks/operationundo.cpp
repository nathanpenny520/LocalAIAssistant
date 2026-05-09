#include "operationundo.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>

OperationUndo::OperationUndo() {
    loadLog();
}

QString OperationUndo::generateReverse(const ShellOperation& op) {
    // ── Native file operations: generate appropriate reverse ──
    switch (op.type) {
        case ShellOperation::CreateDir:
            return QStringLiteral("__script_reverse__");  // Can't auto-remove non-empty dirs
        case ShellOperation::MoveFile:
            // Reverse: move back
            return QStringLiteral("__native_reverse_move__");
        case ShellOperation::CopyFile:
            // Reverse: delete the copy
            return QStringLiteral("__native_reverse_delete__");
        case ShellOperation::DeleteFile:
            return QStringLiteral("__script_reverse__");  // Files are gone
        case ShellOperation::WriteFile:
            return QStringLiteral("__script_reverse__");  // Content overwritten
        case ShellOperation::SearchFiles:
            return QStringLiteral("__not_undoable__");  // Read-only
        default:
            break;
    }

    // ── Shell commands: parse command string ──
    const QString cmd = op.command.trimmed();

    // 检测 mkdir -p PATH → rmdir PATH
    static QRegularExpression mkdirRe(QStringLiteral("^mkdir\\s+(-p\\s+)?\"?([^\"]+)\"?$"));
    QRegularExpressionMatch m = mkdirRe.match(cmd);
    if (m.hasMatch()) {
        return QStringLiteral("rmdir \"%1\"").arg(m.captured(2));
    }

    // 检测 mv SRC DEST → mv DEST SRC
    static QRegularExpression mvRe(QStringLiteral("^mv\\s+\"?(.+?)\"?\\s+\"?(.+?)\"?$"));
    m = mvRe.match(cmd);
    if (m.hasMatch()) {
        QString src = m.captured(1);
        QString dest = m.captured(2);
        return QStringLiteral("mv \"%1\" \"%2\"").arg(dest, src);
    }

    // 检测 cp -r SRC DEST → rm -rf DEST
    static QRegularExpression cpRe(
            QStringLiteral("^cp\\s+(-[a-zA-Z]*r[a-zA-Z]*\\s+)?\"?(.+?)\"?\\s+\"?(.+?)\"?$"));
    m = cpRe.match(cmd);
    if (m.hasMatch()) {
        QString dest = m.captured(3);
        return QStringLiteral("rm -rf \"%1\"").arg(dest);
    }

    // 检测 rm 操作 → ScriptReverse（无法自动恢复文件内容）
    static QRegularExpression rmRe(QStringLiteral("^rm\\b"));
    if (rmRe.match(cmd).hasMatch()) {
        return QStringLiteral("__script_reverse__");
    }

    // 复杂操作（含管道、条件判断、循环）→ ScriptReverse
    if (cmd.contains(QLatin1Char('|')) || cmd.contains(QStringLiteral("&&")) ||
        cmd.contains(QStringLiteral("||")) ||
        cmd.contains(QRegularExpression("\\b(for|while|if|case)\\b"))) {
        return QStringLiteral("__script_reverse__");
    }

    // 网络操作 → NotUndoable
    if (cmd.contains(QRegularExpression("\\b(curl|wget|git\\s+push|rsync|scp)\\b"))) {
        return QStringLiteral("__not_undoable__");
    }

    // 其他命令不可自动撤销 → ScriptReverse
    return QStringLiteral("__script_reverse__");
}

void OperationUndo::recordBefore(const ShellOperation& op) {
    UndoEntry entry;
    entry.description = op.description;

    QString reverse = generateReverse(op);
    if (reverse == QStringLiteral("__not_undoable__")) {
        return;
    } else if (reverse == QStringLiteral("__native_reverse_move__")) {
        entry.strategy = UndoEntry::AutoReverse;
        // Store source/dest swap as a native MoveFile reverse
        entry.reverseCommand = QStringLiteral("__native_move:%1:%2").arg(op.target, op.source);
        m_undoStack.append(entry);
    } else if (reverse == QStringLiteral("__native_reverse_delete__")) {
        entry.strategy = UndoEntry::AutoReverse;
        // Reverse a CopyFile by deleting the target (original is still there)
        entry.reverseCommand = QStringLiteral("__native_delete:%1").arg(op.target);
        m_undoStack.append(entry);
    } else if (reverse == QStringLiteral("__script_reverse__")) {
        entry.strategy = UndoEntry::ScriptReverse;
        if (op.type == ShellOperation::DeleteFile) {
            entry.undoHint = tr("文件已删除，无法自动恢复。请从备份恢复。");
        } else if (op.type == ShellOperation::WriteFile) {
            entry.undoHint = tr("文件内容已被覆盖，无法自动恢复。");
        } else if (op.type == ShellOperation::CreateDir) {
            entry.undoHint = tr("目录已创建，请手动删除空目录以撤销。");
        } else if (op.command.contains(QRegularExpression("\\brm\\b"))) {
            entry.undoHint = tr("文件已删除，无法自动恢复。请从备份或 Time Machine 恢复。");
        } else if (op.command.contains(QLatin1Char('|')) ||
                   op.command.contains(QStringLiteral("&&"))) {
            entry.undoHint = tr("复杂命令，需手动撤销。建议确认操作结果。");
        } else {
            entry.undoHint = tr("此操作无自动逆向命令，请手动检查和撤销。");
        }
        m_undoStack.append(entry);
    } else if (!reverse.isEmpty()) {
        entry.strategy = UndoEntry::AutoReverse;
        entry.reverseCommand = reverse;
        m_undoStack.append(entry);
    } else {
        entry.strategy = UndoEntry::ScriptReverse;
        entry.undoHint = tr("无法自动生成逆向命令，请手动检查。");
        m_undoStack.append(entry);
    }
}

void OperationUndo::recordBefore(const OperationPlan& plan) {
    for (const auto& op : plan.operations) recordBefore(op);
}

QVector<CommandResult> OperationUndo::undoLastPlan() {
    QVector<CommandResult> results;

    if (m_undoStack.isEmpty()) {
        CommandResult r;
        r.success = false;
        r.errorMessage = tr("没有可撤销的操作");
        results.append(r);
        return results;
    }

    // 临时创建执行器来执行逆向命令
    CommandExecutor executor;

    // 反向遍历撤销
    for (int i = m_undoStack.size() - 1; i >= 0; --i) {
        const auto& entry = m_undoStack[i];
        CommandResult r;

        if (entry.strategy == UndoEntry::AutoReverse) {
            ShellOperation op;
            op.workingDir = QStringLiteral("~");
            op.description = tr("撤销: %1").arg(entry.description);
            op.timeoutSecs = 30;

            // Check for native reverse markers
            if (entry.reverseCommand.startsWith(QStringLiteral("__native_move:"))) {
                // Format: __native_move:src:dest
                QString payload = entry.reverseCommand.mid(14);  // after "__native_move:"
                int colon = payload.indexOf(QLatin1Char(':'));
                if (colon > 0) {
                    op.type = ShellOperation::MoveFile;
                    op.source = payload.left(colon);
                    op.target = payload.mid(colon + 1);
                } else {
                    op.type = ShellOperation::ShellCommand;
                    op.command = entry.reverseCommand;
                }
            } else if (entry.reverseCommand.startsWith(QStringLiteral("__native_delete:"))) {
                // Format: __native_delete:target
                op.type = ShellOperation::DeleteFile;
                op.source = entry.reverseCommand.mid(16);
            } else {
                op.type = ShellOperation::ShellCommand;
                op.command = entry.reverseCommand;
            }

            r = executor.execute(op);
        } else if (entry.strategy == UndoEntry::ScriptReverse) {
            r.success = false;
            r.errorMessage = tr("需手动撤销: %1 — %2").arg(entry.description, entry.undoHint);
        } else {
            r.success = false;
            r.errorMessage = tr("此操作不可撤销: %1").arg(entry.description);
        }

        results.prepend(r);
    }

    m_undoStack.clear();
    saveLog();
    return results;
}

bool OperationUndo::canUndo() const {
    return !m_undoStack.isEmpty();
}

QString OperationUndo::logFilePath() {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                      QStringLiteral("/tasks");
    return dataDir + QStringLiteral("/operation_log.json");
}

void OperationUndo::saveLog() {
    QJsonArray arr;
    for (const auto& e : m_undoStack) {
        QJsonObject obj;
        obj[QStringLiteral("strategy")] = static_cast<int>(e.strategy);
        obj[QStringLiteral("reverseCommand")] = e.reverseCommand;
        obj[QStringLiteral("undoHint")] = e.undoHint;
        obj[QStringLiteral("description")] = e.description;
        arr.append(obj);
    }

    QString path = logFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void OperationUndo::loadLog() {
    QFile file(logFilePath());
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) return;

    m_undoStack.clear();
    for (const auto& val : doc.array()) {
        QJsonObject obj = val.toObject();
        UndoEntry entry;
        entry.strategy = static_cast<UndoEntry::Strategy>(obj[QStringLiteral("strategy")].toInt());
        entry.reverseCommand = obj[QStringLiteral("reverseCommand")].toString();
        entry.undoHint = obj[QStringLiteral("undoHint")].toString();
        entry.description = obj[QStringLiteral("description")].toString();
        m_undoStack.append(entry);
    }
}
