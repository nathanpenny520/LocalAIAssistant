#include "commandexecutor.h"

#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

CommandExecutor::CommandExecutor(QObject* parent) : QObject(parent) {
    detectAvailableShell();
}

CommandExecutor::~CommandExecutor() {
    cancel();
}

void CommandExecutor::detectAvailableShell() {
#ifdef Q_OS_WIN
    // Try PowerShell Core first, then Windows PowerShell, fall back to cmd
    QString pwsh = QStandardPaths::findExecutable(QStringLiteral("pwsh.exe"));
    if (!pwsh.isEmpty()) {
        m_shellPath = pwsh;
        m_shellArgs = QStringList() << QStringLiteral("-NoProfile") << QStringLiteral("-Command");
        return;
    }
    QString powershell = QStandardPaths::findExecutable(QStringLiteral("powershell.exe"));
    if (!powershell.isEmpty()) {
        m_shellPath = powershell;
        m_shellArgs = QStringList() << QStringLiteral("-NoProfile") << QStringLiteral("-Command");
        return;
    }
    m_shellPath = QStringLiteral("cmd.exe");
    m_shellArgs = QStringList() << QStringLiteral("/c");
#else
    // Use $SHELL if set, otherwise fall back to zsh → bash → /bin/sh
    QString shellEnv = qEnvironmentVariable("SHELL");
    if (!shellEnv.isEmpty() && QFile::exists(shellEnv)) {
        m_shellPath = shellEnv;
        m_shellArgs = QStringList() << QStringLiteral("-c");
        return;
    }
    QString zsh = QStandardPaths::findExecutable(QStringLiteral("zsh"));
    if (!zsh.isEmpty()) {
        m_shellPath = zsh;
        m_shellArgs = QStringList() << QStringLiteral("-c");
        return;
    }
    QString bash = QStandardPaths::findExecutable(QStringLiteral("bash"));
    if (!bash.isEmpty()) {
        m_shellPath = bash;
        m_shellArgs = QStringList() << QStringLiteral("-c");
        return;
    }
    m_shellPath = QStringLiteral("/bin/sh");
    m_shellArgs = QStringList() << QStringLiteral("-c");
#endif
}

QString CommandExecutor::shellName() const {
    QFileInfo fi(m_shellPath);
    return fi.baseName();  // "zsh", "bash", "pwsh", "powershell", "cmd"
}

QString CommandExecutor::expandPath(const QString& path) {
    if (path.isEmpty()) return path;

    QString expanded = path;

    // Expand ~/ and ~\ to home directory
    if (expanded.startsWith(QLatin1String("~/"))) {
        expanded.replace(0, 1, QDir::homePath());
    } else if (expanded.startsWith(QLatin1String("~\\"))) {
        expanded.replace(0, 1, QDir::homePath());
    } else if (expanded == QLatin1String("~")) {
        expanded = QDir::homePath();
    }

    // Expand Windows-style %VAR% environment variables (e.g. %APPDATA%, %USERPROFILE%)
    static QRegularExpression winEnvVar(QStringLiteral("%([A-Za-z_][A-Za-z0-9_]*)%"));
    QRegularExpressionMatch m;
    while ((m = winEnvVar.match(expanded)).hasMatch()) {
        QString varName = m.captured(1);
        QString varValue = QProcessEnvironment::systemEnvironment().value(varName);
        if (varValue.isEmpty()) break;  // unknown variable, stop to avoid infinite loop
        expanded.replace(m.capturedStart(), m.capturedLength(), varValue);
    }

    // Expand Unix-style $VAR and ${VAR} environment variables
    static QRegularExpression unixEnvVar(QStringLiteral("\\$\\{([A-Za-z_][A-Za-z0-9_]*)\\}"));
    while ((m = unixEnvVar.match(expanded)).hasMatch()) {
        QString varName = m.captured(1);
        QString varValue = QProcessEnvironment::systemEnvironment().value(varName);
        if (varValue.isEmpty()) break;
        expanded.replace(m.capturedStart(), m.capturedLength(), varValue);
    }
    static QRegularExpression unixEnvVarShort(QStringLiteral("\\$([A-Za-z_][A-Za-z0-9_]*)"));
    while ((m = unixEnvVarShort.match(expanded)).hasMatch()) {
        QString varName = m.captured(1);
        QString varValue = QProcessEnvironment::systemEnvironment().value(varName);
        if (varValue.isEmpty()) break;
        expanded.replace(m.capturedStart(), m.capturedLength(), varValue);
    }

    return QDir::cleanPath(expanded);
}

// ── Native file operations ──────────────────────────────────────

CommandResult CommandExecutor::executeCreateDir(const ShellOperation& op) {
    CommandResult result;
    QElapsedTimer timer;
    timer.start();

    QString targetPath = expandPath(op.target);
    if (targetPath.isEmpty()) {
        result.success = false;
        result.errorMessage = tr("创建目录: 目标路径为空");
        result.elapsedMs = timer.elapsed();
        return result;
    }

    QDir dir;
    if (dir.mkpath(targetPath)) {
        result.success = true;
        result.exitCode = 0;
        result.stdoutOutput = tr("已创建目录: %1").arg(targetPath);
    } else {
        result.success = false;
        result.exitCode = 1;
        result.errorMessage = tr("创建目录失败: %1").arg(targetPath);
    }
    result.elapsedMs = timer.elapsed();
    return result;
}

CommandResult CommandExecutor::executeMoveFile(const ShellOperation& op) {
    CommandResult result;
    QElapsedTimer timer;
    timer.start();

    QString src = expandPath(op.source);
    QString dst = expandPath(op.target);
    if (src.isEmpty() || dst.isEmpty()) {
        result.success = false;
        result.errorMessage = tr("移动文件: 源或目标路径为空");
        result.elapsedMs = timer.elapsed();
        return result;
    }

    if (QFile::rename(src, dst)) {
        result.success = true;
        result.exitCode = 0;
        result.stdoutOutput = tr("已移动: %1 → %2").arg(src, dst);
        result.elapsedMs = timer.elapsed();
        return result;
    }

    // Cross-device rename fails — try copy + delete
    QFileInfo srcInfo(src);
    if (srcInfo.isDir()) {
        // Recursive copy then remove source
        CommandResult copyResult = executeCopyFile(op);
        if (!copyResult.success) {
            result.success = false;
            result.exitCode = 1;
            result.errorMessage = tr("跨设备移动失败 (复制阶段): %1").arg(copyResult.errorMessage);
            result.elapsedMs = timer.elapsed();
            return result;
        }
        QDir srcDir(src);
        if (srcDir.removeRecursively()) {
            result.success = true;
            result.exitCode = 0;
            result.stdoutOutput = tr("已跨设备移动: %1 → %2").arg(src, dst);
        } else {
            result.success = true;  // copy succeeded, cleanup failed but not critical
            result.exitCode = 0;
            result.stdoutOutput = tr("已复制到: %2 (源目录清理失败)").arg(src, dst);
        }
    } else {
        if (QFile::copy(src, dst)) {
            QFile::remove(src);
            result.success = true;
            result.exitCode = 0;
            result.stdoutOutput = tr("已跨设备移动: %1 → %2").arg(src, dst);
        } else {
            result.success = false;
            result.exitCode = 1;
            result.errorMessage = tr("移动文件失败: %1 → %2").arg(src, dst);
        }
    }
    result.elapsedMs = timer.elapsed();
    return result;
}

CommandResult CommandExecutor::executeDeleteFile(const ShellOperation& op) {
    CommandResult result;
    QElapsedTimer timer;
    timer.start();

    QString src = expandPath(op.source);
    if (src.isEmpty()) {
        result.success = false;
        result.errorMessage = tr("删除: 路径为空");
        result.elapsedMs = timer.elapsed();
        return result;
    }

    QFileInfo info(src);
    if (!info.exists()) {
        result.success = false;
        result.exitCode = 1;
        result.errorMessage = tr("路径不存在: %1").arg(src);
        result.elapsedMs = timer.elapsed();
        return result;
    }

    bool ok = false;
    QString errorDetail;
    if (info.isDir()) {
        ok = QDir(src).removeRecursively();
        if (!ok) errorDetail = tr("(directory may be non-empty or permission denied)");
    } else {
        QFile file(src);
        ok = file.remove();
        if (!ok) errorDetail = file.errorString();
    }

    if (ok) {
        result.success = true;
        result.exitCode = 0;
        result.stdoutOutput = tr("已删除: %1").arg(src);
    } else {
        result.success = false;
        result.exitCode = 1;
        result.errorMessage = tr("删除失败: %1 — %2").arg(src, errorDetail);
    }
    result.elapsedMs = timer.elapsed();
    return result;
}

CommandResult CommandExecutor::executeCopyFile(const ShellOperation& op) {
    CommandResult result;
    QElapsedTimer timer;
    timer.start();

    QString src = expandPath(op.source);
    QString dst = expandPath(op.target);
    if (src.isEmpty() || dst.isEmpty()) {
        result.success = false;
        result.errorMessage = tr("复制文件: 源或目标路径为空");
        result.elapsedMs = timer.elapsed();
        return result;
    }

    QFileInfo srcInfo(src);
    if (!srcInfo.exists()) {
        result.success = false;
        result.exitCode = 1;
        result.errorMessage = tr("源路径不存在: %1").arg(src);
        result.elapsedMs = timer.elapsed();
        return result;
    }

    if (srcInfo.isDir()) {
        // Recursive directory copy using QDirIterator
        QDir srcDir(src);
        int fileCount = 0;
        QDir().mkpath(dst);  // ensure target root exists

        QDirIterator it(src, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            QString relativePath = srcDir.relativeFilePath(it.filePath());
            QString dstPath = dst + QStringLiteral("/") + relativePath;

            if (it.fileInfo().isDir()) {
                QDir().mkpath(dstPath);
            } else {
                QDir().mkpath(QFileInfo(dstPath).absolutePath());
                if (QFile::copy(it.filePath(), dstPath))
                    fileCount++;
                else {
                    result.success = false;
                    result.exitCode = 1;
                    result.errorMessage = tr("复制失败: %1 → %2").arg(it.filePath(), dstPath);
                    result.elapsedMs = timer.elapsed();
                    return result;
                }
            }
        }
        result.success = true;
        result.exitCode = 0;
        result.stdoutOutput = tr("已复制目录 (%1 个文件): %2 → %3").arg(fileCount).arg(src, dst);
    } else {
        QDir().mkpath(QFileInfo(dst).absolutePath());
        if (QFile::copy(src, dst)) {
            result.success = true;
            result.exitCode = 0;
            result.stdoutOutput = tr("已复制: %1 → %2").arg(src, dst);
        } else {
            result.success = false;
            result.exitCode = 1;
            result.errorMessage = tr("复制文件失败: %1 → %2").arg(src, dst);
        }
    }
    result.elapsedMs = timer.elapsed();
    return result;
}

CommandResult CommandExecutor::executeWriteFile(const ShellOperation& op) {
    CommandResult result;
    QElapsedTimer timer;
    timer.start();

    QString targetPath = expandPath(op.target);
    if (targetPath.isEmpty()) {
        result.success = false;
        result.errorMessage = tr("写入文件: 目标路径为空");
        result.elapsedMs = timer.elapsed();
        return result;
    }

    QDir().mkpath(QFileInfo(targetPath).absolutePath());

    QFile file(targetPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QByteArray content = op.command.toUtf8();
        qint64 written = file.write(content);
        file.close();
        if (written == content.size()) {
            result.success = true;
            result.exitCode = 0;
            result.stdoutOutput = tr("已写入文件: %1 (%2 字节)").arg(targetPath).arg(written);
        } else {
            result.success = false;
            result.exitCode = 1;
            result.errorMessage = tr("写入不完整: %1").arg(targetPath);
        }
    } else {
        result.success = false;
        result.exitCode = 1;
        result.errorMessage = tr("无法打开文件: %1 — %2").arg(targetPath, file.errorString());
    }
    result.elapsedMs = timer.elapsed();
    return result;
}

CommandResult CommandExecutor::executeSearchFiles(const ShellOperation& op) {
    CommandResult result;
    QElapsedTimer timer;
    timer.start();

    QString searchDir = expandPath(op.source);
    QString pattern = op.command.trimmed();
    if (searchDir.isEmpty()) {
        searchDir = QDir::homePath();
    }
    if (pattern.isEmpty()) {
        pattern = QStringLiteral("*");
    }

    QDir dir(searchDir);
    if (!dir.exists()) {
        result.success = false;
        result.exitCode = 1;
        result.errorMessage = tr("搜索目录不存在: %1").arg(searchDir);
        result.elapsedMs = timer.elapsed();
        return result;
    }

    QStringList results;
    QDirIterator it(searchDir, QStringList() << pattern,
                    QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        results.append(dir.relativeFilePath(it.filePath()));
    }

    result.success = true;
    result.exitCode = 0;
    result.stdoutOutput = results.join(QStringLiteral("\n"));
    result.elapsedMs = timer.elapsed();
    return result;
}

// ── Main execution dispatch ─────────────────────────────────────

CommandResult CommandExecutor::execute(const ShellOperation& op) {
    switch (op.type) {
        case ShellOperation::CreateDir:
            return executeCreateDir(op);
        case ShellOperation::MoveFile:
            return executeMoveFile(op);
        case ShellOperation::DeleteFile:
            return executeDeleteFile(op);
        case ShellOperation::CopyFile:
            return executeCopyFile(op);
        case ShellOperation::WriteFile:
            return executeWriteFile(op);
        case ShellOperation::SearchFiles:
            return executeSearchFiles(op);
        case ShellOperation::ShellCommand:
        case ShellOperation::ShellScript:
            return runCommand(op.command, expandPath(op.workingDir), op.timeoutSecs, QStringList());
    }
    return runCommand(op.command, expandPath(op.workingDir), op.timeoutSecs, QStringList());
}

// ── Batch execution ─────────────────────────────────────────────

QVector<CommandResult> CommandExecutor::executePlan(const OperationPlan& plan) {
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
        const auto& op = plan.operations[i];

        // Display description for native ops, command for shell ops
        QString displayText = op.description.isEmpty() ? op.command : op.description;
        emit operationStarted(i, displayText);

        CommandResult result = execute(op);
        results.append(result);

        emit operationFinished(i, result);

        if (!result.success) {
            break;
        }
    }

    return results;
}

void CommandExecutor::cancel() {
    m_cancelled = true;
    if (m_currentProcess && m_currentProcess->state() != QProcess::NotRunning) {
        m_currentProcess->kill();
        m_currentProcess->waitForFinished(3000);
        m_currentProcess->terminate();
    }
}

// ── Shell command execution ─────────────────────────────────────

CommandResult CommandExecutor::runCommand(const QString& command, const QString& workingDir,
                                          int timeoutSecs, const QStringList& extraEnv) {
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

    if (!workingDir.isEmpty()) {
        QDir dir(workingDir);
        if (dir.exists())
            process.setWorkingDirectory(workingDir);
        else
            process.setWorkingDirectory(QDir::homePath());
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const auto& kv : extraEnv) {
        int eq = kv.indexOf(QLatin1Char('='));
        if (eq > 0) env.insert(kv.left(eq), kv.mid(eq + 1));
    }
    process.setProcessEnvironment(env);

    QStringList args = m_shellArgs;
    args << command;
    process.start(m_shellPath, args);

    if (!process.waitForStarted(5000)) {
        result.success = false;
        result.errorMessage = tr("无法启动进程: ") + process.errorString();
        result.elapsedMs = timer.elapsed();
        return result;
    }

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);

    QEventLoop loop;
    QObject::connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop,
                     &QEventLoop::quit);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, [&]() {
        process.kill();
        process.waitForFinished(2000);
        if (process.state() != QProcess::NotRunning) process.terminate();
    });
    QObject::connect(&process, &QProcess::readyReadStandardOutput, &loop, [&]() {
        while (process.canReadLine()) {
            QByteArray line = process.readLine();
            QString text = QString::fromUtf8(line).trimmed();
            if (!text.isEmpty()) emit stdoutLineReceived(text, m_currentOpIndex);
        }
    });
    QObject::connect(&process, &QProcess::readyReadStandardError, &loop, [&]() {
        while (process.canReadLine()) {
            QByteArray line = process.readLine();
            QString text = QString::fromUtf8(line).trimmed();
            if (!text.isEmpty()) emit stderrLineReceived(text, m_currentOpIndex);
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

    if (timeoutTimer.isActive()) timeoutTimer.stop();

    if (process.exitStatus() == QProcess::CrashExit) {
        if (process.error() == QProcess::Timedout || result.elapsedMs >= timeoutSecs * 1000) {
            result.success = false;
            result.errorMessage = tr("命令执行超时 (%1 秒)").arg(timeoutSecs);
        } else {
            result.success = false;
            result.errorMessage = tr("进程异常终止: ") + process.errorString();
        }
    } else {
        result.success = (result.exitCode == 0);
        if (!result.success) result.errorMessage = tr("命令退出码: %1").arg(result.exitCode);
    }

    return result;
}
