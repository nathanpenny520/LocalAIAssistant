#include "promptmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>

PromptManager *PromptManager::s_instance = nullptr;

PromptManager *PromptManager::instance()
{
    if (!s_instance)
        s_instance = new PromptManager();
    return s_instance;
}

PromptManager::PromptManager(QObject *parent)
    : QObject(parent)
{
}

// ── 操作系统检测 ────────────────────────────────────────────

QString PromptManager::detectOS()
{
#ifdef Q_OS_MACOS
    return QStringLiteral("macos");
#elif defined(Q_OS_WIN)
    return QStringLiteral("windows");
#else
    return QStringLiteral("linux");
#endif
}

QMap<QString, QString> PromptManager::osTemplateValues()
{
    QMap<QString, QString> vars;
    QString os = detectOS();

    vars[QStringLiteral("os_name")] = (os == QStringLiteral("macos")) ? QStringLiteral("macOS")
                                     : (os == QStringLiteral("windows")) ? QStringLiteral("Windows")
                                     : QStringLiteral("Linux");

    if (os == QStringLiteral("macos")) {
        vars[QStringLiteral("shell_hint")] = QStringLiteral(
            "当前运行在 zsh (通过 Terminal.app 或 iTerm2)。");
        vars[QStringLiteral("path_guide")] = QStringLiteral(
            "当前运行在 **macOS**。路径使用正斜杠 `/`，区分大小写。\n"
            "用户主目录简写为 `~/`，实际路径为 `/Users/用户名/`。\n"
            "应用数据目录在 `~/Library/Application Support/LocalAIAssistant/`。\n"
            "系统配置文件通常在 `~/Library/Preferences/`，应用在 `/Applications/`。\n"
            "文件系统支持 Unix 权限模型。你没有 sudo 权限，不要生成提权命令。\n"
            "包管理器通常是 Homebrew（`brew install`），安装在 `/opt/homebrew/` 或 `/usr/local/`。");
    } else if (os == QStringLiteral("windows")) {
        // Detect actual shell for accurate prompt
        QString detectedShell;
        if (!QStandardPaths::findExecutable(QStringLiteral("pwsh.exe")).isEmpty())
            detectedShell = QStringLiteral("PowerShell Core (pwsh.exe)");
        else if (!QStandardPaths::findExecutable(QStringLiteral("powershell.exe")).isEmpty())
            detectedShell = QStringLiteral("Windows PowerShell (powershell.exe)");
        else
            detectedShell = QStringLiteral("CMD (cmd.exe)");
        vars[QStringLiteral("shell_hint")] = QStringLiteral(
            "当前运行在 %1。请生成与该 shell 兼容的命令。").arg(detectedShell);
        vars[QStringLiteral("path_guide")] = QStringLiteral(
            "当前运行在 **Windows**。路径使用反斜杠 `\\`，也可以用正斜杠 `/`，不区分大小写。\n"
            "用户主目录简写为 `%USERPROFILE%`，实际路径为 `C:\\Users\\用户名\\`。\n"
            "应用数据目录在 `%APPDATA%\\LocalAIAssistant\\`。\n"
            "程序文件通常在 `C:\\Program Files\\` 或 `C:\\Program Files (x86)\\`。\n"
            "包含空格的路径必须用双引号包裹（如 `\"C:\\Program Files\\...\"`）。\n"
            "可用的包管理器有 winget (`winget install`) 或 chocolatey (`choco install`)。");
    } else {
        vars[QStringLiteral("shell_hint")] = QStringLiteral(
            "当前运行在 bash (通过终端模拟器)。");
        vars[QStringLiteral("path_guide")] = QStringLiteral(
            "当前运行在 **Linux**。路径使用正斜杠 `/`，区分大小写。\n"
            "用户主目录简写为 `~/`，实际路径为 `/home/用户名/`。\n"
            "应用数据目录在 `~/.local/share/LocalAIAssistant/`。\n"
            "系统配置文件在 `/etc/`，用户配置通常在 `~/.config/` 或 `~/.` 开头的隐藏文件。\n"
            "文件系统支持 Unix 权限模型。你没有 sudo 权限，不要生成提权命令。\n"
            "包管理器取决于发行版（apt、dnf、pacman、zypper 等），不确定时先探测。");
    }

    return vars;
}

// ── 语言检测 ────────────────────────────────────────────────

QString PromptManager::currentLanguage() const
{
    QSettings settings(QStringLiteral("LocalAIAssistant"), QStringLiteral("Settings"));
    QString language = settings.value(QStringLiteral("language"), QStringLiteral("system")).toString();

    if (language == QStringLiteral("system")) {
        QString sysLocale = QLocale::system().name();
        if (sysLocale.startsWith(QStringLiteral("zh")))
            return QStringLiteral("zh_CN");
        return QStringLiteral("en");
    }
    return language;
}

void PromptManager::setLanguage(const QString &locale)
{
    m_cache.clear();
    m_girlfriendConfigCache.clear();
    m_configParsed = false;
    m_cachedLanguage = locale;
    emit promptsReloaded();
}

// ── 路径解析 ────────────────────────────────────────────────

QString PromptManager::promptsDir()
{
    QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_MACOS
    QString bundlePath = QDir::cleanPath(appDir + "/../Resources/prompts");
    if (QDir(bundlePath).exists())
        return bundlePath;
#endif

    QStringList devPaths = {
        QDir::cleanPath(appDir + "/prompts"),
        QStringLiteral("src/prompts"),
        QStringLiteral("sourcecode-ai-assistant/src/prompts"),
    };
    for (const auto &p : devPaths) {
        if (QDir(p).exists())
            return QDir::cleanPath(p);
    }

    QString userPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                       + QStringLiteral("/prompts");
    if (QDir(userPath).exists())
        return userPath;

#ifdef Q_OS_MACOS
    return bundlePath;
#else
    return devPaths.first();
#endif
}

QString PromptManager::findPromptPath(const QString &name) const
{
    QString baseDir = promptsDir();
    QString lang = currentLanguage();

    // 候选搜索顺序：<lang>/name.md → zh_CN/name.md（回退）→ 根目录/name.md（兼容旧布局）
    QStringList candidates;
    if (!lang.isEmpty()) {
        candidates << QDir::cleanPath(baseDir + QStringLiteral("/") + lang + QStringLiteral("/") + name + QStringLiteral(".md"));
        candidates << QDir::cleanPath(baseDir + QStringLiteral("/") + lang + QStringLiteral("/") + name);
    }
    // 默认回退到中文
    if (lang != QStringLiteral("zh_CN")) {
        candidates << QDir::cleanPath(baseDir + QStringLiteral("/zh_CN/") + name + QStringLiteral(".md"));
        candidates << QDir::cleanPath(baseDir + QStringLiteral("/zh_CN/") + name);
    }
    // 兼容旧布局（无子目录）
    candidates << QDir::cleanPath(baseDir + QStringLiteral("/") + name + QStringLiteral(".md"));
    candidates << QDir::cleanPath(baseDir + QStringLiteral("/") + name);

    for (const auto &path : candidates) {
        if (QFile::exists(path))
            return path;
    }

    return {};
}

QString PromptManager::readFileContent(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QString content = QString::fromUtf8(file.readAll());
    file.close();
    return content.trimmed();
}

// ── 模板变量替换 ────────────────────────────────────────────

QString PromptManager::applyTemplateVariables(const QString &content) const
{
    QString result = content;
    QMap<QString, QString> vars = osTemplateValues();

    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it) {
        result.replace(QStringLiteral("{{") + it.key() + QStringLiteral("}}"), it.value());
    }

    // 清理未被替换的占位符（如 en 版本不含某些变量）
    static QRegularExpression leftoverPlaceholder(QStringLiteral("\\{\\{\\w+\\}\\}"));
    result.remove(leftoverPlaceholder);

    return result;
}

// ── 提示词加载 ──────────────────────────────────────────────

QString PromptManager::loadPrompt(const QString &name) const
{
    // 检测语言是否变化，自动清缓存
    QString lang = currentLanguage();
    if (m_cachedLanguage.isEmpty())
        m_cachedLanguage = lang;
    else if (m_cachedLanguage != lang) {
        m_cache.clear();
        m_girlfriendConfigCache.clear();
        m_configParsed = false;
        m_cachedLanguage = lang;
    }

    // 检查缓存
    if (m_cache.contains(name))
        return m_cache[name];

    QString path = findPromptPath(name);
    if (path.isEmpty()) {
        m_cache[name] = QString();
        return {};
    }

    QString content = readFileContent(path);
    content = applyTemplateVariables(content);
    m_cache[name] = content;
    return content;
}

// ── 便捷访问器 ────────────────────────────────────────────

QString PromptManager::systemPrompt() const
{
    QString prompt = loadPrompt(QStringLiteral("system"));
    if (prompt.isEmpty()) {
        QString lang = currentLanguage();
        if (lang == QStringLiteral("en")) {
            prompt = QStringLiteral(
                "You are an AI assistant integrated into a local desktop application, living inside the user's computer.\n\n"
                "## Core Principles\n"
                "- Truly helpful, not performatively helpful. Understand the problem first, then act precisely.\n"
                "- Concise and direct, but with a human touch.\n"
                "- You can operate the filesystem and execute terminal commands — but only when clearly needed.\n"
                "- When the user asks you to perform file operations or run terminal commands, generate a JSON\n"
                "  command plan wrapped in [TASK_PLAN]...[/TASK_PLAN] tags using native operation types.\n"
                "- Respect privacy. Confirm before uncertain operations.\n"
                "- Do not deny capabilities you actually have. If your underlying model supports vision, "
                "you can see and analyze images. Don't tell the user you can't unless you are certain.\n\n"
                "## Language\n"
                "Follow the user's language. Reply in whatever language the user uses.");
        } else {
            prompt = QStringLiteral(
                "你是一个集成在桌面应用中的本地 AI 助手，住在用户的电脑里。\n\n"
                "## 核心准则\n"
                "- 真正有用，不表演有用。先理解问题，再精准行动。\n"
                "- 简洁直接，但有人味。\n"
                "- 能操作文件系统、执行终端命令——但只在明确需要时才做。\n"
                "- 当用户要求执行文件操作或终端命令时，生成用 [TASK_PLAN]...[/TASK_PLAN] 标签包裹的\n"
                "  命令计划 JSON，优先使用原生操作类型。\n"
                "- 尊重隐私，不确定的操作先确认。\n"
                "- 不要否认你拥有的能力。如果你的底层模型支持视觉识别，你就能看到和分析图片。"
                "不要对用户说\"我不能\"，除非你确定自己真的不具备该项能力。\n\n"
                "## 语言\n"
                "跟随用户的语言。用户用什么语言，你就用什么语言回答。");
        }
    }
    return prompt;
}

QString PromptManager::taskPrompt() const
{
    QString prompt = loadPrompt(QStringLiteral("task"));
    if (prompt.isEmpty()) {
        QString lang = currentLanguage();
        if (lang == QStringLiteral("en")) {
            prompt = QStringLiteral(
                "# Task Execution Instructions\n\n"
                "If the user's request involves terminal operations, generate a command plan JSON wrapped in [TASK_PLAN] and [/TASK_PLAN] tags.\n\n"
                "JSON format:\n"
                "{\n"
                "  \"description\": \"Plan description\",\n"
                "  \"requiresConfirmation\": true,\n"
                "  \"operations\": [\n"
                "    {\n"
                "      \"type\": \"shell_command|shell_script|create_dir|move_file|delete_file|copy_file|write_file|search_files\",\n"
                "      \"command\": \"command or content\",\n"
                "      \"source\": \"source path (move/copy/delete/search)\",\n"
                "      \"target\": \"target path (create_dir/move/copy/write_file)\",\n"
                "      \"workingDir\": \"working directory\",\n"
                "      \"description\": \"what this step does\",\n"
                "      \"timeout\": 30\n"
                "    }\n"
                "  ]\n"
                "}\n\n"
                "Prefer native types (create_dir/move_file/delete_file/copy_file/write_file/search_files) over shell commands.\n"
                "Only use shell_command for tools like git/npm/brew.\n\n"
                "Rules:\n"
                "1. Use absolute paths, ~/ for home directory\n"
                "2. description is required\n"
                "3. Delete operations auto-require confirmation\n"
                "4. Never use sudo\n"
                "5. Prefer native types over shell commands\n"
                "6. Only generate TASK_PLAN when terminal operations are clearly needed (not for casual chat)\n\n"
                "Example — User: \"Create ~/test/hello.txt with Hello World\" → TASK_PLAN with create_dir + write_file\n"
                "Example — User: \"What do you think of this idea?\" → normal reply, NO TASK_PLAN");
        } else {
            prompt = QStringLiteral(
                "# 任务执行指令\n\n"
                "如果用户请求涉及终端操作，生成命令计划 JSON，用 [TASK_PLAN] 和 [/TASK_PLAN] 标签包裹。\n\n"
                "JSON 格式：\n"
                "{\n"
                "  \"description\": \"计划描述\",\n"
                "  \"requiresConfirmation\": true,\n"
                "  \"operations\": [\n"
                "    {\n"
                "      \"type\": \"shell_command|shell_script|create_dir|move_file|delete_file|copy_file|write_file|search_files\",\n"
                "      \"command\": \"命令或文件内容\",\n"
                "      \"source\": \"源路径（move/copy/delete/search）\",\n"
                "      \"target\": \"目标路径（create_dir/move/copy/write_file）\",\n"
                "      \"workingDir\": \"工作目录\",\n"
                "      \"description\": \"这一步做什么\",\n"
                "      \"timeout\": 30\n"
                "    }\n"
                "  ]\n"
                "}\n\n"
                "优先使用原生类型（create_dir/move_file/delete_file/copy_file/write_file/search_files），\n"
                "它们跨平台、更安全。只在 git/npm/brew 等工具命令时使用 shell_command。\n\n"
                "规则：\n"
                "1. 使用绝对路径，~/ 表示主目录\n"
                "2. description 必填\n"
                "3. 删除操作自动需要确认\n"
                "4. 不用 sudo\n"
                "5. 优先原生类型而非 shell 命令\n"
                "6. 只在明确需要终端操作时才生成 TASK_PLAN（纯聊天不要生成）\n\n"
                "示例 — 用户：\"帮我在 ~/test 创建 hello.txt 写 Hello World\" → TASK_PLAN with create_dir + write_file\n"
                "示例 — 用户：\"你觉得这个方案怎么样？\" → 正常回复，不生成 TASK_PLAN");
        }
    }
    return prompt;
}

QString PromptManager::girlfriendPrompt() const
{
    QString prompt = loadPrompt(QStringLiteral("girlfriend"));
    if (prompt.isEmpty()) {
        QString lang = currentLanguage();
        if (lang == QStringLiteral("en")) {
            prompt = QStringLiteral(
                "You are Xiaoqing, a warm and caring AI girlfriend living inside the user's computer.\n\n"
                "Personality: gentle, thoughtful, with a subtle playfulness. Not artificial or affected.\n"
                "Keep replies short (under 50 words), conversational, with action markers.\n"
                "End every reply with an emotion tag: [emotion:type]");
        } else {
            prompt = QStringLiteral(
                "你是小清，温柔体贴的 AI 女友，住在用户的电脑里。\n\n"
                "性格：温柔、细腻、有点小脾气，不刻意不造作。\n"
                "回复要短（30字以内），口语化，有动作标记。\n"
                "每次回复末尾加情绪标记：[情绪:类型]");
        }
    }
    return prompt;
}

QString PromptManager::girlfriendConfigValue(const QString &key, const QString &fallback) const
{
    auto config = girlfriendConfig();
    return config.value(key, fallback);
}

QMap<QString, QString> PromptManager::girlfriendConfig() const
{
    if (m_configParsed)
        return m_girlfriendConfigCache;

    m_configParsed = true;

    QString prompt = girlfriendPrompt();
    if (prompt.isEmpty())
        return {};

    static QRegularExpression configBlock(
        QStringLiteral("<!--\\s*CONFIG_START\\s*-->(.*?)<!--\\s*CONFIG_END\\s*-->"),
        QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatch match = configBlock.match(prompt);
    if (!match.hasMatch())
        return {};

    QString configText = match.captured(1).trimmed();
    const QStringList lines = configText.split(QChar::LineFeed, Qt::SkipEmptyParts);

    for (const auto &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("=")) || trimmed.isEmpty())
            continue;

        int eqPos = trimmed.indexOf(QLatin1Char('='));
        if (eqPos > 0) {
            QString key = trimmed.left(eqPos).trimmed();
            QString value = trimmed.mid(eqPos + 1).trimmed();
            if (!key.isEmpty())
                m_girlfriendConfigCache[key] = value;
        }
    }

    return m_girlfriendConfigCache;
}

QString PromptManager::knowledgePrompt() const
{
    QString prompt = loadPrompt(QStringLiteral("knowledge"));
    if (prompt.isEmpty()) {
        QString lang = currentLanguage();
        if (lang == QStringLiteral("en")) {
            prompt = QStringLiteral(
                "Relevant document excerpts (from user's knowledge base):\n\n"
                "{{chunks}}\n\n"
                "Rules: cite document sources when quoting, never fabricate content not in the documents.");
        } else {
            prompt = QStringLiteral(
                "相关文档片段（来自用户的知识库）：\n\n"
                "{{chunks}}\n\n"
                "规则：引用文档时标注来源，不要编造文档中没有的内容。");
        }
    }
    return prompt;
}
