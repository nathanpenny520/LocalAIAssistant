#include "voicemanager.h"

#include <QAudioDevice>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaDevices>
#include <QMessageAuthenticationCode>
#include <QSettings>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>

#include "girlfriend_translations.h"
#include "girlfriendsettings.h"

// Xunfei Voice WebSocket API — default auth parameters
static const QString ASR_DEFAULT_HOST = "iat-api.xfyun.cn";
static const QString ASR_DEFAULT_PATH = "/v2/iat";
static const QString TTS_DEFAULT_HOST = "cbm01.cn-huabei-1.xf-yun.com";
static const QString TTS_DEFAULT_PATH = "/v1/private/mcd9m97e6";

VoiceManager::VoiceManager(QObject* parent)
        : QObject(parent)
        , m_asrWebSocket(nullptr)
        , m_asrConnected(false)
        , m_ttsWebSocket(nullptr)
        , m_ttsConnected(false)
        , m_audioSource(nullptr)
        , m_audioIODevice(nullptr)
        , m_isRecording(false)
        , m_actualSampleRate(16000)
        , m_actualChannelCount(1)
        , m_audioPollTimer(nullptr)
        , m_mediaPlayer(nullptr)
        , m_audioOutput(nullptr)
        , m_ttsTempFile(nullptr)
        , m_isSpeaking(false)
        , m_voiceType("x6_lingxiaoxuan_pro")  // super-realistic default voice
        , m_enableVoiceOutput(true)
        , m_asrFrameIndex(0)
        , m_ttsSeq(0)
        , m_ttsStreaming(false)
        , m_ttsStreamingBuffer("") {
    loadConfig();

    initAsrWebSocket();
    initTtsWebSocket();

    // Initialize media player and audio output
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);

    m_mediaPlayer->setAudioOutput(m_audioOutput);

    m_audioOutput->setVolume(1.0);

    // Use system default audio output device
    QAudioDevice defaultOutput = QMediaDevices::defaultAudioOutput();
    if (!defaultOutput.isNull()) {
        m_audioOutput->setDevice(defaultOutput);
        qDebug() << "VoiceManager: Initial audio output device:" << defaultOutput.description();
    }

    // Monitor audio output device changes (e.g. AirPods connect/disconnect)
    QMediaDevices* mediaDevices = new QMediaDevices(this);
    connect(mediaDevices, &QMediaDevices::audioOutputsChanged, this,
            &VoiceManager::onAudioOutputsChanged);

    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this,
            &VoiceManager::onPlaybackStateChanged);
}

VoiceManager::~VoiceManager() {
    stopRecording();
    stopSpeaking();

    if (m_asrWebSocket) {
        m_asrWebSocket->close();
        m_asrWebSocket->deleteLater();
    }
    if (m_ttsWebSocket) {
        m_ttsWebSocket->close();
        m_ttsWebSocket->deleteLater();
    }
    if (m_ttsTempFile) {
        m_ttsTempFile->remove();
        m_ttsTempFile->deleteLater();
    }
}

// ==================== Config Loading ====================

QString VoiceManager::findConfigFilePath() const {
    QStringList paths;

    // 1. Project root .env file (preferred)
    QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_MACOS
    // macOS app bundle: LocalAIAssistant.app/Contents/MacOS/
    // Need to traverse up several levels to find .env
    paths << QDir::cleanPath(
            appDir + "/../../../.env");  // build/LocalAIAssistant.app/Contents/MacOS/../../../.env
    paths << QDir::cleanPath(appDir + "/../../.env");         // Contents/.env or build/.env
    paths << QDir::cleanPath(appDir + "/../.env");            // LocalAIAssistant.app/.env
    paths << QDir::cleanPath(appDir + "/../Resources/.env");  // macOS app bundle Resources
    paths << QDir::cleanPath(appDir + "/../Resources/girlfriend/.env");
#elif defined(Q_OS_WIN)
    // Windows: executable in build directory, resources at same level
    paths << QDir::cleanPath(appDir + "/.env");
    paths << QDir::cleanPath(appDir + "/../.env");
    paths << QDir::cleanPath(appDir + "/../sourcecode-ai-assistant/.env");
#else
    // Linux: executable in build directory
    paths << QDir::cleanPath(appDir + "/../sourcecode-ai-assistant/.env");
    paths << QDir::cleanPath(appDir + "/.env");
#endif

    // Current working directory (common during development)
    paths << ".env";
    paths << "../.env";
    paths << "../../.env";
    paths << "../sourcecode-ai-assistant/.env";
    paths << "../../sourcecode-ai-assistant/.env";

    // 2. User data directory .env file
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    paths << QDir::cleanPath(dataDir + "/.env");
    paths << QDir::cleanPath(dataDir + "/girlfriend/.env");

    // Debug: output all search paths
    qDebug() << "VoiceManager: Searching for config in paths:";
    for (const QString& p : paths) {
        qDebug() << "  -" << p << (QFile::exists(p) ? "[EXISTS]" : "");
    }

    for (const QString& path : paths) {
        if (QFile::exists(path)) {
            qDebug() << "VoiceManager: Found config file:" << path;
            return path;
        }
    }

    return QString();
}

bool VoiceManager::loadConfig() {
    // Priority 1: GirlfriendSettings (user-editable via UI — most user-friendly)
    GirlfriendSettings* gs = GirlfriendSettings::instance();
    if (gs->isXfyunConfigured()) {
        m_appId = gs->xfyunAppId();
        m_apiKey = gs->xfyunApiKey();
        m_apiSecret = gs->xfyunApiSecret();
        m_asrUrl = gs->xfyunAsrUrl().isEmpty() ? QStringLiteral("wss://iat-api.xfyun.cn/v2/iat")
                                               : gs->xfyunAsrUrl();
        m_ttsUrl =
                gs->xfyunTtsUrl().isEmpty()
                        ? QStringLiteral("wss://cbm01.cn-huabei-1.xf-yun.com/v1/private/mcd9m97e6")
                        : gs->xfyunTtsUrl();
        if (!gs->xfyunVoiceType().isEmpty()) m_voiceType = gs->xfyunVoiceType();
        qDebug() << "VoiceManager: Loaded credentials from GirlfriendSettings";
        return true;
    }

    // Priority 2: System environment variables
    m_appId = qEnvironmentVariable("XFYUN_APP_ID");
    m_apiKey = qEnvironmentVariable("XFYUN_API_KEY");
    m_apiSecret = qEnvironmentVariable("XFYUN_API_SECRET");

    if (!m_appId.isEmpty() && !m_apiKey.isEmpty() && !m_apiSecret.isEmpty()) {
        qDebug() << "VoiceManager: Loaded credentials from system environment variables";
        m_asrUrl = qEnvironmentVariable("XFYUN_ASR_URL", "wss://iat-api.xfyun.cn/v2/iat");
        m_ttsUrl = qEnvironmentVariable("XFYUN_TTS_URL",
                                        "wss://cbm01.cn-huabei-1.xf-yun.com/v1/private/mcd9m97e6");
        return true;
    }

    // Priority 3: Config files (.env or voice_config.json)
    QString configPath = findConfigFilePath();
    if (configPath.isEmpty()) {
        qDebug() << "VoiceManager: No config file found (.env or voice_config.json)";
        return false;
    }

    if (configPath.endsWith(".env")) {
        return loadFromEnvFile(configPath);
    } else {
        return loadFromJsonFile(configPath);
    }
}

bool VoiceManager::loadFromEnvFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "VoiceManager: Cannot open .env file:" << path;
        return false;
    }

    qDebug() << "VoiceManager: Loading credentials from .env:" << path;

    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine()).trimmed();

        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        // Parse KEY=VALUE format
        int eqPos = line.indexOf('=');
        if (eqPos <= 0) {
            continue;
        }

        QString key = line.left(eqPos).trimmed();
        QString value = line.mid(eqPos + 1).trimmed();

        // Strip quotes if present
        if (value.startsWith('"') && value.endsWith('"')) {
            value = value.mid(1, value.length() - 2);
        }
        if (value.startsWith("'") && value.endsWith("'")) {
            value = value.mid(1, value.length() - 2);
        }

        // Map to member variables
        if (key == "XFYUN_APP_ID") {
            m_appId = value;
        } else if (key == "XFYUN_API_KEY") {
            m_apiKey = value;
        } else if (key == "XFYUN_API_SECRET") {
            m_apiSecret = value;
        } else if (key == "XFYUN_ASR_URL") {
            m_asrUrl = value;
        } else if (key == "XFYUN_TTS_URL") {
            m_ttsUrl = value;
        } else if (key == "XFYUN_VOICE_TYPE") {
            m_voiceType = value;
        }
    }

    file.close();

    qDebug() << "VoiceManager: AppId:" << m_appId << "ApiKey:" << m_apiKey.left(8) + "...";

    return isConfigured();
}

bool VoiceManager::loadFromJsonFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "VoiceManager: Cannot open config file:" << path;
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        qDebug() << "VoiceManager: Invalid config format";
        return false;
    }

    qDebug() << "VoiceManager: Loading credentials from JSON:" << path;

    QJsonObject config = doc.object();
    QJsonObject xfyun = config["xfyun"].toObject();

    m_appId = xfyun["app_id"].toString();
    m_apiKey = xfyun["api_key"].toString();
    m_apiSecret = xfyun["api_secret"].toString();
    m_asrUrl = xfyun["asr_url"].toString();
    m_ttsUrl = xfyun["tts_url"].toString();

    m_voiceType = config["voice_type"].toString("x6_lingxiaoxuan_pro");
    m_enableVoiceOutput = config["enable_voice_output"].toBool(true);

    qDebug() << "VoiceManager: AppId:" << m_appId << "ApiKey:" << m_apiKey.left(8) + "...";

    return isConfigured();
}

bool VoiceManager::isConfigured() const {
    return !m_appId.isEmpty() && !m_apiKey.isEmpty() && !m_apiSecret.isEmpty();
}

void VoiceManager::setVoiceType(const QString& voiceType) {
    m_voiceType = voiceType;
}

// ==================== Xunfei Auth URL Generation ====================

QString VoiceManager::generateAsrAuthUrl() {
    // Parse configured URL, fall back to defaults
    QUrl url(m_asrUrl.isEmpty() ? QString("wss://%1%2").arg(ASR_DEFAULT_HOST, ASR_DEFAULT_PATH)
                                : m_asrUrl);
    return generateAuthUrl(url.host(), url.path());
}

QString VoiceManager::generateTtsAuthUrl() {
    QUrl url(m_ttsUrl.isEmpty() ? QString("wss://%1%2").arg(TTS_DEFAULT_HOST, TTS_DEFAULT_PATH)
                                : m_ttsUrl);
    return generateAuthUrl(url.host(), url.path());
}

QString VoiceManager::generateAuthUrl(const QString& host, const QString& path) {
    // RFC1123 format timestamp
    QString date = QDateTime::currentDateTimeUtc().toString("ddd, dd MMM yyyy HH:mm:ss") + " GMT";

    // Signature origin: host + date + path
    QString signatureOrigin = "host: " + host + "\ndate: " + date + "\nGET " + path + " HTTP/1.1";

    // HMAC-SHA256 signature
    QByteArray key = m_apiSecret.toUtf8();
    QByteArray data = signatureOrigin.toUtf8();
    QByteArray signature = QMessageAuthenticationCode::hash(data, key, QCryptographicHash::Sha256);

    // Base64-encode signature
    QString signatureBase64 = signature.toBase64();

    // authorization origin
    QString authorizationOrigin = QString("api_key=\"%1\", algorithm=\"%2\", headers=\"%3\", "
                                          "signature=\"%4\"")
                                          .arg(m_apiKey)
                                          .arg("hmac-sha256")
                                          .arg("host date request-line")
                                          .arg(signatureBase64);

    // Base64-encode authorization
    QString authorization = authorizationOrigin.toUtf8().toBase64();

    // Build URL using QUrl
    QUrl url;
    url.setScheme("wss");
    url.setHost(host);
    url.setPath(path);

    // Add query parameters
    QUrlQuery query;
    query.addQueryItem("authorization", authorization);
    query.addQueryItem("date", date);
    query.addQueryItem("host", host);
    url.setQuery(query);

    qDebug() << "VoiceManager: Generated auth URL:"
             << url.toString(QUrl::FullyEncoded).left(100) + "...";

    return url.toString(QUrl::FullyEncoded);
}

// ==================== ASR (Speech-to-Text) ====================

void VoiceManager::initAsrWebSocket() {
    m_asrWebSocket = new QWebSocket();

    // Configure SSL to avoid authentication issues
    QSslConfiguration sslConfig = m_asrWebSocket->sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    m_asrWebSocket->setSslConfiguration(sslConfig);

    m_asrWebSocket->ignoreSslErrors();

    connect(m_asrWebSocket, &QWebSocket::connected, this, &VoiceManager::onAsrConnected);
    connect(m_asrWebSocket, &QWebSocket::disconnected, this, &VoiceManager::onAsrDisconnected);
    connect(m_asrWebSocket, &QWebSocket::textMessageReceived, this,
            &VoiceManager::onAsrTextMessageReceived);
    connect(m_asrWebSocket, &QWebSocket::errorOccurred, this, &VoiceManager::onAsrError);
}

void VoiceManager::startRecording() {
    if (!isConfigured()) {
        emit asrError(GTr::xunfeiCredentialsNotConfigured());
        return;
    }

    if (m_isRecording) {
        return;
    }

    m_isRecording = true;
    m_asrPartialText.clear();
    m_asrFinalText.clear();
    m_asrFrameIndex = 0;
    m_audioBuffer.clear();

    emit recordingStarted();
    emit statusChanged(GTr::connectingVoiceService());

    // Connect to ASR WebSocket
    QString authUrl = generateAsrAuthUrl();
    qDebug() << "VoiceManager: Connecting to ASR:" << authUrl.left(100) + "...";
    m_asrWebSocket->open(QUrl(authUrl));

    // Use QAudioSource for audio capture (cross-platform, including Windows)
    QAudioDevice defaultDevice = QMediaDevices::defaultAudioInput();
    if (defaultDevice.isNull()) {
        qDebug() << "VoiceManager: No audio input device available!";
#ifdef Q_OS_WIN
        emit asrError(GTr::voiceNotConfigured() +
                      "\n\nPlease check:\n"
                      "1. Windows Settings > Privacy > Microphone\n"
                      "2. Ensure microphone access is enabled for this app");
#else
        emit asrError(GTr::voiceNotConfigured());
#endif
        m_isRecording = false;
        return;
    }

    qDebug() << "VoiceManager: Audio input device:" << defaultDevice.description();

    // Configure desired audio format: 16kHz, 16bit, mono (Xunfei required)
    QAudioFormat desiredFormat;
    desiredFormat.setSampleRate(16000);
    desiredFormat.setChannelCount(1);
    desiredFormat.setSampleFormat(QAudioFormat::Int16);

    // Check if device supports the desired format
    QAudioFormat actualFormat = desiredFormat;
    if (!defaultDevice.isFormatSupported(desiredFormat)) {
        // Use the closest supported format
        actualFormat = defaultDevice.preferredFormat();
        qDebug() << "VoiceManager: 16kHz not directly supported, using device preferred format:"
                 << actualFormat.sampleRate() << "Hz," << actualFormat.channelCount() << "channels,"
                 << "will resample later";

        // Ensure 16-bit integer format (easier to process)
        if (actualFormat.sampleFormat() != QAudioFormat::Int16) {
            actualFormat.setSampleFormat(QAudioFormat::Int16);
            if (!defaultDevice.isFormatSupported(actualFormat)) {
                // Try Float format if still unsupported
                actualFormat.setSampleFormat(QAudioFormat::Float);
                qDebug() << "VoiceManager: Using Float format instead of Int16";
            }
        }
    } else {
        qDebug() << "VoiceManager: Using 16kHz audio format directly";
    }

    m_audioFormat = actualFormat;
    m_actualSampleRate = actualFormat.sampleRate();
    m_actualChannelCount = actualFormat.channelCount();

    // Create QAudioSource
    m_audioSource = new QAudioSource(defaultDevice, actualFormat, this);

    // Set buffer size (Windows needs enough buffer)
    // 16kHz, 16bit, mono: 16000 * 2 * 1 = 32000 bytes per second
    // 100ms buffer = 3200 bytes
    m_audioSource->setBufferSize(6400);  // 200ms buffer

    m_audioIODevice = m_audioSource->start();

    if (!m_audioIODevice) {
        qDebug() << "VoiceManager: Failed to start audio source";
#ifdef Q_OS_WIN
        emit asrError(GTr::voiceNotConfigured() +
                      "\n\nPossible causes:\n"
                      "1. Microphone is in use by another application\n"
                      "2. Windows Settings > Privacy > Microphone - ensure access is enabled\n"
                      "3. Check that your microphone is properly connected");
#else
        emit asrError(GTr::voiceNotConfigured());
#endif
        m_audioSource->deleteLater();
        m_audioSource = nullptr;
        m_isRecording = false;
        return;
    }

    // Check audio source state
    QAudio::State audioState = m_audioSource->state();
    qDebug() << "VoiceManager: Audio source state:" << audioState
             << "- buffer size:" << m_audioSource->bufferSize()
             << "- bytes available:" << m_audioIODevice->bytesAvailable();

    if (audioState == QAudio::StoppedState || audioState == QAudio::IdleState) {
        qDebug() << "VoiceManager: Audio source not in ActiveState, checking error...";
        QAudio::Error audioError = m_audioSource->error();
        qDebug() << "VoiceManager: Audio error:" << audioError;
        if (audioError != QAudio::NoError) {
            emit asrError(GTr::voiceNotConfigured() + QString(" (error: %1)").arg(audioError));
            m_audioSource->stop();
            m_audioSource->deleteLater();
            m_audioSource = nullptr;
            m_audioIODevice = nullptr;
            m_isRecording = false;
            return;
        }
    }

    // Use timer polling for audio data (Windows compatibility)
    // On Windows, the readyRead signal may be unreliable — poll proactively
    m_audioPollTimer = new QTimer(this);
    connect(m_audioPollTimer, &QTimer::timeout, this, &VoiceManager::onAudioPollTimeout);
    m_audioPollTimer->start(50);  // poll every 50ms

    // Also connect readyRead as fallback (works on some platforms)
    connect(m_audioIODevice, &QIODevice::readyRead, this, &VoiceManager::onAudioDataReady);

    qDebug() << "VoiceManager: Recording started with QAudioSource"
             << "- sample rate:" << m_actualSampleRate << "- channels:" << m_actualChannelCount
             << "- format:"
             << (m_audioFormat.sampleFormat() == QAudioFormat::Int16 ? "Int16" : "Float")
             << "- polling mode: 50ms interval";
}

void VoiceManager::stopRecording() {
    if (!m_isRecording) {
        return;
    }

    m_isRecording = false;

    // Stop polling timer
    if (m_audioPollTimer) {
        m_audioPollTimer->stop();
        m_audioPollTimer->deleteLater();
        m_audioPollTimer = nullptr;
    }

    // Stop audio source
    if (m_audioSource) {
        m_audioSource->stop();
        // Read remaining data
        if (m_audioIODevice && m_audioIODevice->bytesAvailable() > 0) {
            onAudioDataReady();
        }
    }

    emit recordingStopped();
    emit statusChanged(GTr::recognizing());

    qDebug() << "VoiceManager: Recording stopped, total audio buffer size:" << m_audioBuffer.size();

    // Process and send accumulated audio data
    processAndSendAudioData();
}

void VoiceManager::onAsrConnected() {
    m_asrConnected = true;
    emit statusChanged(GTr::startSpeaking());

    // Send initial frame (business parameters)
    QJsonObject frame;
    QJsonObject common;
    common["app_id"] = m_appId;
    frame["common"] = common;

    QJsonObject business;
    business["language"] = "zh_cn";
    business["domain"] = "iat";
    business["accent"] = "mandarin";
    business["vad_eos"] = 5000;  // VAD timeout: 5 seconds to allow longer pauses
    business["dwa"] = "wpgs";    // dynamic correction
    business["ptt"] = 1;         // punctuation prediction
    // Note: rhxd parameter has been removed from the Xunfei API
    frame["business"] = business;

    QJsonObject data;
    data["status"] = 0;
    data["format"] = "audio/L16;rate=16000";
    data["encoding"] = "raw";
    data["audio"] = "";
    frame["data"] = data;

    QString jsonFrame = QJsonDocument(frame).toJson(QJsonDocument::Compact);
    m_asrWebSocket->sendTextMessage(jsonFrame);

    qDebug() << "VoiceManager: ASR WebSocket connected, sent initial frame";
}

void VoiceManager::onAsrDisconnected() {
    m_asrConnected = false;
    qDebug() << "VoiceManager: ASR WebSocket disconnected";
}

void VoiceManager::onAsrTextMessageReceived(const QString& message) {
    qDebug() << "VoiceManager: ASR received message:" << message.left(200);
    parseAsrResponse(message);
}

void VoiceManager::onAsrError(QAbstractSocket::SocketError error) {
    QString errorMsg = GTr::asrConnectionError(m_asrWebSocket->errorString());
    qDebug() << "VoiceManager:" << errorMsg;
    emit asrError(errorMsg);
    emit statusChanged(errorMsg);
}

void VoiceManager::sendAudioFrame(const QByteArray& audioData) {
    if (!m_asrConnected || audioData.isEmpty()) {
        return;
    }

    m_asrFrameIndex++;

    QJsonObject frame;
    QJsonObject data;
    data["status"] = 1;
    data["format"] = "audio/L16;rate=16000";
    data["encoding"] = "raw";
    data["audio"] = QString(audioData.toBase64());
    frame["data"] = data;

    QString jsonFrame = QJsonDocument(frame).toJson(QJsonDocument::Compact);
    m_asrWebSocket->sendTextMessage(jsonFrame);
}

void VoiceManager::sendAsrEndSignal() {
    if (!m_asrConnected) {
        return;
    }

    QJsonObject frame;
    QJsonObject data;
    data["status"] = 2;
    data["format"] = "audio/L16;rate=16000";
    data["encoding"] = "raw";
    data["audio"] = "";
    frame["data"] = data;

    QString jsonFrame = QJsonDocument(frame).toJson(QJsonDocument::Compact);
    m_asrWebSocket->sendTextMessage(jsonFrame);

    qDebug() << "VoiceManager: Sent ASR end signal";
}

void VoiceManager::parseAsrResponse(const QString& jsonResponse) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonResponse.toUtf8());
    if (!doc.isObject()) {
        qDebug() << "VoiceManager: ASR response is not JSON object";
        return;
    }

    QJsonObject response = doc.object();

    // Check for errors
    if (response.contains("code") && response["code"].toInt() != 0) {
        QString error = GTr::asrConnectionError(QString("[%1]: %2")
                                                        .arg(response["code"].toInt())
                                                        .arg(response["message"].toString()));
        emit asrError(error);
        return;
    }

    // Parse recognition result
    QJsonObject data = response["data"].toObject();
    if (data.isEmpty()) {
        qDebug() << "VoiceManager: ASR data is empty";
        return;
    }

    int status = data["status"].toInt();

    // result is an object containing a ws array
    QJsonObject resultObj = data["result"].toObject();
    QJsonArray wsArray = resultObj["ws"].toArray();

    QString resultText;
    for (const QJsonValue& wsVal : wsArray) {
        QJsonObject ws = wsVal.toObject();
        QJsonArray cwArray = ws["cw"].toArray();
        for (const QJsonValue& cwVal : cwArray) {
            QJsonObject cw = cwVal.toObject();
            resultText += cw["w"].toString();
        }
    }

    qDebug() << "VoiceManager: ASR parsed text:" << resultText << "status:" << status;

    // Process result
    if (!resultText.isEmpty()) {
        // Check pgs ("apd" = append, "rpl" = replace)
        QString pgs = resultObj["pgs"].toString();

        if (pgs == "rpl") {
            // Replace previous result
            m_asrPartialText = resultText;
        } else {
            // Append result
            m_asrPartialText += resultText;
        }

        emit asrPartialResult(m_asrPartialText);

        // status=2 indicates final result
        if (status == 2) {
            m_asrFinalText = m_asrPartialText;
            emit asrFinalResult(m_asrFinalText);
            emit statusChanged(GTr::recognitionComplete());

            qDebug() << "VoiceManager: ASR final result:" << m_asrFinalText;
        }
    }
}

void VoiceManager::processAndSendAudioData() {
    // Check if WebSocket is connected
    if (!m_asrConnected) {
        qDebug() << "VoiceManager: ASR WebSocket not connected yet, waiting...";
        // Wait for connection to establish (up to 3 seconds)
        int waitCount = 0;
        while (!m_asrConnected && waitCount < 30) {
            QCoreApplication::processEvents();
            QThread::msleep(100);
            waitCount++;
        }
        qDebug() << "VoiceManager: Waited" << waitCount * 100 << "ms, connected:" << m_asrConnected;
    }

    if (!m_asrConnected) {
        qDebug() << "VoiceManager: ASR WebSocket still not connected, aborting";
        emit asrError(GTr::asrConnectionError("WebSocket connection timeout"));
        m_audioBuffer.clear();
        return;
    }

    if (m_audioBuffer.isEmpty()) {
        qDebug() << "VoiceManager: Audio buffer is empty, nothing to send";
        sendAsrEndSignal();
        return;
    }

    qDebug() << "VoiceManager: Processing audio data, size:" << m_audioBuffer.size();

    QByteArray audioData = m_audioBuffer;
    m_audioBuffer.clear();

    // Handle Float format conversion if needed
    if (m_audioFormat.sampleFormat() == QAudioFormat::Float) {
        qDebug() << "VoiceManager: Converting Float to Int16";
        // Float (32-bit) to Int16 (16-bit) conversion
        int sampleCount = audioData.size() / 4;  // 4 bytes per float sample
        QByteArray int16Data;
        int16Data.reserve(sampleCount * 2);

        const float* floatData = reinterpret_cast<const float*>(audioData.constData());
        for (int i = 0; i < sampleCount; i++) {
            float sample = floatData[i];
            // Clamp to [-1.0, 1.0]
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;
            // Convert to Int16 range [-32768, 32767]
            short int16Sample = static_cast<short>(sample * 32767.0f);
            int16Data.append(reinterpret_cast<char*>(&int16Sample), 2);
        }
        audioData = int16Data;
    }

    // Channel conversion: multi-channel to mono (Xunfei requires mono)
    // Performed after Float-to-Int16, when data is already 16-bit PCM
    if (m_actualChannelCount > 1) {
        qDebug() << "VoiceManager: Converting" << m_actualChannelCount << "channels to mono";

        int bytesPerSample = 2;                                     // 16-bit = 2 bytes
        int bytesPerFrame = bytesPerSample * m_actualChannelCount;  // one frame = all channels
        int frameCount = audioData.size() / bytesPerFrame;

        QByteArray monoData;
        monoData.reserve(frameCount * bytesPerSample);

        for (int i = 0; i < frameCount; i++) {
            int frameOffset = i * bytesPerFrame;

            // Average across all channels
            int sum = 0;
            for (int ch = 0; ch < m_actualChannelCount; ch++) {
                int sampleOffset = frameOffset + ch * bytesPerSample;
                if (sampleOffset + 2 <= audioData.size()) {
                    short sample =
                            *reinterpret_cast<const short*>(audioData.constData() + sampleOffset);
                    sum += sample;
                }
            }
            // Compute average
            short monoSample = static_cast<short>(sum / m_actualChannelCount);
            monoData.append(reinterpret_cast<char*>(&monoSample), 2);
        }

        audioData = monoData;
        qDebug() << "VoiceManager: Converted to mono, data size:" << audioData.size();
    }

    // Resample if sample rate is not 16kHz
    if (m_actualSampleRate != 16000 && m_actualSampleRate > 16000) {
        int ratio = m_actualSampleRate / 16000;
        qDebug() << "VoiceManager: Resampling from" << m_actualSampleRate
                 << "Hz to 16000 Hz (ratio:" << ratio << ")";

        // Simple downsampling: take every Nth sample
        // 16-bit PCM: 2 bytes per sample
        int bytesPerSample = 2;
        int sampleCount = audioData.size() / bytesPerSample;

        QByteArray resampledData;
        for (int i = 0; i < sampleCount; i += ratio) {
            int offset = i * bytesPerSample;
            if (offset + bytesPerSample <= audioData.size()) {
                resampledData.append(audioData.mid(offset, bytesPerSample));
            }
        }

        audioData = resampledData;
        qDebug() << "VoiceManager: Resampled data size:" << audioData.size();
    }

    // Audio gain — amplify by 1.5x (addresses low recording volume)
    // 16-bit PCM: each sample is a signed short
    int bytesPerSample = 2;  // 16-bit = 2 bytes
    int sampleCount = audioData.size() / bytesPerSample;
    float gainFactor = 1.5f;

    for (int i = 0; i < sampleCount; i++) {
        int offset = i * bytesPerSample;
        if (offset + 2 <= audioData.size()) {
            // Read 16-bit sample value
            short sample = *reinterpret_cast<const short*>(audioData.constData() + offset);
            // Apply gain
            float amplified = sample * gainFactor;
            // Clamp to prevent overflow
            if (amplified > 32767) amplified = 32767;
            if (amplified < -32768) amplified = -32768;
            // Write back
            short newSample = static_cast<short>(amplified);
            audioData[offset] = reinterpret_cast<char*>(&newSample)[0];
            audioData[offset + 1] = reinterpret_cast<char*>(&newSample)[1];
        }
    }
    qDebug() << "VoiceManager: Applied audio gain factor:" << gainFactor;

    // Xunfei required format: 16kHz, 16bit, mono
    QString formatStr = "audio/L16;rate=16000";

    // Send in frames (16kHz: 1280 bytes = 40ms)
    int frameSize = 1280;
    int offset = 0;
    int framesSent = 0;
    while (offset < audioData.size()) {
        QByteArray frameData = audioData.mid(offset, frameSize);

        // Send audio frame
        QJsonObject frame;
        QJsonObject data;
        data["status"] = 1;
        data["format"] = formatStr;
        data["encoding"] = "raw";
        data["audio"] = QString(frameData.toBase64());
        frame["data"] = data;

        QString jsonFrame = QJsonDocument(frame).toJson(QJsonDocument::Compact);
        m_asrWebSocket->sendTextMessage(jsonFrame);

        offset += frameSize;
        framesSent++;
    }

    qDebug() << "VoiceManager: Sent" << framesSent << "audio frames at 16kHz";

    // Send end frame
    sendAsrEndSignal();

    // Cleanup audio source
    if (m_audioSource) {
        m_audioSource->deleteLater();
        m_audioSource = nullptr;
        m_audioIODevice = nullptr;
    }
}

void VoiceManager::onAudioDataReady() {
    if (!m_audioIODevice || !m_isRecording) {
        return;
    }

    // Read all available audio data
    QByteArray newData = m_audioIODevice->readAll();
    if (newData.isEmpty()) {
        return;
    }

    // Accumulate in buffer
    m_audioBuffer.append(newData);

    qDebug() << "VoiceManager: Read" << newData.size()
             << "bytes, total buffer:" << m_audioBuffer.size();
}

void VoiceManager::onAudioPollTimeout() {
    // Timer-based polling for audio data (Windows compatibility)
    // Proactively check and read available audio data
    if (!m_audioIODevice || !m_isRecording || !m_audioSource) {
        return;
    }

    // Check audio source state (may stop unexpectedly on Windows)
    QAudio::State state = m_audioSource->state();
    if (state == QAudio::StoppedState) {
        QAudio::Error error = m_audioSource->error();
        if (error != QAudio::NoError) {
            qDebug() << "VoiceManager: Audio source stopped with error:" << error;
            m_isRecording = false;
            emit asrError(GTr::voiceNotConfigured() + QString(" (audio error: %1)").arg(error));
            if (m_audioPollTimer) {
                m_audioPollTimer->stop();
            }
            return;
        }
    }

    // Check for available data
    qint64 bytesAvailable = m_audioIODevice->bytesAvailable();
    if (bytesAvailable > 0) {
        // Log every 500ms to avoid spam
        static int pollCount = 0;
        pollCount++;
        if (pollCount % 10 == 0) {  // 50ms * 10 = 500ms
            qDebug() << "VoiceManager: Polling - bytes available:" << bytesAvailable
                     << ", buffer size:" << m_audioBuffer.size() << ", audio state:" << state;
        }
        onAudioDataReady();
    }
}

// ==================== TTS (Text-to-Speech) ====================

void VoiceManager::initTtsWebSocket() {
    m_ttsWebSocket = new QWebSocket();

    // Configure SSL
    QSslConfiguration sslConfig = m_ttsWebSocket->sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    m_ttsWebSocket->setSslConfiguration(sslConfig);
    m_ttsWebSocket->ignoreSslErrors();

    connect(m_ttsWebSocket, &QWebSocket::connected, this, &VoiceManager::onTtsConnected);
    connect(m_ttsWebSocket, &QWebSocket::disconnected, this, &VoiceManager::onTtsDisconnected);
    connect(m_ttsWebSocket, &QWebSocket::binaryMessageReceived, this,
            &VoiceManager::onTtsBinaryMessageReceived);
    connect(m_ttsWebSocket, &QWebSocket::textMessageReceived, this,
            &VoiceManager::onTtsTextMessageReceived);
    connect(m_ttsWebSocket, &QWebSocket::errorOccurred, this, &VoiceManager::onTtsError);
}

void VoiceManager::speak(const QString& text) {
    if (!isConfigured()) {
        emit ttsError(GTr::xunfeiCredentialsNotConfigured());
        return;
    }

    if (text.isEmpty()) {
        return;
    }

    // Stop any active playback first
    if (m_isSpeaking) {
        stopSpeaking();
    }

    // Don't set m_isSpeaking here — set it when playback actually starts
    m_ttsAudioBuffer.clear();
    m_ttsText = text;  // save text to synthesize

    emit statusChanged(GTr::synthesizingVoice());

    // Connect to TTS WebSocket
    QString authUrl = generateTtsAuthUrl();
    qDebug() << "VoiceManager: Connecting to TTS:" << authUrl.left(100) + "...";
    m_ttsWebSocket->open(QUrl(authUrl));
}

void VoiceManager::stopSpeaking() {
    if (!m_isSpeaking) {
        // Clean up residual resources even if not in speaking state
        if (m_mediaPlayer) {
            m_mediaPlayer->stop();
            m_mediaPlayer->setSource(QUrl());  // clear source and free buffer
        }
        // Clean up temp file
        if (m_ttsTempFile) {
            m_ttsTempFile->remove();
            m_ttsTempFile->deleteLater();
            m_ttsTempFile = nullptr;
        }
        // Clear audio buffer
        m_ttsAudioBuffer.clear();
        return;
    }

    m_isSpeaking = false;

    // Immediately stop media player
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
        // Wait for player to fully stop (briefly block main thread)
        QCoreApplication::processEvents();
        // Clear source and free buffer
        m_mediaPlayer->setSource(QUrl());
    }

    // Close WebSocket
    if (m_ttsWebSocket && m_ttsConnected) {
        m_ttsWebSocket->close();
    }

    // Clean up temp file
    if (m_ttsTempFile) {
        m_ttsTempFile->remove();
        m_ttsTempFile->deleteLater();
        m_ttsTempFile = nullptr;
    }

    // Clear audio buffer
    m_ttsAudioBuffer.clear();

    // Process event queue to ensure cleanup completes
    QCoreApplication::processEvents();

    emit speakingFinished();
    emit statusChanged(GTr::voiceStopped());

    qDebug() << "VoiceManager: Audio completely cleared and resources released";
}

void VoiceManager::onTtsConnected() {
    m_ttsConnected = true;
    emit statusChanged(GTr::voiceServiceConnected());
    qDebug() << "VoiceManager: TTS WebSocket connected";

    // In streaming mode, wait for caller to send text — only send the first frame
    if (m_ttsStreaming) {
        sendStreamingFirstFrame();
        return;
    }

    // Non-streaming mode: send request immediately after connecting
    if (!m_ttsText.isEmpty()) {
        sendTtsRequest(m_ttsText);
    }
}

void VoiceManager::onTtsDisconnected() {
    m_ttsConnected = false;
    qDebug() << "VoiceManager: TTS WebSocket disconnected, audio buffer size:"
             << m_ttsAudioBuffer.size();

    // Playback is handled separately in onTtsTextMessageReceived
    // Clear buffer for next synthesis
    m_ttsAudioBuffer.clear();
}

void VoiceManager::onTtsBinaryMessageReceived(const QByteArray& message) {
    // Xunfei TTS returns binary audio data
    qDebug() << "VoiceManager: TTS received binary data:" << message.size() << "bytes";
    m_ttsAudioBuffer.append(message);
}

void VoiceManager::onTtsTextMessageReceived(const QString& message) {
    // Parse super-realistic TTS JSON response
    qDebug() << "VoiceManager: TTS received text message:" << message.left(100);

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        qDebug() << "VoiceManager: TTS response is not JSON object";
        return;
    }

    QJsonObject response = doc.object();

    // Parse header
    QJsonObject header = response["header"].toObject();
    int code = header["code"].toInt();
    if (code != 0) {
        QString errorMsg = header["message"].toString();
        emit ttsError(GTr::ttsErrorWithCode(code, errorMsg));
        return;
    }

    // Parse payload
    QJsonObject payload = response["payload"].toObject();
    if (payload.isEmpty()) {
        // Empty data frame — ignore per API docs (data empty + code=0 → skip)
        return;
    }

    // Parse audio
    QJsonObject audioObj = payload["audio"].toObject();
    QString audioBase64 = audioObj["audio"].toString();

    if (!audioBase64.isEmpty()) {
        // Decode base64 and append to audio buffer
        QByteArray audioData = QByteArray::fromBase64(audioBase64.toUtf8());
        m_ttsAudioBuffer.append(audioData);
        qDebug() << "VoiceManager: TTS decoded audio:" << audioData.size()
                 << "bytes, total buffer:" << m_ttsAudioBuffer.size();
    }

    // Check synthesis status
    int status = audioObj["status"].toInt();
    if (status == 2) {
        // Synthesis complete, play audio
        qDebug() << "VoiceManager: TTS synthesis complete, total audio:" << m_ttsAudioBuffer.size()
                 << "bytes";
        emit statusChanged(GTr::voiceSynthesisComplete());

        if (!m_ttsAudioBuffer.isEmpty()) {
            QByteArray audioToPlay = m_ttsAudioBuffer;
            m_ttsAudioBuffer.clear();
            playTtsAudio(audioToPlay);
        }
    }
}

void VoiceManager::onTtsError(QAbstractSocket::SocketError error) {
    QString errorMsg = GTr::ttsConnectionError(m_ttsWebSocket->errorString());
    qDebug() << "VoiceManager:" << errorMsg;
    emit ttsError(errorMsg);
    emit statusChanged(errorMsg);
}

void VoiceManager::sendTtsRequest(const QString& text) {
    if (!m_ttsConnected || text.isEmpty()) {
        return;
    }

    // Super-realistic TTS protocol structure
    QJsonObject frame;

    // header
    QJsonObject header;
    header["app_id"] = m_appId;
    header["status"] = 2;  // one-shot synthesis = 2, streaming = 0/1/2
    frame["header"] = header;

    // parameter — capability parameters
    QJsonObject parameter;

    // oral — colloquialization config (only supported for x4 series voices)
    QJsonObject oral;
    oral["oral_level"] = "mid";  // high/mid/low
    oral["spark_assist"] = 1;    // LLM-assisted colloquialization
    oral["stop_split"] = 0;      // keep server-side sentence splitting on
    oral["remain"] = 0;          // don't preserve original formal text
    parameter["oral"] = oral;

    // tts — synthesis parameters
    QJsonObject tts;
    tts["vcn"] = m_voiceType;  // voice name
    tts["speed"] = 50;         // speed 0-100
    tts["volume"] = 50;        // volume 0-100
    tts["pitch"] = 50;         // pitch 0-100
    tts["bgs"] = 0;            // no background sound
    tts["reg"] = 0;            // English pronunciation: auto-detect
    tts["rdn"] = 0;            // digit pronunciation: auto-detect
    tts["rhy"] = 0;            // don't return pinyin annotations

    // audio — audio format parameters
    QJsonObject audio;
    audio["encoding"] = "lame";    // MP3 format
    audio["sample_rate"] = 24000;  // 24k sample rate (higher quality for super-realistic)
    audio["channels"] = 1;         // mono
    audio["bit_depth"] = 16;       // 16 bit
    audio["frame_size"] = 0;       // default frame size
    tts["audio"] = audio;

    parameter["tts"] = tts;
    frame["parameter"] = parameter;

    // payload — input data
    QJsonObject payload;
    QJsonObject textObj;
    textObj["encoding"] = "utf8";
    textObj["compress"] = "raw";
    textObj["format"] = "plain";
    textObj["status"] = 2;  // data status: 2 = end
    textObj["seq"] = 0;     // sequence number
    textObj["text"] = QString(text.toUtf8().toBase64());
    payload["text"] = textObj;
    frame["payload"] = payload;

    QString jsonFrame = QJsonDocument(frame).toJson(QJsonDocument::Compact);
    m_ttsWebSocket->sendTextMessage(jsonFrame);

    qDebug() << "VoiceManager: Sent super realistic TTS request";
    qDebug() << "  - Text:" << text;
    qDebug() << "  - Voice:" << m_voiceType;
}

void VoiceManager::startStreamingTts() {
    if (!isConfigured()) {
        emit ttsError(GTr::xunfeiCredentialsNotConfigured());
        return;
    }

    m_ttsAudioBuffer.clear();
    m_ttsSeq = 0;
    m_ttsStreaming = true;

    emit statusChanged(GTr::synthesizingVoice());

    // Connect to TTS WebSocket
    QString authUrl = generateTtsAuthUrl();
    qDebug() << "VoiceManager: Connecting to streaming TTS:" << authUrl.left(100) + "...";
    m_ttsWebSocket->open(QUrl(authUrl));
}

void VoiceManager::sendStreamingText(const QString& text) {
    if (!m_ttsConnected || text.isEmpty() || !m_ttsStreaming) {
        return;
    }

    m_ttsSeq++;

    QJsonObject frame;

    QJsonObject header;
    header["app_id"] = m_appId;
    header["status"] = 1;  // streaming intermediate frame
    frame["header"] = header;

    // parameter (may be omitted after the first frame, but sent each time for safety)
    QJsonObject parameter;
    QJsonObject oral;
    oral["oral_level"] = "mid";
    oral["spark_assist"] = 1;
    parameter["oral"] = oral;

    QJsonObject tts;
    tts["vcn"] = m_voiceType;
    QJsonObject audio;
    audio["encoding"] = "lame";
    audio["sample_rate"] = 24000;
    audio["channels"] = 1;
    audio["bit_depth"] = 16;
    audio["frame_size"] = 0;
    tts["audio"] = audio;
    parameter["tts"] = tts;
    frame["parameter"] = parameter;

    // payload
    QJsonObject payload;
    QJsonObject textObj;
    textObj["encoding"] = "utf8";
    textObj["compress"] = "raw";
    textObj["format"] = "plain";
    textObj["status"] = 1;  // intermediate data
    textObj["seq"] = m_ttsSeq;
    textObj["text"] = QString(text.toUtf8().toBase64());
    payload["text"] = textObj;
    frame["payload"] = payload;

    QString jsonFrame = QJsonDocument(frame).toJson(QJsonDocument::Compact);
    m_ttsWebSocket->sendTextMessage(jsonFrame);

    qDebug() << "VoiceManager: Sent streaming TTS text seq=" << m_ttsSeq << "text=" << text.left(20)
             << "...";
}

void VoiceManager::finishStreamingTts() {
    if (!m_ttsConnected || !m_ttsStreaming) {
        return;
    }

    m_ttsStreaming = false;

    // Send end frame
    QJsonObject frame;

    QJsonObject header;
    header["app_id"] = m_appId;
    header["status"] = 2;  // end
    frame["header"] = header;

    QJsonObject payload;
    QJsonObject textObj;
    textObj["encoding"] = "utf8";
    textObj["compress"] = "raw";
    textObj["format"] = "plain";
    textObj["status"] = 2;  // end data
    textObj["seq"] = m_ttsSeq + 1;
    textObj["text"] = "";
    payload["text"] = textObj;
    frame["payload"] = payload;

    QString jsonFrame = QJsonDocument(frame).toJson(QJsonDocument::Compact);
    m_ttsWebSocket->sendTextMessage(jsonFrame);

    qDebug() << "VoiceManager: Sent streaming TTS end signal";
}

void VoiceManager::sendStreamingFirstFrame() {
    if (!m_ttsConnected || !m_ttsStreaming) {
        return;
    }

    // Send initial streaming frame, status=0 indicates start
    QJsonObject frame;

    QJsonObject header;
    header["app_id"] = m_appId;
    header["status"] = 0;  // first frame, start
    frame["header"] = header;

    // parameter — full parameters required in first frame
    QJsonObject parameter;
    QJsonObject oral;
    oral["oral_level"] = "mid";
    oral["spark_assist"] = 1;
    oral["stop_split"] = 0;
    oral["remain"] = 0;
    parameter["oral"] = oral;

    QJsonObject tts;
    tts["vcn"] = m_voiceType;
    tts["speed"] = 50;
    tts["volume"] = 50;
    tts["pitch"] = 50;
    tts["bgs"] = 0;
    tts["reg"] = 0;
    tts["rdn"] = 0;
    tts["rhy"] = 0;

    QJsonObject audio;
    audio["encoding"] = "lame";
    audio["sample_rate"] = 24000;
    audio["channels"] = 1;
    audio["bit_depth"] = 16;
    audio["frame_size"] = 0;
    tts["audio"] = audio;
    parameter["tts"] = tts;
    frame["parameter"] = parameter;

    // payload — first frame typically has no text, just establishes the session
    QJsonObject payload;
    QJsonObject textObj;
    textObj["encoding"] = "utf8";
    textObj["compress"] = "raw";
    textObj["format"] = "plain";
    textObj["status"] = 0;  // first frame
    textObj["seq"] = 0;
    textObj["text"] = "";
    payload["text"] = textObj;
    frame["payload"] = payload;

    QString jsonFrame = QJsonDocument(frame).toJson(QJsonDocument::Compact);
    m_ttsWebSocket->sendTextMessage(jsonFrame);

    qDebug() << "VoiceManager: Sent streaming TTS first frame (status=0)";
}

void VoiceManager::parseTtsResponse(const QByteArray& binaryData, const QString& jsonMeta) {
    // Accumulate audio data
    if (!binaryData.isEmpty()) {
        m_ttsAudioBuffer.append(binaryData);
    }

    // Parse metadata (if any text message was received)
    if (!jsonMeta.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(jsonMeta.toUtf8());
        if (doc.isObject()) {
            QJsonObject response = doc.object();
            int code = response["code"].toInt();
            if (code != 0) {
                QString error = GTr::ttsErrorWithCode(code, response["message"].toString());
                emit ttsError(error);
                return;
            }

            int status = response["data"].toObject()["status"].toInt();
            if (status == 2) {
                // Synthesis complete
                emit statusChanged(GTr::voiceSynthesisComplete());
            }
        }
    }
}

void VoiceManager::playTtsAudio(const QByteArray& audioData) {
    if (audioData.isEmpty()) {
        qDebug() << "VoiceManager: TTS audio data is empty, cannot play";
        emit ttsError(GTr::audioDataEmpty());
        return;
    }

    // Super-realistic TTS returns MP3 audio (encoding=lame), 24kHz sample rate
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempFile = tempPath + "/tts_output_" +
                       QDateTime::currentDateTime().toString("yyyyMMddHHmmss") + ".mp3";

    QFile file(tempFile);
    if (!file.open(QIODevice::WriteOnly)) {
        emit ttsError(GTr::cannotCreateAudioFile());
        return;
    }

    // Write MP3 data directly
    file.write(audioData);
    file.flush();
    file.close();

    // Verify file exists
    if (!QFile::exists(tempFile)) {
        qDebug() << "VoiceManager: MP3 file not found after write:" << tempFile;
        emit ttsError(GTr::audioFileCreateFailed());
        return;
    }

    qDebug() << "VoiceManager: MP3 file created:" << tempFile << "size:" << audioData.size();

    // Clean up old temp file
    if (m_ttsTempFile) {
        m_ttsTempFile->remove();
        m_ttsTempFile->deleteLater();
        m_ttsTempFile = nullptr;
    }

    // Set playback state
    m_isSpeaking = true;
    emit speakingStarted();

    // Before playback, ensure we use the current default output device (double safeguard)
    QAudioDevice currentDefault = QMediaDevices::defaultAudioOutput();
    if (!currentDefault.isNull() && m_audioOutput->device() != currentDefault) {
        qDebug() << "VoiceManager: Switching to current default output:"
                 << currentDefault.description();
        m_audioOutput->setDevice(currentDefault);
    }

    // Play using QMediaPlayer
    m_ttsTempFile = new QFile(tempFile, this);
    QUrl audioUrl = QUrl::fromLocalFile(tempFile);

    qDebug() << "VoiceManager: Playing WAV from:" << audioUrl.toString();

    m_mediaPlayer->setSource(audioUrl);
    m_mediaPlayer->play();

    emit statusChanged(GTr::playingVoice());
}

void VoiceManager::onPlaybackStateChanged(QMediaPlayer::PlaybackState state) {
    if (state == QMediaPlayer::StoppedState) {
        m_isSpeaking = false;
        emit speakingFinished();
        emit statusChanged(GTr::playbackComplete());

        // Clean up temp file
        if (m_ttsTempFile) {
            m_ttsTempFile->remove();
            m_ttsTempFile->deleteLater();
            m_ttsTempFile = nullptr;
        }
    }
}

void VoiceManager::onAudioOutputsChanged() {
    // Audio output device changed (e.g., AirPods connect/disconnect) — switch to new default
    QAudioDevice newDefault = QMediaDevices::defaultAudioOutput();
    if (!newDefault.isNull()) {
        qDebug() << "VoiceManager: Audio output device changed, switching to:"
                 << newDefault.description();
        m_audioOutput->setDevice(newDefault);
    } else {
        qDebug() << "VoiceManager: No audio output device available after change";
    }
}