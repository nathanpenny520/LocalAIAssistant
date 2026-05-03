#include "cli_application.h"
#include "filemanager.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>
#include <QSettings>
#include <QFileInfo>
#include <iostream>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "networkmanager.h"
#include "sessionmanager.h"

namespace {
#ifdef Q_OS_WIN
// Initialize Windows console for UTF-8 support
// Returns true if successful, false otherwise
bool initWindowsConsole()
{
    // Save original console modes for restoration if needed
    static DWORD originalOutputMode = 0;
    static DWORD originalInputMode = 0;
    static bool initialized = false;

    if (initialized) {
        return true;
    }

    // Set console code page to UTF-8
    if (!SetConsoleOutputCP(CP_UTF8)) {
        std::cerr << "Warning: Failed to set console output code page to UTF-8" << std::endl;
    }

    if (!SetConsoleCP(CP_UTF8)) {
        std::cerr << "Warning: Failed to set console input code page to UTF-8" << std::endl;
    }

    // Enable virtual terminal processing for ANSI escape sequences (colors, clear screen, etc.)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

    if (hOut != INVALID_HANDLE_VALUE) {
        GetConsoleMode(hOut, &originalOutputMode);
        DWORD newMode = originalOutputMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, newMode);
    }

    if (hIn != INVALID_HANDLE_VALUE) {
        GetConsoleMode(hIn, &originalInputMode);
        // Enable line input and echo for proper input handling
        DWORD newMode = originalInputMode | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT;
        SetConsoleMode(hIn, newMode);
    }

    initialized = true;
    return true;
}
#endif
}

CLIApplication::CLIApplication(QObject *parent)
    : QObject(parent)
    , m_networkManager(nullptr)
    , m_fileManager(nullptr)
    , m_running(false)
    , m_interactiveMode(false)
    , m_isStreaming(false)
{
}

int CLIApplication::run(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // Initialize Windows console for UTF-8 support (must be before any I/O)
    initWindowsConsole();
#endif

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("LocalAIAssistant");
    QCoreApplication::setApplicationVersion("1.0.0");

    m_networkManager = new NetworkManager(this);
    m_fileManager = new FileManager(this);
    connect(m_networkManager, &NetworkManager::responseReceived,
            this, &CLIApplication::onResponseReceived);
    connect(m_networkManager, &NetworkManager::errorOccurred,
            this, &CLIApplication::onErrorOccurred);
    connect(m_networkManager, &NetworkManager::streamChunkReceived,
            this, &CLIApplication::onStreamChunkReceived);
    connect(m_networkManager, &NetworkManager::streamFinished,
            this, &CLIApplication::onStreamFinished);

    QCommandLineParser parser;
    parser.setApplicationDescription("\nLocalAIAssistant - CLI Version\n"
                                     "Supports interactive chat and single query modes");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("command",
        "Command: chat (interactive) | ask <question> (single query) | sessions (session management) | config (config management)");

    QCommandLineOption sessionOpt(QStringList() << "s" << "session",
        "Specify session ID for chat", "session-id");
    QCommandLineOption newSessionOpt(QStringList() << "n" << "new",
        "Create new session");
    QCommandLineOption listOpt(QStringList() << "l" << "list",
        "List all sessions");
    QCommandLineOption deleteOpt(QStringList() << "d" << "delete",
        "Delete specified session", "session-id");
    QCommandLineOption showOpt(QStringList() << "show",
        "Show session content", "session-id");
    QCommandLineOption apiURLOpt(QStringList() << "api-url",
        "Set API base URL", "url");
    QCommandLineOption apiKeyOpt(QStringList() << "api-key",
        "Set API key", "key");
    QCommandLineOption modelOpt(QStringList() << "model",
        "Set model name", "name");
    QCommandLineOption localModeOpt(QStringList() << "local",
        "Use local mode");
    QCommandLineOption externalModeOpt(QStringList() << "external",
        "Use external API mode");
    QCommandLineOption showConfigOpt(QStringList() << "show-config",
        "Show current config");
    QCommandLineOption noStreamOpt(QStringList() << "no-stream",
        "Disable streaming output");
    QCommandLineOption temperatureOpt(QStringList() << "temperature",
        "Set temperature (0.0-2.0)", "value");
    QCommandLineOption topPOpt(QStringList() << "top-p",
        "Set top_p (0.0-1.0)", "value");
    QCommandLineOption maxTokensOpt(QStringList() << "max-tokens",
        "Set max output tokens (1-128000)", "value");
    QCommandLineOption presencePenaltyOpt(QStringList() << "presence-penalty",
        "Set presence penalty (-2.0-2.0)", "value");
    QCommandLineOption frequencyPenaltyOpt(QStringList() << "frequency-penalty",
        "Set frequency penalty (-2.0-2.0)", "value");
    QCommandLineOption seedOpt(QStringList() << "seed",
        "Set random seed (integer, -1 to clear)", "value");
    QCommandLineOption maxContextOpt(QStringList() << "max-context",
        "Set max context messages (1-100)", "value");

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
        // No arguments: default to interactive chat mode (for double-click launch)
        return runInteractiveMode(app, parser);
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
            std::cerr << "Error: ask command requires a question" << std::endl;
            return 1;
        }
        return runSingleQuery(app, args.mid(1).join(" "), parser);
    } else if (command == "sessions") {
        return handleSessionsCommand(parser);
    } else if (command == "config") {
        return handleConfigCommand(parser);
    } else {
        std::cerr << "Error: Unknown command '" << command.toStdString() << "'" << std::endl;
        printUsage();
        return 1;
    }
}

void CLIApplication::printUsage()
{
    std::cout << "\nLocalAIAssistant - CLI v1.0.0\n\n";
    std::cout << "Usage (ai is alias for ./LocalAIAssistant-CLI):\n";
    std::cout << "  ai chat                    Enter interactive chat mode\n";
    std::cout << "  ai ask <question>          Single query\n";
    std::cout << "  ai sessions [options]      Session management\n";
    std::cout << "  ai config [options]        Config management\n\n";
    std::cout << "Session options:\n";
    std::cout << "  -l, --list                 List all sessions\n";
    std::cout << "  -n, --new                  Create new session\n";
    std::cout << "  -d, --delete <id>          Delete specified session\n";
    std::cout << "  --show <id>                Show session content\n";
    std::cout << "  -s, --session <id>         Switch to specified session\n\n";
    std::cout << "Config options:\n";
    std::cout << "  --api-url <url>            Set API base URL\n";
    std::cout << "  --api-key <key>            Set API key\n";
    std::cout << "  --model <name>             Set model name\n";
    std::cout << "  --local                    Use local mode\n";
    std::cout << "  --external                 Use external API mode\n";
    std::cout << "  --show-config              Show current config\n\n";
    std::cout << "Model parameter options:\n";
    std::cout << "  --temperature <value>      Set temperature (0.0-2.0)\n";
    std::cout << "  --top-p <value>            Set top_p (0.0-1.0)\n";
    std::cout << "  --max-tokens <value>       Set max output tokens (1-128000)\n";
    std::cout << "  --presence-penalty <value> Set presence penalty (-2.0-2.0)\n";
    std::cout << "  --frequency-penalty <value> Set frequency penalty (-2.0-2.0)\n";
    std::cout << "  --seed <value>             Set random seed (integer, -1 to clear)\n";
    std::cout << "  --max-context <value>      Set max context messages (1-100)\n\n";
    std::cout << "Output options:\n";
    std::cout << "  --no-stream                Disable streaming output\n\n";
    std::cout << "Examples:\n";
    std::cout << "  ai chat                    # Start interactive chat\n";
    std::cout << "  ai ask \"What is quantum computing?\"   # Single query\n";
    std::cout << "  ai ask \"Hello\" --no-stream  # Disable streaming\n";
    std::cout << "  ai sessions -l             # List all sessions\n";
    std::cout << "  ai config --show-config    # Show current config\n";
    std::cout << "  ai config --temperature 0.7 --top-p 0.9  # Set model params\n";
    std::cout << std::endl;
}

int CLIApplication::runInteractiveMode(QCoreApplication &app, const QCommandLineParser &parser)
{
    m_interactiveMode = true;

    if (parser.isSet("session")) {
        QString sessionId = parser.value("session");
        SessionManager::instance()->switchToSession(sessionId);
    } else if (parser.isSet("new")) {
        QSettings settings("LocalAIAssistant", "Settings");
    QString locale = settings.value("language", "zh_CN").toString();
    QString defaultTitle = (locale == "en") ? "CLI Chat" : QStringLiteral("CLI 对话");
    SessionManager::instance()->createNewSession(defaultTitle);
    }

    std::cout << "\n====================================\n";
    std::cout << "  LocalAIAssistant - Interactive Mode\n";
    std::cout << "====================================\n";
    std::cout << "Current session: " << SessionManager::instance()->currentSession().title.toStdString() << "\n";
    std::cout << "Session ID: " << SessionManager::instance()->currentSessionId().toStdString() << "\n";
    std::cout << "Streaming: " << (m_networkManager->isStreamingEnabled() ? "ON" : "OFF") << "\n";
    std::cout << "------------------------------------\n";
    std::cout << "Commands:\n";
    std::cout << "  /help     - Show help\n";
    std::cout << "  /new      - Create new session\n";
    std::cout << "  /list     - List all sessions\n";
    std::cout << "  /switch <id> - Switch session\n";
    std::cout << "  /delete <id> - Delete session\n";
    std::cout << "  /config   - Show config\n";
    std::cout << "  /stream   - Toggle streaming\n";
    std::cout << "  /clear    - Clear screen\n";
    std::cout << "  /file <path> - Add file to pending list\n";
    std::cout << "  /listfiles   - Show pending files\n";
    std::cout << "  /clearfiles  - Clear pending files\n";
    std::cout << "  /search <keyword> - Search messages\n";
    std::cout << "  /exit     - Exit program\n";
    std::cout << "====================================\n\n";

    QTimer::singleShot(0, this, &CLIApplication::readInput);
    return app.exec();
}

void CLIApplication::readInput()
{
    std::cout << "\nYou: ";
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

    // Create message with attachments
    QVector<FileAttachment> attachments;
    if (m_fileManager->pendingFileCount() > 0) {
        attachments = m_fileManager->pendingFiles();
        std::cout << "Sending message with " << attachments.size() << " file(s)" << std::endl;
    }

    // Save message with attachments
    if (attachments.isEmpty()) {
        SessionManager::instance()->addMessageToCurrentSession("user", qInput);
    } else {
        SessionManager::instance()->addMessageToCurrentSession("user", qInput, attachments);
        m_fileManager->clearPendingFiles();
    }

    if (m_networkManager->isStreamingEnabled()) {
        std::cout << "\nAI: " << std::flush;
    } else {
        std::cout << "\nAI is thinking..." << std::endl;
    }

    // Build message list and send
    QVector<ChatMessage> messages = SessionManager::instance()->currentSession().messages;

    m_networkManager->sendChatRequestWithContext(messages);
}

void CLIApplication::handleCommand(const QString &command)
{
    QString cmd = command.toLower();

    if (cmd == "/help") {
        showHelp();
    } else if (cmd == "/new") {
        QSettings settings("LocalAIAssistant", "Settings");
    QString locale = settings.value("language", "zh_CN").toString();
    QString defaultTitle = (locale == "en") ? "CLI Chat" : QStringLiteral("CLI 对话");
    SessionManager::instance()->createNewSession(defaultTitle);
        std::cout << "Created new session: "
                  << SessionManager::instance()->currentSessionId().toStdString()
                  << std::endl;
    } else if (cmd == "/list") {
        listSessions();
    } else if (cmd.startsWith("/switch ")) {
        QString sessionId = command.mid(8).trimmed();
        SessionManager::instance()->switchToSession(sessionId);
        std::cout << "Switched to session: "
                  << SessionManager::instance()->currentSession().title.toStdString()
                  << std::endl;
    } else if (cmd.startsWith("/delete ")) {
        QString sessionId = command.mid(8).trimmed();
        SessionManager::instance()->removeSession(sessionId);
        std::cout << "Deleted session: " << sessionId.toStdString() << std::endl;
    } else if (cmd == "/config") {
        showConfig();
    } else if (cmd == "/stream") {
        bool enabled = !m_networkManager->isStreamingEnabled();
        m_networkManager->setStreamingEnabled(enabled);
        std::cout << "Streaming: " << (enabled ? "ON" : "OFF") << std::endl;
    } else if (cmd == "/clear") {
        // Use ANSI escape sequence for cross-platform clear screen
        std::cout << "\033[2J\033[H" << std::flush;
    } else if (cmd.startsWith("/file ")) {
        handleFileCommand(command);
        return;
    } else if (cmd == "/listfiles") {
        listFiles();
        return;  // Fix: prevent double readInput scheduling
    } else if (cmd == "/clearfiles") {
        m_fileManager->clearPendingFiles();
        std::cout << "Cleared pending file list" << std::endl;
    } else if (cmd.startsWith("/search ")) {
        QString keyword = command.mid(8).trimmed();
        searchMessages(keyword);
    } else if (cmd == "/search") {
        std::cout << "Usage: /search <keyword>" << std::endl;
        std::cout << "Example: /search hello" << std::endl;
    } else if (cmd == "/exit" || cmd == "/quit") {
        quit();
        return;
    } else {
        std::cout << "Unknown command: " << command.toStdString() << std::endl;
        std::cout << "Type /help for available commands" << std::endl;
    }

    QTimer::singleShot(0, this, &CLIApplication::readInput);
}

void CLIApplication::showHelp()
{
    std::cout << "\nAvailable commands:\n";
    std::cout << "  /help     - Show this help\n";
    std::cout << "  /new      - Create new session\n";
    std::cout << "  /list     - List all sessions\n";
    std::cout << "  /switch <id> - Switch to session\n";
    std::cout << "  /delete <id> - Delete session\n";
    std::cout << "  /config   - Show current config\n";
    std::cout << "  /stream   - Toggle streaming ON/OFF\n";
    std::cout << "  /clear    - Clear screen\n";
    std::cout << "  /file <path> - Add file to pending list\n";
    std::cout << "  /listfiles   - Show pending files\n";
    std::cout << "  /clearfiles  - Clear pending files\n";
    std::cout << "  /search <keyword> - Search messages in current session\n";
    std::cout << "  /exit     - Exit program\n";
    std::cout << "\nJust type text to chat with AI\n";
}

void CLIApplication::listSessions()
{
    const auto &sessions = SessionManager::instance()->allSessions();
    std::cout << "\nSession list:\n";
    std::cout << "----------------------------------------\n";

    for (auto it = sessions.constBegin(); it != sessions.constEnd(); ++it) {
        QString current = (it.key() == SessionManager::instance()->currentSessionId())
                        ? " [current]" : "";
        std::cout << "ID: " << it.key().toStdString() << current.toStdString() << "\n";
        std::cout << "Title: " << it.value().title.toStdString() << "\n";
        std::cout << "Messages: " << it.value().messages.size() << "\n";
        std::cout << "----------------------------------------\n";
    }
}

void CLIApplication::showConfig()
{
    QSettings settings("LocalAIAssistant", "Settings");
    std::cout << "\nCurrent config:\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Mode: "
              << (settings.value("localMode", true).toBool() ? "Local mode" : "External API mode")
              << "\n";
    std::cout << "API URL: "
              << settings.value("apiBaseUrl", "http://127.0.0.1:8080").toString().toStdString()
              << "\n";
    std::cout << "Model: "
              << settings.value("modelName", "local-model").toString().toStdString()
              << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Model parameters:\n";
    std::cout << "  temperature: " << settings.value("temperature", 0.4).toDouble() << "\n";
    std::cout << "  top_p: " << settings.value("topP", 1.0).toDouble() << "\n";
    std::cout << "  max_tokens: " << settings.value("maxTokens", 8192).toInt() << "\n";
    std::cout << "  presence_penalty: " << settings.value("presencePenalty", 0.2).toDouble() << "\n";
    std::cout << "  frequency_penalty: " << settings.value("frequencyPenalty", 0.0).toDouble() << "\n";

    int seed = settings.value("seed", -1).toInt();
    if (seed >= 0) {
        std::cout << "  seed: " << seed << "\n";
    } else {
        std::cout << "  seed: not set (null)\n";
    }

    std::cout << "  max_context: " << settings.value("maxContext", 10).toInt() << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Streaming: " << (m_networkManager->isStreamingEnabled() ? "ON" : "OFF") << "\n";
    std::cout << "----------------------------------------\n";
}

int CLIApplication::runSingleQuery(QCoreApplication &app, const QString &query, const QCommandLineParser &parser)
{
    m_interactiveMode = false;

    if (parser.isSet("session")) {
        QString sessionId = parser.value("session");
        SessionManager::instance()->switchToSession(sessionId);
    } else if (parser.isSet("new")) {
        QSettings settings("LocalAIAssistant", "Settings");
        QString locale = settings.value("language", "zh_CN").toString();
        QString singleQueryTitle = (locale == "en") ? "Single Query" : QStringLiteral("单次查询");
        SessionManager::instance()->createNewSession(singleQueryTitle);
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
        QSettings settings("LocalAIAssistant", "Settings");
    QString locale = settings.value("language", "zh_CN").toString();
    QString defaultTitle = (locale == "en") ? "CLI Chat" : QStringLiteral("CLI 对话");
    SessionManager::instance()->createNewSession(defaultTitle);
        std::cout << "Created new session: "
                  << SessionManager::instance()->currentSessionId().toStdString()
                  << std::endl;
    } else if (parser.isSet("delete")) {
        QString sessionId = parser.value("delete");
        SessionManager::instance()->removeSession(sessionId);
        std::cout << "Deleted session: " << sessionId.toStdString() << std::endl;
    } else if (parser.isSet("show")) {
        QString sessionId = parser.value("show");
        showSessionContent(sessionId);
    } else if (parser.isSet("session")) {
        QString sessionId = parser.value("session");
        SessionManager::instance()->switchToSession(sessionId);
        std::cout << "Switched to session: " << sessionId.toStdString() << std::endl;
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
        std::cerr << "Error: Session not found" << std::endl;
        return;
    }

    const ChatSession &session = sessions[sessionId];
    std::cout << "\nSession: " << session.title.toStdString() << "\n";
    std::cout << "ID: " << session.id.toStdString() << "\n";
    std::cout << "========================================\n";

    for (const auto &msg : session.messages) {
        if (msg.role == "user") {
            std::cout << "\n[User]: " << msg.content.toStdString() << "\n";
        } else {
            std::cout << "\n[AI]: " << msg.content.toStdString() << "\n";
        }
        std::cout << "----------------------------------------\n";
    }
}

namespace {
    bool validateDoubleParam(const std::string &name, double value, double min, double max) {
        if (value < min || value > max) {
            std::cerr << "Error: " << name << " value " << value
                      << " out of range [" << min << ", " << max << "]" << std::endl;
            return false;
        }
        return true;
    }

    bool validateIntParam(const std::string &name, int value, int min, int max) {
        if (value < min || value > max) {
            std::cerr << "Error: " << name << " value " << value
                      << " out of range [" << min << ", " << max << "]" << std::endl;
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
        std::cerr << "Config not updated, please fix parameter values and retry" << std::endl;
        return 1;
    }

    // Update API settings first (triggers NetworkManager to save cached values)
    if (hasChanges) {
        m_networkManager->updateSettings(apiUrl, apiKey, modelName, isLocalMode);
    }

    // Then set model parameters (override NetworkManager saved old values)
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
        std::cout << "Config updated" << std::endl;
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

    SessionManager::instance()->addMessageToCurrentSession("assistant", "Error: " + error);
    SessionManager::instance()->saveSessionsToFile();

    if (m_interactiveMode) {
        std::cerr << "\nError: " << error.toStdString() << std::endl;
        m_running = false;
        QTimer::singleShot(0, this, &CLIApplication::readInput);
    } else {
        std::cerr << "Error: " << error.toStdString() << std::endl;
        quit();
    }
}

void CLIApplication::handleFileCommand(const QString &command)
{
    // Extract file path (supports spaces: wrap with quotes)
    QString arg = command.mid(6).trimmed();

    if (arg.isEmpty()) {
        std::cout << "Usage: /file <file_path>" << std::endl;
        std::cout << "Tip: Use quotes for paths with spaces, e.g. /file \"path with space/file.txt\"" << std::endl;
        QTimer::singleShot(0, this, &CLIApplication::readInput);
        return;
    }

    // Parse path (handle quotes)
    QString filePath = arg;
    if (filePath.startsWith('"') && filePath.endsWith('"')) {
        filePath = filePath.mid(1, filePath.length() - 2);
    }

    // Add file
    QFileInfo info(filePath);
    if (!info.exists()) {
        std::cout << "Error: File not found: " << filePath.toStdString() << std::endl;
        QTimer::singleShot(0, this, &CLIApplication::readInput);
        return;
    }

    if (info.size() > 10 * 1024 * 1024) {
        std::cout << "Error: File too large (>10MB): " << filePath.toStdString() << std::endl;
        QTimer::singleShot(0, this, &CLIApplication::readInput);
        return;
    }

    if (m_fileManager->addFile(filePath)) {
        QString typeStr;
        if (FileManager::isTextFile(filePath)) {
            typeStr = "text";
        } else if (FileManager::isImageFile(filePath)) {
            typeStr = "image";
        } else {
            typeStr = "binary";
        }
        std::cout << "Added file: " << filePath.toStdString()
                  << " (" << typeStr.toStdString() << ", "
                  << info.size() << " bytes)" << std::endl;
        std::cout << "Pending files: " << m_fileManager->pendingFileCount() << std::endl;
    } else {
        std::cout << "Error: Cannot add file" << std::endl;
    }

    QTimer::singleShot(0, this, &CLIApplication::readInput);
}

void CLIApplication::listFiles()
{
    QString summary = m_fileManager->fileListSummary();
    std::cout << summary.toStdString() << std::endl;
    QTimer::singleShot(0, this, &CLIApplication::readInput);
}

void CLIApplication::searchMessages(const QString &keyword)
{
    if (keyword.isEmpty()) {
        std::cout << "Error: Search keyword is empty" << std::endl;
        std::cout << "Usage: /search <keyword>" << std::endl;
        QTimer::singleShot(0, this, &CLIApplication::readInput);
        return;
    }

    const ChatSession &session = SessionManager::instance()->currentSession();
    const QVector<ChatMessage> &messages = session.messages;

    if (messages.isEmpty()) {
        std::cout << "No messages in current session" << std::endl;
        QTimer::singleShot(0, this, &CLIApplication::readInput);
        return;
    }

    std::cout << "\nSearching for: \"" << keyword.toStdString() << "\"\n";
    std::cout << "========================================\n";

    int matchCount = 0;
    int messageIndex = 0;

    for (const ChatMessage &msg : messages) {
        messageIndex++;
        QString roleLabel = (msg.role == "user") ? "[User]" : "[AI]";

        // 搜索消息内容
        if (msg.content.contains(keyword, Qt::CaseInsensitive)) {
            matchCount++;
            std::cout << "\nMessage #" << messageIndex << " " << roleLabel.toStdString() << "\n";

            // 显示匹配上下文（截取包含关键词的部分）
            QString content = msg.content;
            int keywordPos = content.indexOf(keyword, 0, Qt::CaseInsensitive);

            if (keywordPos != -1) {
                // 显示前后各50个字符的上下文
                int contextStart = qMax(0, keywordPos - 50);
                int contextEnd = qMin(content.length(), keywordPos + keyword.length() + 50);
                QString context = content.mid(contextStart, contextEnd - contextStart);

                // 添加省略号指示
                if (contextStart > 0) {
                    context = "..." + context;
                }
                if (contextEnd < content.length()) {
                    context = context + "...";
                }

                std::cout << context.toStdString() << "\n";
            }
        }

        // 搜索附件内容（如果有）
        for (const FileAttachment &attachment : msg.attachments) {
            if (attachment.type == "text" && attachment.content.contains(keyword, Qt::CaseInsensitive)) {
                matchCount++;
                std::cout << "\nMessage #" << messageIndex << " " << roleLabel.toStdString()
                          << " [Attachment: " << attachment.path.toStdString() << "]\n";

                // 显示匹配上下文
                int keywordPos = attachment.content.indexOf(keyword, 0, Qt::CaseInsensitive);
                if (keywordPos != -1) {
                    int contextStart = qMax(0, keywordPos - 30);
                    int contextEnd = qMin(attachment.content.length(), keywordPos + keyword.length() + 30);
                    QString context = attachment.content.mid(contextStart, contextEnd - contextStart);

                    if (contextStart > 0) context = "..." + context;
                    if (contextEnd < attachment.content.length()) context = context + "...";

                    std::cout << context.toStdString() << "\n";
                }
            }
        }
    }

    std::cout << "\n========================================\n";
    if (matchCount == 0) {
        std::cout << "No matches found\n";
    } else {
        std::cout << "Found " << matchCount << " match(es)\n";
    }

    QTimer::singleShot(0, this, &CLIApplication::readInput);
}

void CLIApplication::quit()
{
    SessionManager::instance()->saveSessionsToFile();
    QCoreApplication::quit();
}
