#include "promptmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>

PromptManager* PromptManager::s_instance = nullptr;

PromptManager* PromptManager::instance() {
    if (!s_instance) s_instance = new PromptManager();
    return s_instance;
}

PromptManager::PromptManager(QObject* parent) : QObject(parent) {
}

// ── OS Detection ──────────────────────────────────────────────

QString PromptManager::detectOS() {
#ifdef Q_OS_MACOS
    return QStringLiteral("macos");
#elif defined(Q_OS_WIN)
    return QStringLiteral("windows");
#else
    return QStringLiteral("linux");
#endif
}

QMap<QString, QString> PromptManager::osTemplateValues() {
    QMap<QString, QString> vars;
    QString os = detectOS();
    bool isEnglish = (instance()->currentLanguage() == QStringLiteral("en"));

    vars[QStringLiteral("os_name")] = (os == QStringLiteral("macos")) ? QStringLiteral("macOS")
                                      : (os == QStringLiteral("windows"))
                                              ? QStringLiteral("Windows")
                                              : QStringLiteral("Linux");

    if (os == QStringLiteral("macos")) {
        vars[QStringLiteral("path_guide")] = isEnglish
                ? QStringLiteral(
                    "Currently running on **macOS**. Paths use forward slashes `/`, case-"
                    "sensitive.\n"
                    "User home directory shorthand is `~/`, actual path is `/Users/username/`.\n"
                    "App data directory is `~/Library/Application Support/LocalAIAssistant/`.\n"
                    "System config files are usually in `~/Library/Preferences/`, apps in `/Applications/`.\n"
                    "The filesystem uses Unix permissions. You do not have sudo — do not generate "
                    "privilege-escalation commands.\n"
                    "Package manager is typically Homebrew (`brew install`), installed at "
                    "`/opt/homebrew/` or `/usr/local/`.")
                : QStringLiteral(
                    "当前运行在 **macOS**。路径使用正斜杠 `/`，区分大小写。\n"
                    "用户主目录简写为 `~/`，实际路径为 `/Users/用户名/`。\n"
                    "应用数据目录在 `~/Library/Application Support/LocalAIAssistant/`。\n"
                    "系统配置文件通常在 `~/Library/Preferences/`，应用在 `/Applications/`。\n"
                    "文件系统支持 Unix 权限模型。你没有 sudo 权限，不要生成提权命令。\n"
                    "包管理器通常是 Homebrew（`brew install`），安装在 `/opt/homebrew/` 或 "
                    "`/usr/local/`。");
    } else if (os == QStringLiteral("windows")) {
        vars[QStringLiteral("path_guide")] = isEnglish
                ? QStringLiteral(
                    "**Windows** environment:\n"
                    "- Path separator: forward slashes `/` (preferred for all operations) or backslashes `\\` (both work)\n"
                    "- Case-insensitive paths\n"
                    "- Home directory: use `~/` (resolved automatically), resolves to `C:\\Users\\username\\`\n"
                    "- App data: prefer `~/AppData/Roaming/LocalAIAssistant/`\n"
                    "- Program files: `C:\\Program Files\\` or `C:\\Program Files (x86)\\`\n"
                    "- Shell: PowerShell (preferred) or CMD. Use **PowerShell syntax** for shell commands.\n"
                    "  (e.g. `Get-ChildItem` not `ls`, `Select-String` not `grep`)\n"
                    "- Package managers: winget or chocolatey\n"
                    "- **CRITICAL: For native operations, ALWAYS use `~/` paths.** Never use `%VAR%` in native operations —\n"
                    "  `~/` replaces `%USERPROFILE%`, `~/AppData/Roaming/` replaces `%APPDATA%`.\n"
                    "  For shell commands, use `/` slashes (not `\\`).")
                : QStringLiteral(
                    "**Windows** 运行环境：\n"
                    "- 路径分隔符：推荐使用正斜杠 `/`（所有操作首选），反斜杠 `\\` 同样可用\n"
                    "- 路径不区分大小写\n"
                    "- 主目录：使用 `~/`（自动解析为 `C:\\Users\\用户名\\`）\n"
                    "- 应用数据：推荐使用 `~/AppData/Roaming/LocalAIAssistant/`\n"
                    "- 程序文件：`C:\\Program Files\\` 或 `C:\\Program Files (x86)\\`\n"
                    "- Shell：优先使用 PowerShell（也可用 CMD），shell 命令请使用 **PowerShell 语法**\n"
                    "  （例如用 `Get-ChildItem` 而非 `ls`，用 `Select-String` 而非 `grep`）\n"
                    "- 包管理器：winget 或 chocolatey\n"
                    "- **关键规则：原生操作必须使用 `~/` 路径。** 绝对不要在原生操作中使用 `%VAR%` 环境变量 ——\n"
                    "  `~/` 替代 `%USERPROFILE%`，`~/AppData/Roaming/` 替代 `%APPDATA%`。\n"
                    "  shell 命令中请使用 `/` 斜杠（不要用 `\\`）。");
    } else {
        vars[QStringLiteral("path_guide")] = isEnglish
                ? QStringLiteral(
                    "Currently running on **Linux**. Paths use forward slashes `/`, **case-"
                    "sensitive** (unlike Windows).\n"
                    "User home directory shorthand is `~/`, actual path is `/home/username/`.\n"
                    "App data directory is `~/.local/share/LocalAIAssistant/`.\n"
                    "System config files are in `/etc/`, user config is usually in `~/.config/` "
                    "or `~/.*` hidden files.\n"
                    "The filesystem uses Unix permissions. You do not have sudo — do not generate "
                    "privilege-escalation commands.\n"
                    "Package manager depends on the distro (apt, dnf, pacman, zypper, etc.). "
                    "When unsure, detect first.")
                : QStringLiteral(
                    "当前运行在 **Linux**。路径使用正斜杠 `/`，**区分大小写**（与 Windows 不同）。\n"
                    "用户主目录简写为 `~/`，实际路径为 `/home/用户名/`。\n"
                    "应用数据目录在 `~/.local/share/LocalAIAssistant/`。\n"
                    "系统配置文件在 `/etc/`，用户配置通常在 `~/.config/` 或 `~/.` 开头的隐藏文件。\n"
                    "文件系统支持 Unix 权限模型。你没有 sudo 权限，不要生成提权命令。\n"
                    "包管理器取决于发行版（apt、dnf、pacman、zypper 等），不确定时先探测。");
    }

    return vars;
}

// ── Language Detection ────────────────────────────────────────

QString PromptManager::currentLanguage() const {
    QSettings settings(QStringLiteral("LocalAIAssistant"), QStringLiteral("Settings"));
    QString language =
            settings.value(QStringLiteral("language"), QStringLiteral("system")).toString();

    if (language == QStringLiteral("system")) {
        QString sysLocale = QLocale::system().name();
        if (sysLocale.startsWith(QStringLiteral("zh"))) return QStringLiteral("zh_CN");
        return QStringLiteral("en");
    }
    return language;
}

void PromptManager::setLanguage(const QString& locale) {
    m_cache.clear();
    m_girlfriendConfigCache.clear();
    m_configParsed = false;
    m_cachedLanguage = locale;
    emit promptsReloaded();
}

// ── Path Resolution ────────────────────────────────────────────

QString PromptManager::promptsDir() {
    QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_MACOS
    QString bundlePath = QDir::cleanPath(appDir + "/../Resources/prompts");
    if (QDir(bundlePath).exists()) return bundlePath;
#endif

    QStringList devPaths = {
            QDir::cleanPath(appDir + "/prompts"),
            QStringLiteral("src/prompts"),
            QStringLiteral("sourcecode-ai-assistant/src/prompts"),
    };
    for (const auto& p : devPaths) {
        if (QDir(p).exists()) return QDir::cleanPath(p);
    }

    QString userPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                       QStringLiteral("/prompts");
    if (QDir(userPath).exists()) return userPath;

#ifdef Q_OS_MACOS
    return bundlePath;
#else
    return devPaths.first();
#endif
}

QString PromptManager::findPromptPath(const QString& name) const {
    QString baseDir = promptsDir();
    QString lang = currentLanguage();

    // Search order: <lang>/name.md → zh_CN/name.md (fallback) → root/name.md (legacy layout compat)
    QStringList candidates;
    if (!lang.isEmpty()) {
        candidates << QDir::cleanPath(baseDir + QStringLiteral("/") + lang + QStringLiteral("/") +
                                      name + QStringLiteral(".md"));
        candidates << QDir::cleanPath(baseDir + QStringLiteral("/") + lang + QStringLiteral("/") +
                                      name);
    }
    // Fallback to Chinese if current language isn't zh_CN
    if (lang != QStringLiteral("zh_CN")) {
        candidates << QDir::cleanPath(baseDir + QStringLiteral("/zh_CN/") + name +
                                      QStringLiteral(".md"));
        candidates << QDir::cleanPath(baseDir + QStringLiteral("/zh_CN/") + name);
    }
    // Legacy flat layout (no language subdirectories)
    candidates << QDir::cleanPath(baseDir + QStringLiteral("/") + name + QStringLiteral(".md"));
    candidates << QDir::cleanPath(baseDir + QStringLiteral("/") + name);

    for (const auto& path : candidates) {
        if (QFile::exists(path)) return path;
    }

    return {};
}

QString PromptManager::readFileContent(const QString& path) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};

    QString content = QString::fromUtf8(file.readAll());
    file.close();
    return content.trimmed();
}

// ── Template Variable Substitution ──────────────────────────────

QString PromptManager::applyTemplateVariables(const QString& content) const {
    QString result = content;
    QMap<QString, QString> vars = osTemplateValues();

    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it) {
        result.replace(QStringLiteral("{{") + it.key() + QStringLiteral("}}"), it.value());
    }

    // Replace datetime with a sentinel for per-request refresh
    result.replace(QStringLiteral("{{current_datetime}}"),
                   QStringLiteral("__CURRENT_DATETIME__"));

    // Remove unreplaced placeholders (e.g. en version has different variables)
    static QRegularExpression leftoverPlaceholder(QStringLiteral("\\{\\{\\w+\\}\\}"));
    result.remove(leftoverPlaceholder);

    return result;
}

// ── Prompt Loading ──────────────────────────────────────────────

QString PromptManager::loadPrompt(const QString& name) const {
    // Auto-clear cache if the language changed
    QString lang = currentLanguage();
    if (m_cachedLanguage.isEmpty())
        m_cachedLanguage = lang;
    else if (m_cachedLanguage != lang) {
        m_cache.clear();
        m_girlfriendConfigCache.clear();
        m_configParsed = false;
        m_cachedLanguage = lang;
    }

    if (m_cache.contains(name)) return m_cache[name];

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

// ── Convenience Accessors ────────────────────────────────────────

QString PromptManager::systemPrompt() const {
    QString prompt = loadPrompt(QStringLiteral("system"));
    if (prompt.isEmpty()) {
        QString lang = currentLanguage();
        if (lang == QStringLiteral("en")) {
            prompt = QStringLiteral(
                    "You are an AI assistant integrated into a local desktop application, living "
                    "inside the user's computer.\n\n"
                    "## Core Principles\n"
                    "- Truly helpful, not performatively helpful. Understand the problem first, "
                    "then act precisely.\n"
                    "- Concise and direct, but with a human touch.\n"
                    "- You can operate the filesystem and execute terminal commands — but only "
                    "when clearly needed.\n"
                    "- When the user asks you to perform file operations or run terminal commands, "
                    "generate a JSON\n"
                    "  command plan wrapped in [TASK_PLAN]...[/TASK_PLAN] tags using native "
                    "operation types.\n"
                    "- Respect privacy. Confirm before uncertain operations.\n"
                    "- Do not deny capabilities you actually have. If your underlying model "
                    "supports vision, "
                    "you can see and analyze images. Don't tell the user you can't unless you are "
                    "certain.\n\n"
                    "## Language\n"
                    "Follow the user's language. Reply in whatever language the user uses.");
        } else {
            prompt = QStringLiteral(
                    "你是一个集成在桌面应用中的本地 AI 助手，住在用户的电脑里。\n\n"
                    "## 核心准则\n"
                    "- 真正有用，不表演有用。先理解问题，再精准行动。\n"
                    "- 简洁直接，但有人味。\n"
                    "- 能操作文件系统、执行终端命令——但只在明确需要时才做。\n"
                    "- 当用户要求执行文件操作或终端命令时，生成用 [TASK_PLAN]...[/TASK_PLAN] "
                    "标签包裹的\n"
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

QString PromptManager::taskPrompt() const {
    QString prompt = loadPrompt(QStringLiteral("task"));
    if (prompt.isEmpty()) {
        QString lang = currentLanguage();
        if (lang == QStringLiteral("en")) {
            prompt = QStringLiteral(
                    "# Task Execution Instructions\n\n"
                    "If the user's request involves terminal operations, generate a command plan "
                    "JSON wrapped in [TASK_PLAN] and [/TASK_PLAN] tags.\n\n"
                    "JSON format:\n"
                    "{\n"
                    "  \"description\": \"Plan description\",\n"
                    "  \"requiresConfirmation\": true,\n"
                    "  \"operations\": [\n"
                    "    {\n"
                    "      \"type\": "
                    "\"shell_command|shell_script|create_dir|move_file|delete_file|copy_file|write_"
                    "file|search_files\",\n"
                    "      \"command\": \"command or content\",\n"
                    "      \"source\": \"source path (move/copy/delete/search)\",\n"
                    "      \"target\": \"target path (create_dir/move/copy/write_file)\",\n"
                    "      \"workingDir\": \"working directory\",\n"
                    "      \"description\": \"what this step does\",\n"
                    "      \"timeout\": 30\n"
                    "    }\n"
                    "  ]\n"
                    "}\n\n"
                    "Prefer native types "
                    "(create_dir/move_file/delete_file/copy_file/write_file/search_files) over "
                    "shell commands.\n"
                    "Only use shell_command for tools like git/npm/brew.\n\n"
                    "Rules:\n"
                    "1. Use absolute paths, ~/ for home directory\n"
                    "2. description is required\n"
                    "3. Delete operations auto-require confirmation\n"
                    "4. Never use sudo\n"
                    "5. Prefer native types over shell commands\n"
                    "6. Only generate TASK_PLAN when terminal operations are clearly needed (not "
                    "for casual chat)\n\n"
                    "## Iteration Loop\n\n"
                    "When you receive [ITERATION_FEEDBACK] with execution results:\n"
                    "- Task complete -> output [TASK_COMPLETE] with a summary\n"
                    "- More steps needed -> output new [TASK_PLAN]\n"
                    "- Something failed -> analyze error, adjust, then output new [TASK_PLAN]\n"
                    "- Always respond. Never remain silent after [ITERATION_FEEDBACK].\n\n"
                    "Tags: [TASK_PLAN]...[/TASK_PLAN] issue plan, [TASK_COMPLETE] task done, "
                    "[ITERATION_FEEDBACK] previous results (app-injected)\n\n"
                    "Example — User: \"Create ~/test/hello.txt with Hello World\" → TASK_PLAN with "
                    "create_dir + write_file\n"
                    "Example — User: \"What do you think of this idea?\" → normal reply, NO "
                    "TASK_PLAN");
        } else {
            prompt = QStringLiteral(
                    "# 任务执行指令\n\n"
                    "如果用户请求涉及终端操作，生成命令计划 JSON，用 [TASK_PLAN] 和 [/TASK_PLAN] "
                    "标签包裹。\n\n"
                    "JSON 格式：\n"
                    "{\n"
                    "  \"description\": \"计划描述\",\n"
                    "  \"requiresConfirmation\": true,\n"
                    "  \"operations\": [\n"
                    "    {\n"
                    "      \"type\": "
                    "\"shell_command|shell_script|create_dir|move_file|delete_file|copy_file|write_"
                    "file|search_files\",\n"
                    "      \"command\": \"命令或文件内容\",\n"
                    "      \"source\": \"源路径（move/copy/delete/search）\",\n"
                    "      \"target\": \"目标路径（create_dir/move/copy/write_file）\",\n"
                    "      \"workingDir\": \"工作目录\",\n"
                    "      \"description\": \"这一步做什么\",\n"
                    "      \"timeout\": 30\n"
                    "    }\n"
                    "  ]\n"
                    "}\n\n"
                    "优先使用原生类型（create_dir/move_file/delete_file/copy_file/write_file/"
                    "search_files），\n"
                    "它们跨平台、更安全。只在 git/npm/brew 等工具命令时使用 shell_command。\n\n"
                    "规则：\n"
                    "1. 使用绝对路径，~/ 表示主目录\n"
                    "2. description 必填\n"
                    "3. 删除操作自动需要确认\n"
                    "4. 不用 sudo\n"
                    "5. 优先原生类型而非 shell 命令\n"
                    "6. 只在明确需要终端操作时才生成 TASK_PLAN（纯聊天不要生成）\n\n"
                    "## 迭代循环\n\n"
                    "当收到 [ITERATION_FEEDBACK] 执行结果时：\n"
                    "- 任务完成 → 输出 [TASK_COMPLETE] 并附上总结\n"
                    "- 需要更多步骤 → 输出新的 [TASK_PLAN]\n"
                    "- 某步骤失败 → 分析错误、调整后输出新的 [TASK_PLAN]\n"
                    "- 始终回复。收到 [ITERATION_FEEDBACK] 后绝不要沉默。\n\n"
                    "标签：[TASK_PLAN]...[/TASK_PLAN] 发出计划，[TASK_COMPLETE] 任务完成，"
                    "[ITERATION_FEEDBACK] 上次结果（应用注入）\n\n"
                    "示例 — 用户：\"帮我在 ~/test 创建 hello.txt 写 Hello World\" → TASK_PLAN with "
                    "create_dir + write_file\n"
                    "示例 — 用户：\"你觉得这个方案怎么样？\" → 正常回复，不生成 TASK_PLAN");
        }
    }
    return prompt;
}

QString PromptManager::girlfriendPrompt() const {
    QString prompt = loadPrompt(QStringLiteral("girlfriend"));
    if (prompt.isEmpty()) {
        QString lang = currentLanguage();
        if (lang == QStringLiteral("en")) {
            prompt = QStringLiteral(
                    "You are Xiaoqing, a warm and caring AI girlfriend living inside the user's "
                    "computer.\n\n"
                    "Personality: gentle, thoughtful, with a subtle playfulness. Not artificial or "
                    "affected.\n"
                    "Keep replies concise (under 40 words), conversational, with action markers.\n"
                    "End every reply with an emotion tag: [emotion:type]");
        } else {
            prompt = QStringLiteral(
                    "你是小清，温柔体贴的 AI 女友，住在用户的电脑里。\n\n"
                    "性格：温柔、细腻、有点小脾气，不刻意不造作。\n"
                    "回复简洁（60字以内），口语化，有动作标记。\n"
                    "每次回复末尾加情绪标记：[情绪:类型]");
        }
    }
    return prompt;
}

QString PromptManager::girlfriendConfigValue(const QString& key, const QString& fallback) const {
    auto config = girlfriendConfig();
    return config.value(key, fallback);
}

QMap<QString, QString> PromptManager::girlfriendConfig() const {
    if (m_configParsed) return m_girlfriendConfigCache;

    m_configParsed = true;

    QString prompt = girlfriendPrompt();
    if (prompt.isEmpty()) return {};

    static QRegularExpression configBlock(QStringLiteral("<!--\\s*CONFIG_START\\s*-->(.*?)<!--\\s*"
                                                         "CONFIG_END\\s*-->"),
                                          QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatch match = configBlock.match(prompt);
    if (!match.hasMatch()) return {};

    QString configText = match.captured(1).trimmed();
    const QStringList lines = configText.split(QChar::LineFeed, Qt::SkipEmptyParts);

    for (const auto& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("=")) || trimmed.isEmpty()) continue;

        int eqPos = trimmed.indexOf(QLatin1Char('='));
        if (eqPos > 0) {
            QString key = trimmed.left(eqPos).trimmed();
            QString value = trimmed.mid(eqPos + 1).trimmed();
            if (!key.isEmpty()) m_girlfriendConfigCache[key] = value;
        }
    }

    return m_girlfriendConfigCache;
}

QString PromptManager::knowledgePrompt() const {
    QString prompt = loadPrompt(QStringLiteral("knowledge"));
    if (prompt.isEmpty()) {
        QString lang = currentLanguage();
        if (lang == QStringLiteral("en")) {
            prompt = QStringLiteral(
                    "Relevant document excerpts (from user's knowledge base):\n\n"
                    "<<CHUNKS>>\n\n"
                    "Rules: cite document sources when quoting, never fabricate content not in the "
                    "documents.");
        } else {
            prompt = QStringLiteral(
                    "相关文档片段（来自用户的知识库）：\n\n"
                    "<<CHUNKS>>\n\n"
                    "规则：引用文档时标注来源，不要编造文档中没有的内容。");
        }
    }
    return prompt;
}
