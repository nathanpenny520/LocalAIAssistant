#include "safetychecker.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

SafetyChecker::SafetyChecker()
{
    resetToDefaults();
}

void SafetyChecker::setAllowedPaths(const QStringList &paths)
{
    m_allowedPaths = paths;
}

void SafetyChecker::addAllowedPath(const QString &path)
{
    if (!m_allowedPaths.contains(path))
        m_allowedPaths.append(path);
}

void SafetyChecker::resetToDefaults()
{
    m_allowedPaths.clear();
    m_allowedPaths.append(QDir::homePath());
    m_allowedPaths.append(QDir::tempPath());
    m_allowedPaths.append(QDir::currentPath());
    m_allowedPaths.append(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    m_allowedPaths.append(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    m_allowedPaths.append(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    m_allowedPaths.append(QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
    m_allowedPaths.append(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation));
    m_allowedPaths.append(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
}

QStringList SafetyChecker::allowedPaths() const
{
    return m_allowedPaths;
}

QString SafetyChecker::lastBlockReason() const
{
    return m_lastBlockReason;
}

SafetyChecker::DangerLevel SafetyChecker::dangerLevel(const ShellOperation &op) const
{
    const QString cmd = op.command.trimmed();
    if (cmd.isEmpty())
        return Safe;

    // 危险命令检测
    if (cmd.contains(QRegularExpression("\\bsudo\\b"))
        || cmd.contains(QRegularExpression("\\bsu\\b\\s+-"))
        || cmd.contains(QRegularExpression("\\bdoas\\b"))
        || cmd.contains(QRegularExpression("\\bpkexec\\b"))) {
        return Dangerous;
    }

    if (cmd.contains(QRegularExpression("\\brm\\s+.*(-r|-rf|--recursive)\\b.*/"))
        || cmd.contains(QRegularExpression("\\bmkfs\\."))
        || cmd.contains(QRegularExpression("\\bdd\\s+if="))
        || cmd.contains(QRegularExpression("\\bfdisk\\b"))) {
        return Dangerous;
    }

    if (cmd.contains(QRegularExpression(">\\s*/dev/(sd|disk|nvme)"))
        || cmd.contains(QRegularExpression("\\bchmod\\s+777\\s+/"))
        || cmd.contains(QRegularExpression("\\bchown\\s+-R\\s+/"))) {
        return Dangerous;
    }

    // 需要确认的操作
    if (cmd.contains(QRegularExpression("\\brm\\b"))
        || cmd.contains(QRegularExpression("\\bgit\\s+push\\s+.*--force"))
        || cmd.contains(QRegularExpression("\\bcurl\\b"))
        || cmd.contains(QRegularExpression("\\bwget\\b"))) {
        return Caution;
    }

    return Safe;
}

SafetyChecker::Result SafetyChecker::validatePlan(const OperationPlan &plan)
{
    if (plan.isEmpty()) {
        m_lastBlockReason = tr("操作计划为空");
        return Blocked;
    }

    bool hasDangerous = false;

    for (const auto &op : plan.operations) {
        Result r = validateOperation(op);
        if (r == Blocked)
            return Blocked;
        if (r == NeedsConfirmation)
            hasDangerous = true;
    }

    if (hasDangerous)
        return NeedsConfirmation;

    return plan.requiresConfirmation ? NeedsConfirmation : Approved;
}

SafetyChecker::Result SafetyChecker::validateOperation(const ShellOperation &op)
{
    if (op.command.trimmed().isEmpty()) {
        m_lastBlockReason = tr("命令为空");
        return Blocked;
    }

    // 命令注入检测
    if (hasCommandInjection(op.command)) {
        m_lastBlockReason = tr("检测到潜在的命令注入: ") + op.command.left(80);
        return Blocked;
    }

    // 危险命令检测
    if (isDangerousCommand(op.command)) {
        return Blocked;  // m_lastBlockReason 已在 isDangerousCommand 中设置
    }

    // 危险级别检测
    DangerLevel level = dangerLevel(op);
    if (level == Dangerous) {
        m_lastBlockReason = tr("禁止执行危险命令: ") + op.command.left(80);
        return Blocked;
    }

    // 命令中提取路径进行路径白名单校验
    QStringList paths = extractPathsFromCommand(op.command);
    for (const auto &path : paths) {
        if (isSystemPath(path)) {
            m_lastBlockReason = tr("禁止操作系统目录: ") + path;
            return Blocked;
        }
        if (!isPathSafe(path)) {
            m_lastBlockReason = tr("路径不在允许范围内: ") + path;
            return Blocked;
        }
    }

    if (level == Caution)
        return NeedsConfirmation;

    return Approved;
}

bool SafetyChecker::hasCommandInjection(const QString &command)
{
    // 检测反引号命令替换: `cmd`
    static QRegularExpression backtick(QStringLiteral("`[^`]+`"));
    if (backtick.match(command).hasMatch())
        return true;

    // 检测 $() 命令替换
    static QRegularExpression dollarParen(QStringLiteral("\\$\\([^)]+\\)"));
    if (dollarParen.match(command).hasMatch())
        return true;

    // 检测 eval / exec / source 命令
    static QRegularExpression evalCmd(QStringLiteral("\\beval\\b|\\bexec\\b|\\bsource\\b\\s+/"));
    if (evalCmd.match(command).hasMatch())
        return true;

    return false;
}

bool SafetyChecker::isDangerousCommand(const QString &command)
{
    // 权限提升
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

    // 磁盘级破坏操作
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

    // 系统级权限修改
    if (command.contains(QRegularExpression("\\bchmod\\s+[0-7]*7[0-7]*\\s+/"))
        || command.contains(QRegularExpression("\\bchown\\s+-R\\s+/"))) {
        m_lastBlockReason = tr("禁止修改系统目录权限");
        return true;
    }

    return false;
}

QStringList SafetyChecker::extractPathsFromCommand(const QString &command) const
{
    QStringList paths;

    // 匹配引号内或空白分隔的路径参数
    // 匹配双引号路径 "~/path"
    static QRegularExpression quotedPath(QStringLiteral("\"([^\"]+)\""));
    QRegularExpressionMatchIterator it = quotedPath.globalMatch(command);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString p = m.captured(1);
        if (p.contains(QLatin1Char('/')) || p.startsWith(QLatin1Char('~')))
            paths.append(p);
    }

    // 匹配单引号路径 '~/path'
    static QRegularExpression singleQuoted(QStringLiteral("'([^']+)'"));
    it = singleQuoted.globalMatch(command);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString p = m.captured(1);
        if (p.contains(QLatin1Char('/')) || p.startsWith(QLatin1Char('~')))
            paths.append(p);
    }

    // 匹配未引号的绝对路径或 ~/ 路径
    static QRegularExpression barePath(QStringLiteral("(?<![\\w=-])(/~|/[^\\s;|&<>]+|~[^\\s;|&<>]*)"));
    it = barePath.globalMatch(command);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString p = m.captured(1);
        // 过滤明显不是路径的匹配（如单字符、数字等）
        if (p.length() > 1 && (p.startsWith(QLatin1Char('/')) || p.startsWith(QLatin1Char('~'))))
            paths.append(p);
    }

    return paths;
}

bool SafetyChecker::isPathSafe(const QString &path) const
{
    QString resolved = QDir::cleanPath(
        QFileInfo(path).isAbsolute()
            ? path
            : (QDir::currentPath() + QStringLiteral("/") + path));

    for (const auto &allowed : m_allowedPaths) {
        QString allowedClean = QDir::cleanPath(allowed);
        if (resolved.startsWith(allowedClean))
            return true;
    }

    return false;
}

bool SafetyChecker::isSystemPath(const QString &path) const
{
    QString resolved = QDir::cleanPath(path).toLower();

    static const QStringList systemPaths = {
        QStringLiteral("/system"),
        QStringLiteral("/etc"),
        QStringLiteral("/boot"),
        QStringLiteral("/root"),
        QStringLiteral("/usr/lib"),
        QStringLiteral("/usr/sbin"),
        QStringLiteral("/usr/bin"),
        QStringLiteral("/bin"),
        QStringLiteral("/sbin"),
        QStringLiteral("/lib"),
        QStringLiteral("/lib64"),
        QStringLiteral("/proc"),
        QStringLiteral("/sys"),
        QStringLiteral("/dev"),
    };

    for (const auto &sysPath : systemPaths) {
        if (resolved == sysPath || resolved.startsWith(sysPath + QStringLiteral("/")))
            return true;
    }

#ifdef Q_OS_MACOS
    static const QStringList macSystemPaths = {
        QStringLiteral("/System"),
        QStringLiteral("/Library"),
        QStringLiteral("/.vol"),
        QStringLiteral("/.file"),
        QStringLiteral("/private/etc"),
        QStringLiteral("/private/var"),
    };
    for (const auto &sysPath : macSystemPaths) {
        if (resolved == sysPath || resolved.startsWith(sysPath + QStringLiteral("/")))
            return true;
    }
#endif

#ifdef Q_OS_WIN
    QString winDir = QDir::cleanPath(qEnvironmentVariable("WINDIR")).toLower();
    if (resolved.startsWith(winDir))
        return true;
    if (resolved.contains(QStringLiteral(":\\windows")))
        return true;
#endif

    return false;
}
