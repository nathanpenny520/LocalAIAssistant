#include "safetychecker.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

SafetyChecker::SafetyChecker() {
    resetToDefaults();
}

void SafetyChecker::setAllowedPaths(const QStringList& paths) {
    m_allowedPaths = paths;
}

void SafetyChecker::addAllowedPath(const QString& path) {
    if (!m_allowedPaths.contains(path)) m_allowedPaths.append(path);
}

void SafetyChecker::resetToDefaults() {
    m_allowedPaths.clear();
    m_allowedPaths.append(QDir::homePath());
    m_allowedPaths.append(QDir::tempPath());
    m_allowedPaths.append(QStringLiteral("/tmp"));
    m_allowedPaths.append(QDir::currentPath());
    m_allowedPaths.append(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    m_allowedPaths.append(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    m_allowedPaths.append(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    m_allowedPaths.append(QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
    m_allowedPaths.append(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation));
    m_allowedPaths.append(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
}

QStringList SafetyChecker::allowedPaths() const {
    return m_allowedPaths;
}

QString SafetyChecker::lastBlockReason() const {
    return m_lastBlockReason;
}

SafetyChecker::DangerLevel SafetyChecker::dangerLevel(const ShellOperation& op) const {
    const QString cmd = op.command.trimmed();

    // Native file ops bypass command-based danger level
    if (op.type == ShellOperation::CreateDir || op.type == ShellOperation::MoveFile ||
        op.type == ShellOperation::CopyFile || op.type == ShellOperation::WriteFile ||
        op.type == ShellOperation::SearchFiles) {
        return Safe;
    }
    if (op.type == ShellOperation::DeleteFile) return Caution;

    if (cmd.isEmpty()) return Safe;

    // ── Dangerous commands (Unix) ──
    if (cmd.contains(QRegularExpression("\\bsudo\\b")) ||
        cmd.contains(QRegularExpression("\\bsu\\b\\s+-")) ||
        cmd.contains(QRegularExpression("\\bdoas\\b")) ||
        cmd.contains(QRegularExpression("\\bpkexec\\b"))) {
        return Dangerous;
    }

    if (cmd.contains(QRegularExpression("\\brm\\s+.*(-r|-rf|--recursive)\\b.*/")) ||
        cmd.contains(QRegularExpression("\\bmkfs\\.")) ||
        cmd.contains(QRegularExpression("\\bdd\\s+if=")) ||
        cmd.contains(QRegularExpression("\\bfdisk\\b"))) {
        return Dangerous;
    }

    if (cmd.contains(QRegularExpression(">\\s*/dev/(sd|disk|nvme)")) ||
        cmd.contains(QRegularExpression("\\bchmod\\s+777\\s+/")) ||
        cmd.contains(QRegularExpression("\\bchown\\s+-R\\s+/"))) {
        return Dangerous;
    }

    // ── Dangerous commands (Windows) ──
    if (cmd.contains(QRegularExpression("\\brunas\\b|\\bpsexec\\b|-Verb\\s+RunAs",
                                        QRegularExpression::CaseInsensitiveOption))) {
        return Dangerous;
    }

    if (cmd.contains(QRegularExpression("\\bformat\\s+[A-Za-z]:|\\bdiskpart\\b|>\\s*\\\\\\\\.\\\\",
                                        QRegularExpression::CaseInsensitiveOption))) {
        return Dangerous;
    }

    if (cmd.contains(QRegularExpression("\\breg\\s+delete\\s+(HKLM|HKEY_LOCAL_MACHINE)",
                                        QRegularExpression::CaseInsensitiveOption))) {
        return Dangerous;
    }

    if (cmd.contains(QRegularExpression("\\bsc\\s+(delete|stop)\\b",
                                        QRegularExpression::CaseInsensitiveOption))) {
        return Dangerous;
    }

    if (cmd.contains(QRegularExpression("\\btaskkill\\s+/f\\s+/im\\s+(lsass|winlogon|csrss)",
                                        QRegularExpression::CaseInsensitiveOption))) {
        return Dangerous;
    }

    if (cmd.contains(QRegularExpression("\\bshutdown\\s+/(s|r)\\s+/t\\s+0|\\bbcdedit\\s+/"
                                        "delete|\\bvssadmin\\s+delete",
                                        QRegularExpression::CaseInsensitiveOption))) {
        return Dangerous;
    }

    // ── Operations requiring confirmation ──
    if (cmd.contains(QRegularExpression("\\brm\\b")) ||
        cmd.contains(QRegularExpression("\\bgit\\s+push\\s+.*--force")) ||
        cmd.contains(QRegularExpression("\\bcurl\\b")) ||
        cmd.contains(QRegularExpression("\\bwget\\b"))) {
        return Caution;
    }

    // Windows caution patterns
    if (cmd.contains(QRegularExpression("\\bdel\\s+/f|Remove-Item\\s+.*-Recurse",
                                        QRegularExpression::CaseInsensitiveOption))) {
        return Caution;
    }

    return Safe;
}

SafetyChecker::Result SafetyChecker::validatePlan(const OperationPlan& plan) {
    if (plan.isEmpty()) {
        m_lastBlockReason = tr("操作计划为空");
        return Blocked;
    }

    bool hasDangerous = false;

    for (const auto& op : plan.operations) {
        Result r = validateOperation(op);
        if (r == Blocked) return Blocked;
        if (r == NeedsConfirmation) hasDangerous = true;
    }

    if (hasDangerous) return NeedsConfirmation;

    return plan.requiresConfirmation ? NeedsConfirmation : Approved;
}

SafetyChecker::Result SafetyChecker::validateOperation(const ShellOperation& op) {
    // Native file operations: validate source/target paths directly
    if (op.type == ShellOperation::CreateDir || op.type == ShellOperation::MoveFile ||
        op.type == ShellOperation::DeleteFile || op.type == ShellOperation::CopyFile ||
        op.type == ShellOperation::WriteFile || op.type == ShellOperation::SearchFiles) {
        // Collect paths to check from source/target
        QStringList pathsToCheck;
        if (!op.source.isEmpty()) pathsToCheck.append(op.source);
        if (!op.target.isEmpty()) pathsToCheck.append(op.target);

        if (pathsToCheck.isEmpty() && op.command.trimmed().isEmpty()) {
            m_lastBlockReason = tr("操作缺少路径参数");
            return Blocked;
        }

        for (const auto& path : pathsToCheck) {
            if (isSystemPath(path)) {
                m_lastBlockReason = tr("禁止操作系统目录: ") + path;
                return Blocked;
            }
            if (!isPathSafe(path)) {
                m_lastBlockReason = tr("路径不在允许范围内: ") + path;
                return Blocked;
            }
        }

        // DeleteFile is always Caution (destructive)
        if (op.type == ShellOperation::DeleteFile) return NeedsConfirmation;

        return Approved;
    }

    // Shell commands: validate command string
    if (op.command.trimmed().isEmpty()) {
        m_lastBlockReason = tr("命令为空");
        return Blocked;
    }

    if (hasCommandInjection(op.command)) {
        m_lastBlockReason = tr("检测到潜在的命令注入: ") + op.command.left(80);
        return Blocked;
    }

    if (isDangerousCommand(op.command)) {
        return Blocked;  // m_lastBlockReason is already set inside isDangerousCommand
    }

    DangerLevel level = dangerLevel(op);
    if (level == Dangerous) {
        m_lastBlockReason = tr("禁止执行危险命令: ") + op.command.left(80);
        return Blocked;
    }

    // Extract paths from command for whitelist validation
    QStringList paths = extractPathsFromCommand(op.command);
    for (const auto& path : paths) {
        if (isSystemPath(path)) {
            m_lastBlockReason = tr("禁止操作系统目录: ") + path;
            return Blocked;
        }
        if (!isPathSafe(path)) {
            m_lastBlockReason = tr("路径不在允许范围内: ") + path;
            return Blocked;
        }
    }

    if (level == Caution) return NeedsConfirmation;

    return Approved;
}

bool SafetyChecker::hasCommandInjection(const QString& command) {
    // Unix: backtick command substitution: `cmd`
    static QRegularExpression backtick(QStringLiteral("`[^`]+`"));
    if (backtick.match(command).hasMatch()) return true;

    // Unix: $() command substitution
    static QRegularExpression dollarParen(QStringLiteral("\\$\\([^)]+\\)"));
    if (dollarParen.match(command).hasMatch()) return true;

    // Unix: eval / exec / source commands
    static QRegularExpression evalCmd(QStringLiteral("\\beval\\b|\\bexec\\b|\\bsource\\b\\s+/"));
    if (evalCmd.match(command).hasMatch()) return true;

    // Windows: PowerShell eval (Invoke-Expression / iex)
    static QRegularExpression psEval(QStringLiteral("\\b(Invoke-Expression|iex)\\b"),
                                     QRegularExpression::CaseInsensitiveOption);
    if (psEval.match(command).hasMatch()) return true;

    // Windows: PowerShell Invoke-Command
    static QRegularExpression psIcm(QStringLiteral("\\b(Invoke-Command|icm)\\b"),
                                    QRegularExpression::CaseInsensitiveOption);
    if (psIcm.match(command).hasMatch()) return true;

    // Windows: obfuscated PowerShell (-EncodedCommand)
    static QRegularExpression psEncoded(QStringLiteral("-EncodedCommand\\s+\\S"),
                                        QRegularExpression::CaseInsensitiveOption);
    if (psEncoded.match(command).hasMatch()) return true;

    // Windows: cmd /c or cmd /k sub-shell chaining
    static QRegularExpression cmdSubshell(QStringLiteral("\\bcmd\\s+/(c|k)\\b"),
                                          QRegularExpression::CaseInsensitiveOption);
    if (cmdSubshell.match(command).hasMatch()) return true;

    // Windows: COMSPEC expansion
    static QRegularExpression comspec(QStringLiteral("%COMSPEC%"),
                                      QRegularExpression::CaseInsensitiveOption);
    if (comspec.match(command).hasMatch()) return true;

    // Windows: Living-off-the-land binaries
    static QRegularExpression lolbin(QStringLiteral("\\b(certutil\\s+-urlcache|mshta\\b|cscript\\b|"
                                                    "wscript\\b|rundll32\\b|regsvr32\\b)"),
                                     QRegularExpression::CaseInsensitiveOption);
    if (lolbin.match(command).hasMatch()) return true;

    return false;
}

bool SafetyChecker::isDangerousCommand(const QString& command) {
    // ── Privilege escalation (Unix) ──
    if (command.contains(QRegularExpression("\\bsudo\\b"))) {
        m_lastBlockReason = tr("禁止使用 sudo 提权");
        return true;
    }
    if (command.contains(QRegularExpression("\\bsu\\b\\s+-"))) {
        m_lastBlockReason = tr("禁止切换用户");
        return true;
    }
    if (command.contains(QRegularExpression("\\bdoas\\b|\\bpkexec\\b"))) {
        m_lastBlockReason = tr("禁止使用提权命令");
        return true;
    }

    // ── Privilege escalation (Windows) ──
    if (command.contains(
                QRegularExpression("\\brunas\\b", QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止使用 runas 提权");
        return true;
    }
    if (command.contains(
                QRegularExpression("\\bpsexec\\b", QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止使用 PsExec");
        return true;
    }
    if (command.contains(QRegularExpression("Start-Process\\s+.*-Verb\\s+RunAs",
                                            QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止提权启动进程");
        return true;
    }

    // ── Disk-level destructive operations (Unix) ──
    if (command.contains(QRegularExpression("\\brm\\s+.*-r[^\\w]*f?\\s+/"))) {
        m_lastBlockReason = tr("禁止递归删除根目录或系统目录");
        return true;
    }
    if (command.contains(QRegularExpression("\\brm\\s+.*-rf\\s+(/|~|\\$HOME)\\b"))) {
        m_lastBlockReason = tr("禁止删除主目录或根目录");
        return true;
    }
    if (command.contains(QRegularExpression("\\bmkfs\\.|\\bdd\\s+if=|\\bfdisk\\b"))) {
        m_lastBlockReason = tr("禁止磁盘操作命令");
        return true;
    }
    if (command.contains(QRegularExpression(">\\s*/dev/(sd|disk|nvme|mmcblk)"))) {
        m_lastBlockReason = tr("禁止直接写入磁盘设备");
        return true;
    }

    // ── Disk-level destructive operations (Windows) ──
    if (command.contains(QRegularExpression("\\bformat\\s+[A-Za-z]:\\b",
                                            QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止格式化磁盘");
        return true;
    }
    if (command.contains(
                QRegularExpression("\\bdiskpart\\b", QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止磁盘分区操作");
        return true;
    }
    if (command.contains(QRegularExpression(">\\s*\\\\\\\\\\.\\\\PhysicalDrive",
                                            QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止直接写入物理磁盘");
        return true;
    }

    // ── System-level permission modification (Unix) ──
    if (command.contains(QRegularExpression("\\bchmod\\s+[0-7]*7[0-7]*\\s+/")) ||
        command.contains(QRegularExpression("\\bchown\\s+-R\\s+/"))) {
        m_lastBlockReason = tr("禁止修改系统目录权限");
        return true;
    }

    // ── Windows permission takeover ──
    if (command.contains(QRegularExpression("\\bicacls\\s+.*\\/deny",
                                            QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止修改 ACL 权限");
        return true;
    }
    if (command.contains(QRegularExpression("\\btakeown\\s+/f\\s+[A-Za-z]:\\\\",
                                            QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止夺取系统目录所有权");
        return true;
    }

    // ── Windows registry destruction ──
    if (command.contains(QRegularExpression("\\breg\\s+delete\\s+(HKLM|HKEY_LOCAL_MACHINE)",
                                            QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止删除系统注册表项");
        return true;
    }
    if (command.contains(QRegularExpression("Remove-Item\\s+.*HKLM:",
                                            QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止通过 PowerShell 删除注册表");
        return true;
    }

    // ── Windows critical service operations ──
    if (command.contains(QRegularExpression("\\bsc\\s+(delete|stop)\\b",
                                            QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止操作系统服务");
        return true;
    }

    // ── Windows critical process termination ──
    if (command.contains(QRegularExpression("\\btaskkill\\s+/f\\s+/"
                                            "im\\s+(lsass|winlogon|csrss|services|svchost|"
                                            "explorer)",
                                            QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止终止关键系统进程");
        return true;
    }

    // ── Windows system destruction ──
    if (command.contains(QRegularExpression("\\bshutdown\\s+/(s|r)\\s+/t\\s+0",
                                            QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止强制关机/重启");
        return true;
    }
    if (command.contains(QRegularExpression("\\bbcdedit\\s+/delete",
                                            QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止修改引导配置");
        return true;
    }
    if (command.contains(QRegularExpression("\\bvssadmin\\s+delete\\s+shadows",
                                            QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止删除卷影副本");
        return true;
    }

    // ── Windows firewall disruption ──
    if (command.contains(QRegularExpression("netsh\\s+advfirewall\\s+set\\s+allprofiles\\s+"
                                            "state\\s+off",
                                            QRegularExpression::CaseInsensitiveOption))) {
        m_lastBlockReason = tr("禁止关闭防火墙");
        return true;
    }

    return false;
}

QStringList SafetyChecker::extractPathsFromCommand(const QString& command) const {
    QStringList paths;

    // Match double-quoted paths
    static QRegularExpression quotedPath(QStringLiteral("\"([^\"]+)\""));
    QRegularExpressionMatchIterator it = quotedPath.globalMatch(command);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString p = m.captured(1);
        if (p.contains(QLatin1Char('/')) || p.contains(QLatin1Char('\\')) ||
            p.startsWith(QLatin1Char('~')))
            paths.append(p);
    }

    // Match single-quoted paths
    static QRegularExpression singleQuoted(QStringLiteral("'([^']+)'"));
    it = singleQuoted.globalMatch(command);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString p = m.captured(1);
        if (p.contains(QLatin1Char('/')) || p.contains(QLatin1Char('\\')) ||
            p.startsWith(QLatin1Char('~')))
            paths.append(p);
    }

    // Unix: match unquoted absolute or ~/ paths
    static QRegularExpression barePath(
            QStringLiteral("(?<![\\w=-])(/~|/[^\\s;|&<>]+|~[^\\s;|&<>]*)"));
    it = barePath.globalMatch(command);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString p = m.captured(1);
        if (p.length() > 1 && (p.startsWith(QLatin1Char('/')) || p.startsWith(QLatin1Char('~'))))
            paths.append(p);
    }

    // Windows: match drive letter paths (C:\, D:\)
    static QRegularExpression winPath(QStringLiteral("[A-Za-z]:\\\\[^\\s;|&<>]*"));
    it = winPath.globalMatch(command);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        paths.append(m.captured(0));
    }

    // Windows: match %VAR% environment variable paths
    static QRegularExpression envPath(QStringLiteral("%[A-Za-z_]+%[^\\s;|&<>]*"));
    it = envPath.globalMatch(command);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        paths.append(m.captured(0));
    }

    return paths;
}

static QString resolveCanonicalPath(const QString& path) {
    // Resolve symlinks where possible. For non-existent paths,
    // resolve the longest existing parent and append the rest.
    QString clean = QDir::cleanPath(path);
    QFileInfo fi(clean);
    QString canonical = fi.canonicalFilePath();
    if (!canonical.isEmpty()) return canonical;

    // Path doesn't exist — walk up to find existing parent
    QDir dir(clean);
    QStringList missingParts;
    while (!dir.exists() && !dir.isRoot()) {
        missingParts.prepend(dir.dirName());
        dir = QDir(QDir::cleanPath(dir.path() + QStringLiteral("/..")));
    }
    return QDir::cleanPath(dir.canonicalPath() + QStringLiteral("/") +
                           missingParts.join(QStringLiteral("/")));
}

bool SafetyChecker::isPathSafe(const QString& path) const {
    QString resolved = resolveCanonicalPath(
            QFileInfo(path).isAbsolute() ? path
                                         : (QDir::currentPath() + QStringLiteral("/") + path));

    for (const auto& allowed : m_allowedPaths) {
        QString allowedResolved = resolveCanonicalPath(allowed);
        if (resolved.startsWith(allowedResolved)) return true;
    }

    return false;
}

bool SafetyChecker::isSystemPath(const QString& path) const {
    QString resolved = QDir::cleanPath(path).toLower();

    static const QStringList systemPaths = {
            QStringLiteral("/system"),  QStringLiteral("/etc"),     QStringLiteral("/boot"),
            QStringLiteral("/root"),    QStringLiteral("/usr/lib"), QStringLiteral("/usr/sbin"),
            QStringLiteral("/usr/bin"), QStringLiteral("/bin"),     QStringLiteral("/sbin"),
            QStringLiteral("/lib"),     QStringLiteral("/lib64"),   QStringLiteral("/proc"),
            QStringLiteral("/sys"),     QStringLiteral("/dev"),
    };

    for (const auto& sysPath : systemPaths) {
        if (resolved == sysPath || resolved.startsWith(sysPath + QStringLiteral("/"))) return true;
    }

#ifdef Q_OS_MACOS
    static const QStringList macSystemPaths = {
            QStringLiteral("/System"),      QStringLiteral("/Library"),
            QStringLiteral("/.vol"),        QStringLiteral("/.file"),
            QStringLiteral("/private/etc"), QStringLiteral("/private/var"),
    };
    for (const auto& sysPath : macSystemPaths) {
        if (resolved == sysPath || resolved.startsWith(sysPath + QStringLiteral("/"))) return true;
    }
#endif

#ifdef Q_OS_WIN
    {
        QString winDir = QDir::cleanPath(qEnvironmentVariable("WINDIR")).toLower();
        if (!winDir.isEmpty() && resolved.startsWith(winDir)) return true;
        if (resolved.contains(QStringLiteral(":\\windows"))) return true;

        // System32 and SysWOW64
        static const QStringList winSysPaths = {
                QStringLiteral("c:\\windows\\system32"), QStringLiteral("c:\\windows\\syswow64"),
                QStringLiteral("c:\\program files"),     QStringLiteral("c:\\program files (x86)"),
                QStringLiteral("c:\\programdata"),
        };
        for (const auto& sysPath : winSysPaths) {
            if (resolved == sysPath || resolved.startsWith(sysPath + QStringLiteral("\\")))
                return true;
        }

        // Dynamic paths from environment
        QString progFiles = QDir::cleanPath(qEnvironmentVariable("ProgramFiles")).toLower();
        if (!progFiles.isEmpty() && resolved.startsWith(progFiles)) return true;
        QString progFiles86 = QDir::cleanPath(qEnvironmentVariable("ProgramFiles(x86)")).toLower();
        if (!progFiles86.isEmpty() && resolved.startsWith(progFiles86)) return true;
        QString progData = QDir::cleanPath(qEnvironmentVariable("ProgramData")).toLower();
        if (!progData.isEmpty() && resolved.startsWith(progData)) return true;
        QString sysRoot = QDir::cleanPath(qEnvironmentVariable("SystemRoot")).toLower();
        if (!sysRoot.isEmpty() && resolved.startsWith(sysRoot)) return true;

        // Physical drive access
        if (resolved.startsWith(QStringLiteral("\\\\.\\physicaldrive")) ||
            resolved.startsWith(QStringLiteral("\\\\.\\c:")))
            return true;
    }
#endif

    return false;
}
