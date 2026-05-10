#include "networkmanager.h"

#include <QDebug>
#include <QSettings>

#include "anthropic_provider.h"
#include "apiprovider.h"
#include "llamacpp_provider.h"
#include "ollama_provider.h"
#include "openai_provider.h"

NetworkManager::NetworkManager(QObject* parent)
        : QObject(parent)
        , m_provider(nullptr)
        , m_apiType(ApiType::OpenAI)
        , m_apiBaseUrl("http://127.0.0.1:8080")
        , m_apiKey()
        , m_modelName("local-model")
        , m_temperature(0.4)
        , m_topP(1.0)
        , m_maxContext(20)
        , m_maxTokens(8192)
        , m_presencePenalty(0.2)
        , m_frequencyPenalty(0.0)
        , m_seed(std::nullopt)
        , m_streamingEnabled(true) {
    loadSettings();
    ensureProvider(m_apiType);
    applySettingsToProvider();
}

NetworkManager::~NetworkManager() {
    if (m_provider) {
        m_provider->disconnect(this);
        m_provider->deleteLater();
        m_provider = nullptr;
    }
}

// --- Public API (delegates to provider) ---

void NetworkManager::sendChatRequest(const QString& userMessage) {
    QVector<ChatMessage> singleMessage;
    singleMessage.append(ChatMessage("user", userMessage));
    sendChatRequestWithContext(singleMessage);
}

void NetworkManager::sendChatRequestWithContext(const QVector<ChatMessage>& messages) {
    if (m_provider) m_provider->sendChatRequest(messages);
}

void NetworkManager::abortCurrentRequest() {
    if (m_provider) m_provider->abortCurrentRequest();
}

bool NetworkManager::isStreamingEnabled() const {
    return m_provider ? m_provider->isStreamingEnabled() : m_streamingEnabled;
}

void NetworkManager::setStreamingEnabled(bool enabled) {
    m_streamingEnabled = enabled;
    if (m_provider) m_provider->setStreamingEnabled(enabled);
    QSettings settings("LocalAIAssistant", "Settings");
    settings.setValue("streamingEnabled", enabled);
}

void NetworkManager::setSystemPrompt(const QString& prompt) {
    if (m_provider) m_provider->setSystemPrompt(prompt);
}

void NetworkManager::setKnowledgeContext(const QString& context) {
    if (m_provider) m_provider->setKnowledgeContext(context);
}

void NetworkManager::setApiType(ApiType type) {
    if (m_apiType != type) {
        m_apiType = type;
        ensureProvider(type);
        applySettingsToProvider();
        saveSettings();
    }
}

// --- Settings ---

void NetworkManager::updateSettings(const QString& apiBaseUrl, const QString& apiKey,
                                    const QString& modelName, ApiType apiType) {
    // Auto-detect Ollama via port 11434 for backward compatibility
    if (apiType == ApiType::OpenAI && apiBaseUrl.contains(QStringLiteral("11434")))
        apiType = ApiType::Ollama;

    m_apiBaseUrl = apiBaseUrl.trimmed();
    m_apiKey = apiKey.trimmed();
    m_modelName = modelName.trimmed();
    m_apiType = apiType;

    saveSettings();
    ensureProvider(apiType);
    applySettingsToProvider();
}

void NetworkManager::loadSettings() {
    QSettings settings("LocalAIAssistant", "Settings");

    m_apiBaseUrl = settings.value("apiBaseUrl", "http://127.0.0.1:8080").toString().trimmed();
    m_apiKey = settings.value("apiKey", "").toString().trimmed();
    m_modelName = settings.value("modelName", "local-model").toString().trimmed();

    m_temperature = settings.value("temperature", 0.4).toDouble();
    m_maxContext = settings.value("maxContext", 20).toInt();
    m_maxTokens = settings.value("maxTokens", 8192).toInt();
    m_presencePenalty = settings.value("presencePenalty", 0.2).toDouble();
    m_topP = settings.value("topP", 1.0).toDouble();
    m_frequencyPenalty = settings.value("frequencyPenalty", 0.0).toDouble();

    QString apiTypeStr = settings.value("apiType", "openai").toString().toLower();
    if (apiTypeStr == "ollama")
        m_apiType = ApiType::Ollama;
    else if (apiTypeStr == "llamacpp")
        m_apiType = ApiType::LlamaCpp;
    else if (apiTypeStr == "anthropic")
        m_apiType = ApiType::Anthropic;
    else
        m_apiType = ApiType::OpenAI;

    int seedValue = settings.value("seed", -1).toInt();
    m_seed = (seedValue >= 0) ? std::optional<int>(seedValue) : std::nullopt;

    m_streamingEnabled = settings.value("streamingEnabled", true).toBool();
}

void NetworkManager::saveSettings() {
    QSettings settings("LocalAIAssistant", "Settings");

    settings.setValue("apiBaseUrl", m_apiBaseUrl);
    settings.setValue("apiKey", m_apiKey);
    settings.setValue("modelName", m_modelName);
    settings.setValue("temperature", m_temperature);
    settings.setValue("maxContext", m_maxContext);
    settings.setValue("maxTokens", m_maxTokens);
    settings.setValue("presencePenalty", m_presencePenalty);
    settings.setValue("topP", m_topP);
    settings.setValue("frequencyPenalty", m_frequencyPenalty);
    settings.setValue("seed", m_seed.has_value() ? m_seed.value() : -1);
    settings.setValue("streamingEnabled", m_streamingEnabled);

    switch (m_apiType) {
        case ApiType::Ollama:
            settings.setValue("apiType", "ollama");
            break;
        case ApiType::LlamaCpp:
            settings.setValue("apiType", "llamacpp");
            break;
        case ApiType::Anthropic:
            settings.setValue("apiType", "anthropic");
            break;
        default:
            settings.setValue("apiType", "openai");
            break;
    }
}

// --- Provider management ---

void NetworkManager::ensureProvider(ApiType type) {
    // Check if we already have the right type
    if (m_provider) {
        bool needsSwitch = false;
        switch (type) {
            case ApiType::OpenAI:
                needsSwitch = (qobject_cast<OpenAIProvider*>(m_provider) == nullptr) ||
                              (qobject_cast<LlamaCppProvider*>(m_provider) != nullptr);
                break;
            case ApiType::Ollama:
                needsSwitch = (qobject_cast<OllamaProvider*>(m_provider) == nullptr);
                break;
            case ApiType::LlamaCpp:
                needsSwitch = (qobject_cast<LlamaCppProvider*>(m_provider) == nullptr);
                break;
            case ApiType::Anthropic:
                needsSwitch = (qobject_cast<AnthropicProvider*>(m_provider) == nullptr);
                break;
        }
        if (!needsSwitch) return;

        m_provider->disconnect(this);
        m_provider->deleteLater();
        m_provider = nullptr;
    }

    switch (type) {
        case ApiType::Ollama:
            m_provider = new OllamaProvider(this);
            break;
        case ApiType::LlamaCpp:
            m_provider = new LlamaCppProvider(this);
            break;
        case ApiType::Anthropic:
            m_provider = new AnthropicProvider(this);
            break;
        default:
            m_provider = new OpenAIProvider(this);
            break;
    }

    connectProviderSignals();
}

void NetworkManager::applySettingsToProvider() {
    if (!m_provider) return;

    m_provider->setBaseUrl(m_apiBaseUrl);
    m_provider->setApiKey(m_apiKey);
    m_provider->setModelName(m_modelName);
    m_provider->setStreamingEnabled(m_streamingEnabled);
    m_provider->setTemperature(m_temperature);
    m_provider->setTopP(m_topP);
    m_provider->setMaxTokens(m_maxTokens);
    m_provider->setMaxContext(m_maxContext);
    m_provider->setPresencePenalty(m_presencePenalty);
    m_provider->setFrequencyPenalty(m_frequencyPenalty);
    m_provider->setSeed(m_seed);
}

void NetworkManager::connectProviderSignals() {
    if (!m_provider) return;

    connect(m_provider, &ApiProvider::responseReceived, this, &NetworkManager::responseReceived);
    connect(m_provider, &ApiProvider::streamChunkReceived, this,
            &NetworkManager::streamChunkReceived);
    connect(m_provider, &ApiProvider::streamFinished, this, &NetworkManager::streamFinished);
    connect(m_provider, &ApiProvider::errorOccurred, this, &NetworkManager::errorOccurred);
}
