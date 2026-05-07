#include "operationplan.h"
#include <QRegularExpression>

QString ShellOperation::typeName() const
{
    switch (type) {
    case ShellCommand: return tr("Shell 命令");
    case ShellScript:  return tr("Shell 脚本");
    case WriteFile:    return tr("写入文件");
    case SearchFiles:  return tr("搜索文件");
    }
    return tr("未知");
}

QString ShellOperation::dangerLabel() const
{
    // 检测命令危险级别
    const QString cmd = command.trimmed();

    // 危险操作
    if (cmd.contains(QRegularExpression("\\brm\\b.*(-r|-rf|--recursive)"))
        || cmd.contains(QRegularExpression("\\bgit\\s+push\\s+--force\\b"))
        || cmd.contains(QRegularExpression("\\bcurl\\b.*\\b(?!localhost\\b|127\\.0\\.0\\.1\\b)"))
        || cmd.contains(QRegularExpression("\\bwget\\b.*\\b(?!localhost\\b|127\\.0\\.0\\.1\\b)")))
        return tr("危险");

    // 需要注意的操作
    if (cmd.contains(QRegularExpression("\\brm\\b"))
        || cmd.contains(QRegularExpression("\\bmv\\b"))
        || cmd.contains(QRegularExpression("\\bchmod\\b"))
        || cmd.contains(QRegularExpression("\\bchown\\b"))
        || cmd.contains(QRegularExpression("\\bgit\\b")))
        return tr("注意");

    return tr("安全");
}

QString OperationPlan::generateSummary() const
{
    QStringList lines;
    lines.append(tr("命令计划：%1（共 %2 条命令）")
                     .arg(description).arg(operations.size()));

    for (int i = 0; i < operations.size(); ++i) {
        const auto &op = operations[i];
        QString line = tr("%1. [%2] %3")
                           .arg(i + 1)
                           .arg(op.dangerLabel());

        if (!op.description.isEmpty())
            line = line.arg(op.description);
        else
            line = line.arg(op.command.left(80));

        lines.append(line);
    }

    return lines.join(QStringLiteral("\n"));
}

QString OperationPlan::generateShellPreview() const
{
    QStringList lines;
    lines.append(QStringLiteral("# ") + description);

    for (int i = 0; i < operations.size(); ++i) {
        const auto &op = operations[i];
        if (!op.description.isEmpty())
            lines.append(QStringLiteral("# %1. %2").arg(i + 1).arg(op.description));

        if (!op.workingDir.isEmpty() && op.workingDir != QStringLiteral("~"))
            lines.append(QStringLiteral("cd \"%1\" && \\").arg(op.workingDir));

        switch (op.type) {
        case ShellOperation::ShellCommand:
            lines.append(op.command);
            break;
        case ShellOperation::ShellScript:
            lines.append(op.command);
            break;
        case ShellOperation::WriteFile:
            lines.append(QStringLiteral("# (写入文件操作)"));
            break;
        case ShellOperation::SearchFiles:
            lines.append(QStringLiteral("# (搜索文件操作)"));
            break;
        }
        lines.append(QString());
    }

    return lines.join(QStringLiteral("\n"));
}

bool OperationPlan::isEmpty() const
{
    return operations.isEmpty();
}

int OperationPlan::totalOperations() const
{
    return operations.size();
}
