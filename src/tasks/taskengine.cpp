#include "taskengine.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSettings>

#include "../prompts/promptmanager.h"

TaskEngine* TaskEngine::s_instance = nullptr;

const QString TaskEngine::kTagTaskPlan      = QStringLiteral("[TASK_PLAN]");
const QString TaskEngine::kTagTaskPlanClose = QStringLiteral("[/TASK_PLAN]");
const QString TaskEngine::kTagTaskComplete  = QStringLiteral("[TASK_COMPLETE]");
const QString TaskEngine::kTagTaskFinished  = QStringLiteral("[TASK_FINISHED]");

TaskEngine* TaskEngine::instance() {
    if (!s_instance) s_instance = new TaskEngine();
    return s_instance;
}

TaskEngine::TaskEngine(QObject* parent) : QObject(parent), m_executor(this) {
}

OperationPlan TaskEngine::parsePlanFromAIResponse(const QString& aiResponse) const {
    // Extract content between [TASK_PLAN] ... [/TASK_PLAN]
    QString pattern = QRegularExpression::escape(kTagTaskPlan) + QStringLiteral("\\s*(.*?)\\s*") +
                      QRegularExpression::escape(kTagTaskPlanClose);
    static QRegularExpression re(pattern, QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatch match = re.match(aiResponse);
    if (match.hasMatch()) {
        QString content = match.captured(1).trimmed();

        // Strip markdown code fences if present (```json ... ```)
        static QRegularExpression codeFence(QStringLiteral("```(?:json)?\\s*(\\{.*?\\})\\s*```"),
                                            QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatch fenceMatch = codeFence.match(content);
        if (fenceMatch.hasMatch()) {
            content = fenceMatch.captured(1);
        }

        QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
        if (doc.isObject()) return parsePlanFromJson(doc.object());
        qWarning() << "TaskEngine: [TASK_PLAN] found but JSON parse failed:"
                   << doc.isNull() << content.left(200);
    }

    // Fallback: if no TASK_PLAN tags found, try parsing the entire response as JSON
    QJsonDocument doc = QJsonDocument::fromJson(aiResponse.toUtf8());
    if (doc.isObject()) return parsePlanFromJson(doc.object());

    return {};
}

OperationPlan TaskEngine::parsePlanFromJson(const QJsonObject& json) const {
    OperationPlan plan;
    plan.description = json[QStringLiteral("description")].toString();
    plan.requiresConfirmation = json[QStringLiteral("requiresConfirmation")].toBool(true);

    QJsonArray opsArray = json[QStringLiteral("operations")].toArray();
    for (const auto& val : opsArray) {
        QJsonObject opObj = val.toObject();
        ShellOperation op;

        QString typeStr = opObj[QStringLiteral("type")].toString().toLower();
        if (typeStr == QStringLiteral("shell_command") || typeStr == QStringLiteral("shellcommand"))
            op.type = ShellOperation::ShellCommand;
        else if (typeStr == QStringLiteral("shell_script") || typeStr == QStringLiteral("shellscrip"
                                                                                        "t"))
            op.type = ShellOperation::ShellScript;
        else if (typeStr == QStringLiteral("write_file") || typeStr == QStringLiteral("writefile"))
            op.type = ShellOperation::WriteFile;
        else if (typeStr == QStringLiteral("search_files") || typeStr == QStringLiteral("searchfile"
                                                                                        "s"))
            op.type = ShellOperation::SearchFiles;
        else if (typeStr == QStringLiteral("create_dir") || typeStr == QStringLiteral("createdir"))
            op.type = ShellOperation::CreateDir;
        else if (typeStr == QStringLiteral("move_file") || typeStr == QStringLiteral("movefile"))
            op.type = ShellOperation::MoveFile;
        else if (typeStr == QStringLiteral("delete_file") || typeStr == QStringLiteral("deletefil"
                                                                                       "e"))
            op.type = ShellOperation::DeleteFile;
        else if (typeStr == QStringLiteral("copy_file") || typeStr == QStringLiteral("copyfile"))
            op.type = ShellOperation::CopyFile;
        // Backward compatibility: legacy single-word type names
        else if (typeStr == QStringLiteral("move"))
            op.type = ShellOperation::MoveFile;
        else if (typeStr == QStringLiteral("rename"))
            op.type = ShellOperation::MoveFile;
        else if (typeStr == QStringLiteral("delete"))
            op.type = ShellOperation::DeleteFile;
        else if (typeStr == QStringLiteral("copy"))
            op.type = ShellOperation::CopyFile;
        else if (typeStr == QStringLiteral("write"))
            op.type = ShellOperation::WriteFile;
        else if (typeStr == QStringLiteral("search"))
            op.type = ShellOperation::SearchFiles;
        else {
            qWarning() << "TaskEngine: unrecognized operation type" << typeStr;
            continue;
        }

        op.command = opObj[QStringLiteral("command")].toString();
        op.source = opObj[QStringLiteral("source")].toString();
        op.target = opObj[QStringLiteral("target")].toString();
        op.workingDir = opObj[QStringLiteral("workingDir")].toString();
        op.description = opObj[QStringLiteral("description")].toString();
        op.timeoutSecs = opObj[QStringLiteral("timeout")].toInt(30);

        // Backward compatibility: map legacy 'content' field to 'command'
        if (op.command.isEmpty()) {
            QString content = opObj[QStringLiteral("content")].toString();
            if (!content.isEmpty()) {
                op.command = content;
            }
        }

        // Backward compatibility: legacy search format where target held the pattern → map to command
        if (op.type == ShellOperation::SearchFiles && op.command.isEmpty() &&
            !op.target.isEmpty()) {
            op.command = op.target;
            op.target.clear();
        }

        // Native file ops use source/target; shell ops use command
        if (!op.command.isEmpty() || !op.source.isEmpty() || !op.target.isEmpty())
            plan.operations.append(op);
    }

    return plan;
}

QString TaskEngine::taskPromptTemplate() const {
    return QStringLiteral("\n\n") + PromptManager::instance()->taskPrompt();
}

SafetyChecker::Result TaskEngine::validatePlan(const OperationPlan& plan) {
    return m_safetyChecker.validatePlan(plan);
}

QVector<CommandResult> TaskEngine::executePlan(const OperationPlan& plan) {
    m_undo.recordBefore(plan);

    QVector<CommandResult> results = m_executor.executePlan(plan);
    emit executionFinished(results);
    return results;
}

bool TaskEngine::canUndo() const {
    return m_undo.canUndo();
}

QVector<CommandResult> TaskEngine::undoLast() {
    QVector<CommandResult> results = m_undo.undoLastPlan();
    emit undoFinished(results);
    return results;
}

CommandExecutor* TaskEngine::executor() {
    return &m_executor;
}

SafetyChecker& TaskEngine::safetyChecker() {
    return m_safetyChecker;
}
