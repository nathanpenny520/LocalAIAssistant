#include "taskengine.h"
#include "../prompts/promptmanager.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSettings>

TaskEngine *TaskEngine::s_instance = nullptr;

TaskEngine *TaskEngine::instance()
{
    if (!s_instance)
        s_instance = new TaskEngine();
    return s_instance;
}

TaskEngine::TaskEngine(QObject *parent)
    : QObject(parent)
    , m_executor(this)
{
}

bool TaskEngine::isTaskRequest(const QString &message) const
{
    static const QStringList taskKeywords = {
        // 中文关键词
        QStringLiteral("整理"), QStringLiteral("移动"), QStringLiteral("重命名"),
        QStringLiteral("删除文件"), QStringLiteral("复制"), QStringLiteral("搜索文件"),
        QStringLiteral("创建文件夹"), QStringLiteral("新建文件夹"), QStringLiteral("打包"),
        QStringLiteral("批量"), QStringLiteral("清理"), QStringLiteral("归类"),
        QStringLiteral("整理文件"), QStringLiteral("移动文件"), QStringLiteral("按类型"),
        QStringLiteral("按日期"), QStringLiteral("修改文件"), QStringLiteral("替换"),
        QStringLiteral("安装"), QStringLiteral("下载"), QStringLiteral("编译"),
        QStringLiteral("运行"), QStringLiteral("执行"), QStringLiteral("终端"),
        QStringLiteral("命令"), QStringLiteral("脚本"),
        // 英文关键词
        QStringLiteral("organize"), QStringLiteral("move"), QStringLiteral("rename"),
        QStringLiteral("delete"), QStringLiteral("sort"), QStringLiteral("clean up"),
        QStringLiteral("batch"), QStringLiteral("classify"),
        QStringLiteral("install"), QStringLiteral("download"), QStringLiteral("compile"),
        QStringLiteral("run"), QStringLiteral("execute"), QStringLiteral("terminal"),
        QStringLiteral("command"), QStringLiteral("script"),
        QStringLiteral("mkdir"), QStringLiteral("git"), QStringLiteral("npm"),
        QStringLiteral("pip"), QStringLiteral("brew"), QStringLiteral("apt")
    };

    QString lower = message.toLower();
    for (const auto &kw : taskKeywords) {
        if (lower.contains(kw))
            return true;
    }

    return false;
}

OperationPlan TaskEngine::parsePlanFromAIResponse(const QString &aiResponse) const
{
    static QRegularExpression re(
        QStringLiteral(R"(\[TASK_PLAN\]\s*(\{.*?\})\s*\[/TASK_PLAN\])"),
        QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatch match = re.match(aiResponse);
    if (match.hasMatch()) {
        QString jsonStr = match.captured(1);
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (doc.isObject())
            return parsePlanFromJson(doc.object());
    }

    // 如果没有标签，尝试直接解析整个回复为 JSON
    QJsonDocument doc = QJsonDocument::fromJson(aiResponse.toUtf8());
    if (doc.isObject())
        return parsePlanFromJson(doc.object());

    return {};
}

OperationPlan TaskEngine::parsePlanFromJson(const QJsonObject &json) const
{
    OperationPlan plan;
    plan.description = json[QStringLiteral("description")].toString();
    plan.requiresConfirmation = json[QStringLiteral("requiresConfirmation")].toBool(true);

    QJsonArray opsArray = json[QStringLiteral("operations")].toArray();
    for (const auto &val : opsArray) {
        QJsonObject opObj = val.toObject();
        ShellOperation op;

        QString typeStr = opObj[QStringLiteral("type")].toString().toLower();
        if (typeStr == QStringLiteral("shell_command") || typeStr == QStringLiteral("shellcommand"))
            op.type = ShellOperation::ShellCommand;
        else if (typeStr == QStringLiteral("shell_script") || typeStr == QStringLiteral("shellscript"))
            op.type = ShellOperation::ShellScript;
        else if (typeStr == QStringLiteral("write_file") || typeStr == QStringLiteral("writefile"))
            op.type = ShellOperation::WriteFile;
        else if (typeStr == QStringLiteral("search_files") || typeStr == QStringLiteral("searchfiles"))
            op.type = ShellOperation::SearchFiles;
        else if (typeStr == QStringLiteral("create_dir") || typeStr == QStringLiteral("createdir"))
            op.type = ShellOperation::CreateDir;
        else if (typeStr == QStringLiteral("move_file") || typeStr == QStringLiteral("movefile"))
            op.type = ShellOperation::MoveFile;
        else if (typeStr == QStringLiteral("delete_file") || typeStr == QStringLiteral("deletefile"))
            op.type = ShellOperation::DeleteFile;
        else if (typeStr == QStringLiteral("copy_file") || typeStr == QStringLiteral("copyfile"))
            op.type = ShellOperation::CopyFile;
        // 向后兼容旧类型名（仅单字类型名）
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
        else
            continue;

        op.command = opObj[QStringLiteral("command")].toString();
        op.source = opObj[QStringLiteral("source")].toString();
        op.target = opObj[QStringLiteral("target")].toString();
        op.workingDir = opObj[QStringLiteral("workingDir")].toString();
        op.description = opObj[QStringLiteral("description")].toString();
        op.timeoutSecs = opObj[QStringLiteral("timeout")].toInt(30);

        // 向后兼容旧字段名: content → command 映射
        if (op.command.isEmpty()) {
            QString content = opObj[QStringLiteral("content")].toString();
            if (!content.isEmpty()) {
                op.command = content;
            }
        }

        // 向后兼容: search 旧格式 target 字段是搜索模式 → 映射到 command
        if (op.type == ShellOperation::SearchFiles && op.command.isEmpty() && !op.target.isEmpty()) {
            op.command = op.target;
            op.target.clear();
        }

        // Native file ops use source/target; shell ops use command
        if (!op.command.isEmpty() || !op.source.isEmpty() || !op.target.isEmpty())
            plan.operations.append(op);
    }

    return plan;
}

QString TaskEngine::taskPromptTemplate() const
{
    return QStringLiteral("\n\n") + PromptManager::instance()->taskPrompt();
}

SafetyChecker::Result TaskEngine::validatePlan(const OperationPlan &plan)
{
    return m_safetyChecker.validatePlan(plan);
}

QVector<CommandResult> TaskEngine::executePlan(const OperationPlan &plan)
{
    m_undo.recordBefore(plan);

    QVector<CommandResult> results = m_executor.executePlan(plan);
    emit executionFinished(results);
    return results;
}

bool TaskEngine::canUndo() const
{
    return m_undo.canUndo();
}

QVector<CommandResult> TaskEngine::undoLast()
{
    QVector<CommandResult> results = m_undo.undoLastPlan();
    emit undoFinished(results);
    return results;
}

CommandExecutor *TaskEngine::executor()
{
    return &m_executor;
}

SafetyChecker &TaskEngine::safetyChecker()
{
    return m_safetyChecker;
}
