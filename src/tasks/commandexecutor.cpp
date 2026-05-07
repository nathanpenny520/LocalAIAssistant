#include "commandexecutor.h"
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QEventLoop>

CommandExecutor::CommandExecutor(QObject *parent)
    : QObject(parent)
{
}

CommandExecutor::~CommandExecutor()
{
    cancel();
}

QString CommandExecutor::buildShell() const
{
#ifdef Q_OS_WIN
    return QStringLiteral("cmd.exe");
#else
    return QStringLiteral("/bin/zsh");
#endif
}

QString CommandExecutor::expandPath(const QString &path)
{
    if (path.isEmpty())
        return path;

    QString expanded = path;
    if (expanded.startsWith(QLatin1String("~/"))) {
        expanded.replace(0, 1, QDir::homePath());
    } else if (expanded.startsWith(QLatin1String("~\\"))) {
        expanded.replace(0, 1, QDir::homePath());
    } else if (expanded == QLatin1String("~")) {
        expanded = QDir::homePath();
    }
    return QDir::cleanPath(expanded);
}

CommandResult CommandExecutor::execute(const ShellOperation &op)
{
    CommandResult result;

    // WriteFile 和 SearchFiles 作为便捷封装，内部转为 shell 命令
    if (op.type == ShellOperation::WriteFile) {
        // 将 WriteFile 转为 cat << 'EOF' > file 形式的 shell 命令
        ShellOperation cmd;
        cmd.type = ShellOperation::ShellCommand;
        cmd.command = op.command;
        cmd.workingDir = op.workingDir;
        cmd.timeoutSecs = op.timeoutSecs;
        result = runCommand(op.command, expandPath(op.workingDir),
                            op.timeoutSecs, QStringList());
        return result;
    }

    if (op.type == ShellOperation::SearchFiles) {
        result = runCommand(op.command, expandPath(op.workingDir),
                            op.timeoutSecs, QStringList());
        return result;
    }

    result = runCommand(op.command, expandPath(op.workingDir),
                        op.timeoutSecs, QStringList());
    return result;
}

QVector<CommandResult> CommandExecutor::executePlan(const OperationPlan &plan)
{
    QVector<CommandResult> results;
    results.reserve(plan.operations.size());
    m_cancelled = false;

    for (int i = 0; i < plan.operations.size(); ++i) {
        if (m_cancelled) {
            CommandResult r;
            r.success = false;
            r.errorMessage = tr("执行已取消");
            results.append(r);
            break;
        }

        m_currentOpIndex = i;
        const auto &op = plan.operations[i];
        emit operationStarted(i, op.command);

        CommandResult result = execute(op);
        results.append(result);

        emit operationFinished(i, result);

        if (!result.success) {
            // 遇错即停，后续操作不再执行
            break;
        }
    }

    return results;
}

void CommandExecutor::cancel()
{
    m_cancelled = true;
    if (m_currentProcess && m_currentProcess->state() != QProcess::NotRunning) {
        m_currentProcess->kill();
        m_currentProcess->waitForFinished(3000);
        m_currentProcess->terminate();
    }
}

CommandResult CommandExecutor::runCommand(const QString &command,
                                           const QString &workingDir,
                                           int timeoutSecs,
                                           const QStringList &extraEnv)
{
    CommandResult result;
    QElapsedTimer timer;
    timer.start();

    if (command.trimmed().isEmpty()) {
        result.success = false;
        result.errorMessage = tr("命令为空");
        return result;
    }

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);

    // 设置工作目录
    if (!workingDir.isEmpty()) {
        QDir dir(workingDir);
        if (dir.exists())
            process.setWorkingDirectory(workingDir);
        else
            process.setWorkingDirectory(QDir::homePath());
    }

    // 设置环境变量（继承当前环境，追加自定义变量）
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const auto &kv : extraEnv) {
        int eq = kv.indexOf(QLatin1Char('='));
        if (eq > 0)
            env.insert(kv.left(eq), kv.mid(eq + 1));
    }
    process.setProcessEnvironment(env);

#ifdef Q_OS_WIN
    process.start(QStringLiteral("cmd.exe"),
                  QStringList() << QStringLiteral("/c") << command);
#else
    process.start(QStringLiteral("/bin/zsh"),
                  QStringList() << QStringLiteral("-c") << command);
#endif

    if (!process.waitForStarted(5000)) {
        result.success = false;
        result.errorMessage = tr("无法启动进程: ") + process.errorString();
        result.elapsedMs = timer.elapsed();
        return result;
    }

    // 超时处理
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);

    QEventLoop loop;
    QObject::connect(&process,
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     &loop, &QEventLoop::quit);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, [&]() {
        process.kill();
        process.waitForFinished(2000);
        if (process.state() != QProcess::NotRunning)
            process.terminate();
    });
    QObject::connect(&process, &QProcess::readyReadStandardOutput,
                     &loop, [&]() {
        while (process.canReadLine()) {
            QByteArray line = process.readLine();
            QString text = QString::fromUtf8(line).trimmed();
            if (!text.isEmpty())
                emit stdoutLineReceived(text, m_currentOpIndex);
        }
    });
    QObject::connect(&process, &QProcess::readyReadStandardError,
                     &loop, [&]() {
        while (process.canReadLine()) {
            QByteArray line = process.readLine();
            QString text = QString::fromUtf8(line).trimmed();
            if (!text.isEmpty())
                emit stderrLineReceived(text, m_currentOpIndex);
        }
    });

    timeoutTimer.start(timeoutSecs * 1000);
    m_currentProcess = &process;
    loop.exec();
    m_currentProcess = nullptr;

    result.elapsedMs = timer.elapsed();
    result.exitCode = process.exitCode();
    result.stdoutOutput = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    result.stderrOutput = QString::fromUtf8(process.readAllStandardError()).trimmed();

    if (timeoutTimer.isActive())
        timeoutTimer.stop();

    if (process.exitStatus() == QProcess::CrashExit) {
        if (process.error() == QProcess::Timedout
            || result.elapsedMs >= timeoutSecs * 1000) {
            result.success = false;
            result.errorMessage = tr("命令执行超时 (%1 秒)").arg(timeoutSecs);
        } else {
            result.success = false;
            result.errorMessage = tr("进程异常终止: ") + process.errorString();
        }
    } else {
        result.success = (result.exitCode == 0);
        if (!result.success)
            result.errorMessage = tr("命令退出码: %1").arg(result.exitCode);
    }

    return result;
}
