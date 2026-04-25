#include "cli_application.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>
#include <QSettings>
#include <iostream>
#include "networkmanager.h"
#include "sessionmanager.h"

CLIApplication::CLIApplication(QObject *parent)
    : QObject(parent)
    , m_networkManager(nullptr)
    , m_running(false)
    , m_interactiveMode(false)
    , m_isStreaming(false)
{
}

int CLIApplication::run(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("LocalAIAssistant-CLI");
    QCoreApplication::setApplicationVersion("1.0.0");

    m_networkManager = new NetworkManager(this);
    connect(m_networkManager, &NetworkManager::responseReceived,
            this, &CLIApplication::onResponseReceived);
    connect(m_networkManager, &NetworkManager::errorOccurred,
            this, &CLIApplication::onErrorOccurred);
    connect(m_networkManager, &NetworkManager::streamChunkReceived,
            this, &CLIApplication::onStreamChunkReceived);
    connect(m_networkManager, &NetworkManager::streamFinished,
            this, &CLIApplication::onStreamFinished);

    QCommandLineParser parser;
    parser.setApplicationDescription("\n本地AI助手 - 命令行版本\n"
                                     "支持交互式对话和单次查询模式");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("command",
        "命令: chat (交互式对话) | ask <问题> (单次查询) | sessions (会话管理) | config (配置管理)");

    QCommandLineOption sessionOpt(QStringList() << "s" << "session",
        "指定会话ID进行对话", "session-id");
    QCommandLineOption newSessionOpt(QStringList() << "n" << "new",
        "创建新会话");
    QCommandLineOption listOpt(QStringList() << "l" << "list",
        "列出所有会话");
    QCommandLineOption deleteOpt(QStringList() << "d" << "delete",
        "删除指定会话", "session-id");
    QCommandLineOption showOpt(QStringList() << "show",
        "显示指定会话内容", "session-id");
    QCommandLineOption apiURLOpt(QStringList() << "api-url",
        "设置API基础URL", "url");
    QCommandLineOption apiKeyOpt(QStringList() << "api-key",
        "设置API密钥", "key");
    QCommandLineOption modelOpt(QStringList() << "model",
        "设置模型名称", "name");
    QCommandLineOption localModeOpt(QStringList() << "local",
        "使用本地模式");
    QCommandLineOption externalModeOpt(QStringList() << "external",
        "使用外部API模式");
    QCommandLineOption showConfigOpt(QStringList() << "show-config",
        "显示当前配置");
    QCommandLineOption noStreamOpt(QStringList() << "no-stream",
        "禁用流式输出");
    QCommandLineOption temperatureOpt(QStringList() << "temperature",
        "设置温度参数 (0.0-2.0)", "value");
    QCommandLineOption topPOpt(QStringList() << "top-p",
        "设置 top_p 参数 (0.0-1.0)", "value");
    QCommandLineOption maxTokensOpt(QStringList() << "max-tokens",
        "设置最大输出 tokens (1-128000)", "value");
    QCommandLineOption presencePenaltyOpt(QStringList() << "presence-penalty",
        "设置存在惩罚 (-2.0-2.0)", "value");
    QCommandLineOption frequencyPenaltyOpt(QStringList() << "frequency-penalty",
        "设置频率惩罚 (-2.0-2.0)", "value");
    QCommandLineOption seedOpt(QStringList() << "seed",
        "设置随机种子 (整数，-1 表示清除)", "value");
    QCommandLineOption maxContextOpt(QStringList() << "max-context",
        "设置最大上下文消息数 (1-100)", "value");

    parser.addOption(sessionOpt);
    parser.addOption(newSessionOpt);
    parser.addOption(listOpt);
    parser.addOption(deleteOpt);
    parser.addOption(showOpt);
    parser.addOption(apiURLOpt);
    parser.addOption(apiKeyOpt);
    parser.addOption(modelOpt);
    parser.addOption(localModeOpt);
    parser.addOption(externalModeOpt);
    parser.addOption(showConfigOpt);
    parser.addOption(noStreamOpt);
    parser.addOption(temperatureOpt);
    parser.addOption(topPOpt);
    parser.addOption(maxTokensOpt);
    parser.addOption(presencePenaltyOpt);
    parser.addOption(frequencyPenaltyOpt);
    parser.addOption(seedOpt);
    parser.addOption(maxContextOpt);

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        printUsage();
        return 0;
    }

    if (parser.isSet("no-stream")) {
        m_networkManager->setStreamingEnabled(false);
    }

    SessionManager::instance()->loadSessionsFromFile();

    QString command = args[0].toLower();

    if (command == "chat") {
        return runInteractiveMode(app, parser);
    } else if (command == "ask") {
        if (args.size() < 2) {
            std::cerr << "错误: ask 命令需要提供问题内容" << std::endl;
            return 1;
        }
        return runSingleQuery(app, args.mid(1).join(" "), parser);
    } else if (command == "sessions") {
        return handleSessionsCommand(parser);
    } else if (command == "config") {
        return handleConfigCommand(parser);
    } else {
        std::cerr << "错误: 未知命令 '" << command.toStdString() << "'" << std::endl;
        printUsage();
        return 1;
    }
}

void CLIApplication::printUsage()
{
    std::cout << "\n本地AI助手 - 命令行版本 v1.0.0\n\n";
    std::cout << "用法:\n";
    std::cout << "  ai chat                    进入交互式对话模式\n";
    std::cout << "  ai ask <问题>              单次查询\n";
    std::cout << "  ai sessions [选项]         会话管理\n";
    std::cout << "  ai config [选项]           配置管理\n\n";
    std::cout << "会话管理选项:\n";
    std::cout << "  -l, --list                 列出所有会话\n";
    std::cout << "  -n, --new                  创建新会话\n";
    std::cout << "  -d, --delete <id>          删除指定会话\n";
    std::cout << "  --show <id>                显示会话内容\n";
    std::cout << "  -s, --session <id>         切换到指定会话\n\n";
    std::cout << "配置管理选项:\n";
    std::cout << "  --api-url <url>            设置API基础URL\n";
    std::cout << "  --api-key <key>            设置API密钥\n";
    std::cout << "  --model <name>             设置模型名称\n";
    std::cout << "  --local                    使用本地模式\n";
    std::cout << "  --external                 使用外部API模式\n";
    std::cout << "  --show-config              显示当前配置\n\n";
    std::cout << "模型参数选项:\n";
    std::cout << "  --temperature <value>      设置温度 (0.0-2.0)\n";
    std::cout << "  --top-p <value>            设置 top_p (0.0-1.0)\n";
    std::cout << "  --max-tokens <value>       设置最大输出 tokens (1-128000)\n";
    std::cout << "  --presence-penalty <value> 设置存在惩罚 (-2.0-2.0)\n";
    std::cout << "  --frequency-penalty <value> 设置频率惩罚 (-2.0-2.0)\n";
    std::cout << "  --seed <value>             设置随机种子 (整数，-1清除)\n";
    std::cout << "  --max-context <value>      设置最大上下文消息数 (1-100)\n\n";
    std::cout << "输出选项:\n";
    std::cout << "  --no-stream                禁用流式输出，等待完整响应\n\n";
    std::cout << "示例:\n";
    std::cout << "  ai chat                    # 启动交互式对话\n";
    std::cout << "  ai ask \"什么是量子计算?\"   # 单次查询\n";
    std::cout << "  ai ask \"你好\" --no-stream  # 禁用流式输出\n";
    std::cout << "  ai sessions -l             # 列出所有会话\n";
    std::cout << "  ai config --show-config    # 显示当前配置\n";
    std::cout << "  ai config --temperature 0.7 --top-p 0.9  # 设置模型参数\n";
    std::cout << std::endl;
}

int CLIApplication::runInteractiveMode(QCoreApplication &app, const QCommandLineParser &parser)
{
    m_interactiveMode = true;

    if (parser.isSet("session")) {
        QString sessionId = parser.value("session");
        SessionManager::instance()->switchToSession(sessionId);
    } else if (parser.isSet("new")) {
        SessionManager::instance()->createNewSession("CLI对话");
    }

    std::cout << "\n====================================\n";
    std::cout << "  本地AI助手 - 交互式对话模式\n";
    std::cout << "====================================\n";
    std::cout << "当前会话: " << SessionManager::instance()->currentSession().title.toStdString() << "\n";
    std::cout << "会话ID: " << SessionManager::instance()->currentSessionId().toStdString() << "\n";
    std::cout << "流式输出: " << (m_networkManager->isStreamingEnabled() ? "开启" : "关闭") << "\n";
    std::cout << "------------------------------------\n";
    std::cout << "命令:\n";
    std::cout << "  /help     - 显示帮助信息\n";
    std::cout << "  /new      - 创建新会话\n";
    std::cout << "  /list     - 列出所有会话\n";
    std::cout << "  /switch <id> - 切换会话\n";
    std::cout << "  /delete <id> - 删除会话\n";
    std::cout << "  /config   - 显示配置\n";
    std::cout << "  /stream   - 切换流式输出\n";
    std::cout << "  /clear    - 清屏\n";
    std::cout << "  /exit     - 退出程序\n";
    std::cout << "====================================\n\n";

    QTimer::singleShot(0, this, &CLIApplication::readInput);
    return app.exec();
}

void CLIApplication::readInput()
{
    std::cout << "\n你: ";
    std::cout.flush();

    std::string input;
    if (!std::getline(std::cin, input)) {
        quit();
        return;
    }

    QString qInput = QString::fromStdString(input).trimmed();

    if (qInput.isEmpty()) {
        QTimer::singleShot(0, this, &CLIApplication::readInput);
        return;
    }

    if (qInput.startsWith("/")) {
        handleCommand(qInput);
        return;
    }

    m_running = true;
    m_isStreaming = false;
    m_streamingContent.clear();
    SessionManager::instance()->addMessageToCurrentSession("user", qInput);

    if (m_networkManager->isStreamingEnabled()) {
        std::cout << "\nAI: " << std::flush;
    } else {
        std::cout << "\nAI正在思考..." << std::endl;
    }

    m_networkManager->sendChatRequestWithContext(
        SessionManager::instance()->currentSession().messages);
}

void CLIApplication::handleCommand(const QString &command)
{
    QString cmd = command.toLower();

    if (cmd == "/help") {
        showHelp();
    } else if (cmd == "/new") {
        SessionManager::instance()->createNewSession("CLI对话");
        std::cout << "已创建新会话: "
                  << SessionManager::instance()->currentSessionId().toStdString()
                  << std::endl;
    } else if (cmd == "/list") {
        listSessions();
    } else if (cmd.startsWith("/switch ")) {
        QString sessionId = command.mid(8).trimmed();
        SessionManager::instance()->switchToSession(sessionId);
        std::cout << "已切换到会话: "
                  << SessionManager::instance()->currentSession().title.toStdString()
                  << std::endl;
    } else if (cmd.startsWith("/delete ")) {
        QString sessionId = command.mid(8).trimmed();
        SessionManager::instance()->removeSession(sessionId);
        std::cout << "已删除会话: " << sessionId.toStdString() << std::endl;
    } else if (cmd == "/config") {
        showConfig();
    } else if (cmd == "/stream") {
        bool enabled = !m_networkManager->isStreamingEnabled();
        m_networkManager->setStreamingEnabled(enabled);
        std::cout << "流式输出: " << (enabled ? "已开启" : "已关闭") << std::endl;
    } else if (cmd == "/clear") {
        #ifdef Q_OS_WIN
            system("cls");
        #else
            system("clear");
        #endif
    } else if (cmd == "/exit" || cmd == "/quit") {
        quit();
        return;
    } else {
        std::cout << "未知命令: " << command.toStdString() << std::endl;
        std::cout << "输入 /help 查看可用命令" << std::endl;
    }

    QTimer::singleShot(0, this, &CLIApplication::readInput);
}

void CLIApplication::showHelp()
{
    std::cout << "\n可用命令:\n";
    std::cout << "  /help     - 显示此帮助信息\n";
    std::cout << "  /new      - 创建新会话\n";
    std::cout << "  /list     - 列出所有会话\n";
    std::cout << "  /switch <id> - 切换到指定会话\n";
    std::cout << "  /delete <id> - 删除指定会话\n";
    std::cout << "  /config   - 显示当前配置\n";
    std::cout << "  /stream   - 切换流式输出开/关\n";
    std::cout << "  /clear    - 清屏\n";
    std::cout << "  /exit     - 退出程序\n";
    std::cout << "\n直接输入文本即可与AI对话\n";
}

void CLIApplication::listSessions()
{
    const auto &sessions = SessionManager::instance()->allSessions();
    std::cout << "\n会话列表:\n";
    std::cout << "----------------------------------------\n";

    for (auto it = sessions.constBegin(); it != sessions.constEnd(); ++it) {
        QString current = (it.key() == SessionManager::instance()->currentSessionId())
                        ? " [当前]" : "";
        std::cout << "ID: " << it.key().toStdString() << current.toStdString() << "\n";
        std::cout << "标题: " << it.value().title.toStdString() << "\n";
        std::cout << "消息数: " << it.value().messages.size() << "\n";
        std::cout << "----------------------------------------\n";
    }
}

void CLIApplication::showConfig()
{
    QSettings settings("LocalAIAssistant", "Settings");
    std::cout << "\n当前配置:\n";
    std::cout << "----------------------------------------\n";
    std::cout << "模式: "
              << (settings.value("localMode", true).toBool() ? "本地模式" : "外部API模式")
              << "\n";
    std::cout << "API URL: "
              << settings.value("apiBaseUrl", "http://127.0.0.1:8080").toString().toStdString()
              << "\n";
    std::cout << "模型: "
              << settings.value("modelName", "local-model").toString().toStdString()
              << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << "模型参数:\n";
    std::cout << "  temperature: " << settings.value("temperature", 0.4).toDouble() << "\n";
    std::cout << "  top_p: " << settings.value("topP", 1.0).toDouble() << "\n";
    std::cout << "  max_tokens: " << settings.value("maxTokens", 8192).toInt() << "\n";
    std::cout << "  presence_penalty: " << settings.value("presencePenalty", 0.2).toDouble() << "\n";
    std::cout << "  frequency_penalty: " << settings.value("frequencyPenalty", 0.0).toDouble() << "\n";

    int seed = settings.value("seed", -1).toInt();
    if (seed >= 0) {
        std::cout << "  seed: " << seed << "\n";
    } else {
        std::cout << "  seed: 未设置 (null)\n";
    }

    std::cout << "  max_context: " << settings.value("maxContext", 10).toInt() << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << "流式输出: " << (m_networkManager->isStreamingEnabled() ? "开启" : "关闭") << "\n";
    std::cout << "----------------------------------------\n";
}

int CLIApplication::runSingleQuery(QCoreApplication &app, const QString &query, const QCommandLineParser &parser)
{
    m_interactiveMode = false;

    if (parser.isSet("session")) {
        QString sessionId = parser.value("session");
        SessionManager::instance()->switchToSession(sessionId);
    } else if (parser.isSet("new")) {
        SessionManager::instance()->createNewSession("单次查询");
    }

    SessionManager::instance()->addMessageToCurrentSession("user", query);
    m_running = true;
    m_isStreaming = false;
    m_streamingContent.clear();

    QTimer::singleShot(0, [this, &query]() {
        if (m_networkManager->isStreamingEnabled()) {
            std::cout << "AI: " << std::flush;
        }
        m_networkManager->sendChatRequestWithContext(
            SessionManager::instance()->currentSession().messages);
    });

    return app.exec();
}

int CLIApplication::handleSessionsCommand(const QCommandLineParser &parser)
{
    if (parser.isSet("list")) {
        listSessions();
    } else if (parser.isSet("new")) {
        SessionManager::instance()->createNewSession("CLI对话");
        std::cout << "已创建新会话: "
                  << SessionManager::instance()->currentSessionId().toStdString()
                  << std::endl;
    } else if (parser.isSet("delete")) {
        QString sessionId = parser.value("delete");
        SessionManager::instance()->removeSession(sessionId);
        std::cout << "已删除会话: " << sessionId.toStdString() << std::endl;
    } else if (parser.isSet("show")) {
        QString sessionId = parser.value("show");
        showSessionContent(sessionId);
    } else if (parser.isSet("session")) {
        QString sessionId = parser.value("session");
        SessionManager::instance()->switchToSession(sessionId);
        std::cout << "已切换到会话: " << sessionId.toStdString() << std::endl;
    } else {
        listSessions();
    }

    SessionManager::instance()->saveSessionsToFile();
    return 0;
}

void CLIApplication::showSessionContent(const QString &sessionId)
{
    const auto &sessions = SessionManager::instance()->allSessions();
    if (!sessions.contains(sessionId)) {
        std::cerr << "错误: 会话不存在" << std::endl;
        return;
    }

    const ChatSession &session = sessions[sessionId];
    std::cout << "\n会话: " << session.title.toStdString() << "\n";
    std::cout << "ID: " << session.id.toStdString() << "\n";
    std::cout << "========================================\n";

    for (const auto &msg : session.messages) {
        if (msg.role == "user") {
            std::cout << "\n[用户]: " << msg.content.toStdString() << "\n";
        } else {
            std::cout << "\n[AI]: " << msg.content.toStdString() << "\n";
        }
        std::cout << "----------------------------------------\n";
    }
}

namespace {
    bool validateDoubleParam(const std::string &name, double value, double min, double max) {
        if (value < min || value > max) {
            std::cerr << "错误: " << name << " 值 " << value
                      << " 超出范围 [" << min << ", " << max << "]" << std::endl;
            return false;
        }
        return true;
    }

    bool validateIntParam(const std::string &name, int value, int min, int max) {
        if (value < min || value > max) {
            std::cerr << "错误: " << name << " 值 " << value
                      << " 超出范围 [" << min << ", " << max << "]" << std::endl;
            return false;
        }
        return true;
    }
}

int CLIApplication::handleConfigCommand(const QCommandLineParser &parser)
{
    QSettings settings("LocalAIAssistant", "Settings");

    bool hasChanges = false;
    bool validationFailed = false;
    QString apiUrl = settings.value("apiBaseUrl", "http://127.0.0.1:8080").toString();
    QString apiKey = settings.value("apiKey", "").toString();
    QString modelName = settings.value("modelName", "local-model").toString();
    bool isLocalMode = settings.value("localMode", true).toBool();

    if (parser.isSet("api-url")) {
        apiUrl = parser.value("api-url");
        hasChanges = true;
    }
    if (parser.isSet("api-key")) {
        apiKey = parser.value("api-key");
        hasChanges = true;
    }
    if (parser.isSet("model")) {
        modelName = parser.value("model");
        hasChanges = true;
    }
    if (parser.isSet("local")) {
        isLocalMode = true;
        hasChanges = true;
    }
    if (parser.isSet("external")) {
        isLocalMode = false;
        hasChanges = true;
    }

    // 处理模型参数（新增）
    if (parser.isSet("temperature")) {
        double temp = parser.value("temperature").toDouble();
        if (!validateDoubleParam("temperature", temp, 0.0, 2.0)) {
            validationFailed = true;
        }
    }

    if (parser.isSet("top-p")) {
        double topP = parser.value("top-p").toDouble();
        if (!validateDoubleParam("top_p", topP, 0.0, 1.0)) {
            validationFailed = true;
        }
    }

    if (parser.isSet("max-tokens")) {
        int maxTokens = parser.value("max-tokens").toInt();
        if (!validateIntParam("max_tokens", maxTokens, 1, 128000)) {
            validationFailed = true;
        }
    }

    if (parser.isSet("presence-penalty")) {
        double penalty = parser.value("presence-penalty").toDouble();
        if (!validateDoubleParam("presence_penalty", penalty, -2.0, 2.0)) {
            validationFailed = true;
        }
    }

    if (parser.isSet("frequency-penalty")) {
        double penalty = parser.value("frequency-penalty").toDouble();
        if (!validateDoubleParam("frequency_penalty", penalty, -2.0, 2.0)) {
            validationFailed = true;
        }
    }

    if (parser.isSet("max-context")) {
        int maxContext = parser.value("max-context").toInt();
        if (!validateIntParam("max_context", maxContext, 1, 100)) {
            validationFailed = true;
        }
    }

    if (validationFailed) {
        std::cerr << "配置未更新，请修正参数值后重试" << std::endl;
        return 1;
    }

    // 先更新 API 设置（会触发 NetworkManager 保存其缓存值）
    if (hasChanges) {
        m_networkManager->updateSettings(apiUrl, apiKey, modelName, isLocalMode);
    }

    // 然后设置模型参数（覆盖 NetworkManager 保存的旧值）
    if (parser.isSet("temperature")) {
        double temp = parser.value("temperature").toDouble();
        settings.setValue("temperature", temp);
    }

    if (parser.isSet("top-p")) {
        double topP = parser.value("top-p").toDouble();
        settings.setValue("topP", topP);
    }

    if (parser.isSet("max-tokens")) {
        int maxTokens = parser.value("max-tokens").toInt();
        settings.setValue("maxTokens", maxTokens);
    }

    if (parser.isSet("presence-penalty")) {
        double penalty = parser.value("presence-penalty").toDouble();
        settings.setValue("presencePenalty", penalty);
    }

    if (parser.isSet("frequency-penalty")) {
        double penalty = parser.value("frequency-penalty").toDouble();
        settings.setValue("frequencyPenalty", penalty);
    }

    if (parser.isSet("seed")) {
        int seed = parser.value("seed").toInt();
        settings.setValue("seed", seed);
    }

    if (parser.isSet("max-context")) {
        int maxContext = parser.value("max-context").toInt();
        settings.setValue("maxContext", maxContext);
    }

    settings.sync();

    if (hasChanges) {
        std::cout << "配置已更新" << std::endl;
    }

    if (parser.isSet("show-config") || !hasChanges) {
        showConfig();
    }

    return 0;
}

void CLIApplication::onStreamChunkReceived(const QString &chunk)
{
    if (!m_isStreaming) {
        m_isStreaming = true;
    }

    m_streamingContent += chunk;
    std::cout << chunk.toStdString() << std::flush;
}

void CLIApplication::onStreamFinished(const QString &fullContent)
{
    if (m_isStreaming) {
        std::cout << std::endl;
        SessionManager::instance()->addMessageToCurrentSession("assistant", fullContent);
        SessionManager::instance()->saveSessionsToFile();
        m_isStreaming = false;
        m_streamingContent.clear();
    }

    if (m_interactiveMode) {
        m_running = false;
        QTimer::singleShot(0, this, &CLIApplication::readInput);
    } else {
        quit();
    }
}

void CLIApplication::onResponseReceived(const QString &response)
{
    if (m_isStreaming) {
        return;
    }

    SessionManager::instance()->addMessageToCurrentSession("assistant", response);
    SessionManager::instance()->saveSessionsToFile();

    if (m_interactiveMode) {
        std::cout << "\nAI: " << response.toStdString() << std::endl;
        m_running = false;
        QTimer::singleShot(0, this, &CLIApplication::readInput);
    } else {
        std::cout << response.toStdString() << std::endl;
        quit();
    }
}

void CLIApplication::onErrorOccurred(const QString &error)
{
    m_isStreaming = false;
    m_streamingContent.clear();

    SessionManager::instance()->addMessageToCurrentSession("assistant", "错误: " + error);
    SessionManager::instance()->saveSessionsToFile();

    if (m_interactiveMode) {
        std::cerr << "\n错误: " << error.toStdString() << std::endl;
        m_running = false;
        QTimer::singleShot(0, this, &CLIApplication::readInput);
    } else {
        std::cerr << "错误: " << error.toStdString() << std::endl;
        quit();
    }
}

void CLIApplication::quit()
{
    SessionManager::instance()->saveSessionsToFile();
    QCoreApplication::quit();
}
