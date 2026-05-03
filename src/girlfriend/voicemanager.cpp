#include "voicemanager.h"
#include "girlfriend_translations.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDateTime>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QUrl>
#include <QUrlQuery>
#include <QSettings>
#include <QTemporaryFile>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QMediaDevices>
#include <QAudioDevice>

// 超拟人语音合成 WebSocket API 鉴权参数
static const QString ASR_HOST = "iat-api.xfyun.cn";
static const QString ASR_PATH = "/v2/iat";
static const QString TTS_HOST = "cbm01.cn-huabei-1.xf-yun.com";
static const QString TTS_PATH = "/v1/private/mcd9m97e6";

VoiceManager::VoiceManager(QObject *parent)
    : QObject(parent)
    , m_asrWebSocket(nullptr)
    , m_asrConnected(false)
    , m_ttsWebSocket(nullptr)
    , m_ttsConnected(false)
    , m_audioSource(nullptr)
    , m_audioIODevice(nullptr)
    , m_isRecording(false)
    , m_actualSampleRate(16000)
    , m_mediaPlayer(nullptr)
    , m_audioOutput(nullptr)
    , m_ttsTempFile(nullptr)
    , m_isSpeaking(false)
    , m_voiceType("xiaoyan")
    , m_enableVoiceOutput(true)
    , m_asrFrameIndex(0)
    , m_ttsSeq(0)
    , m_ttsStreaming(false)
    , m_ttsStreamingBuffer("")
{
    loadConfig();

    initAsrWebSocket();
    initTtsWebSocket();

    // 初始化播放器和音频输出
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);

    // 连接音频输出到播放器
    m_mediaPlayer->setAudioOutput(m_audioOutput);

    // 设置音量
    m_audioOutput->setVolume(1.0);  // 最大音量

    // 使用系统默认音频输出设备
    QAudioDevice defaultOutput = QMediaDevices::defaultAudioOutput();
    if (!defaultOutput.isNull()) {
        m_audioOutput->setDevice(defaultOutput);
        qDebug() << "VoiceManager: Initial audio output device:" << defaultOutput.description();
    }

    // 监听音频输出设备变化（如 AirPods 连接/断开）
    QMediaDevices *mediaDevices = new QMediaDevices(this);
    connect(mediaDevices, &QMediaDevices::audioOutputsChanged,
            this, &VoiceManager::onAudioOutputsChanged);

    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged,
            this, &VoiceManager::onPlaybackStateChanged);
}

VoiceManager::~VoiceManager()
{
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

// ==================== 配置加载 ====================

QString VoiceManager::findConfigFilePath() const
{
    QStringList paths;

    // 1. 项目根目录 .env 文件（优先，推荐）
    QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_MACOS
    // macOS app bundle 结构: LocalAIAssistant.app/Contents/MacOS/
    // 需要向上多级查找 .env
    paths << QDir::cleanPath(appDir + "/../../../.env");           // build/LocalAIAssistant.app/Contents/MacOS/../../../.env
    paths << QDir::cleanPath(appDir + "/../../.env");              // Contents/.env 或 build/.env
    paths << QDir::cleanPath(appDir + "/../.env");                 // LocalAIAssistant.app/.env
    paths << QDir::cleanPath(appDir + "/../Resources/.env");       // macOS app bundle Resources
    paths << QDir::cleanPath(appDir + "/../Resources/girlfriend/.env");
#elif defined(Q_OS_WIN)
    // Windows: 可执行文件在 build 目录，资源在同级目录
    paths << QDir::cleanPath(appDir + "/.env");                    // 可执行文件同级
    paths << QDir::cleanPath(appDir + "/../.env");                 // build 目录上级
    paths << QDir::cleanPath(appDir + "/../sourcecode-ai-assistant/.env");
#else
    // Linux: 可执行文件在 build 目录
    paths << QDir::cleanPath(appDir + "/../sourcecode-ai-assistant/.env");
    paths << QDir::cleanPath(appDir + "/.env");
#endif

    // 当前工作目录（开发时常用）
    paths << ".env";
    paths << "../.env";
    paths << "../../.env";
    paths << "../sourcecode-ai-assistant/.env";
    paths << "../../sourcecode-ai-assistant/.env";

    // 2. 用户数据目录 .env 文件
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    paths << QDir::cleanPath(dataDir + "/.env");
    paths << QDir::cleanPath(dataDir + "/girlfriend/.env");

    // 调试输出所有查找路径
    qDebug() << "VoiceManager: Searching for config in paths:";
    for (const QString &p : paths) {
        qDebug() << "  -" << p << (QFile::exists(p) ? "[EXISTS]" : "");
    }

    for (const QString &path : paths) {
        if (QFile::exists(path)) {
            qDebug() << "VoiceManager: Found config file:" << path;
            return path;
        }
    }

    return QString();
}

bool VoiceManager::loadConfig()
{
    // 优先从系统环境变量读取（如果已设置）
    m_appId = qEnvironmentVariable("XFYUN_APP_ID");
    m_apiKey = qEnvironmentVariable("XFYUN_API_KEY");
    m_apiSecret = qEnvironmentVariable("XFYUN_API_SECRET");

    // 如果系统环境变量已设置，直接使用
    if (!m_appId.isEmpty() && !m_apiKey.isEmpty() && !m_apiSecret.isEmpty()) {
        qDebug() << "VoiceManager: Loaded credentials from system environment variables";
        m_asrUrl = qEnvironmentVariable("XFYUN_ASR_URL", "wss://iat-api.xfyun.cn/v2/iat");
        m_ttsUrl = qEnvironmentVariable("XFYUN_TTS_URL", "wss://cbm01.cn-huabei-1.xf-yun.com/v1/private/mcd9m97e6");
        return true;
    }

    // 从配置文件读取（.env 或 voice_config.json）
    QString configPath = findConfigFilePath();
    if (configPath.isEmpty()) {
        qDebug() << "VoiceManager: No config file found (.env or voice_config.json)";
        return false;
    }

    // 判断文件类型
    if (configPath.endsWith(".env")) {
        // 解析 .env 文件
        return loadFromEnvFile(configPath);
    } else {
        // 解析 JSON 配置文件
        return loadFromJsonFile(configPath);
    }
}

bool VoiceManager::loadFromEnvFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "VoiceManager: Cannot open .env file:" << path;
        return false;
    }

    qDebug() << "VoiceManager: Loading credentials from .env:" << path;

    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine()).trimmed();

        // 跳过空行和注释
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        // 解析 KEY=VALUE 格式
        int eqPos = line.indexOf('=');
        if (eqPos <= 0) {
            continue;
        }

        QString key = line.left(eqPos).trimmed();
        QString value = line.mid(eqPos + 1).trimmed();

        // 移除引号（如果有）
        if (value.startsWith('"') && value.endsWith('"')) {
            value = value.mid(1, value.length() - 2);
        }
        if (value.startsWith("'") && value.endsWith("'")) {
            value = value.mid(1, value.length() - 2);
        }

        // 映射到成员变量
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

bool VoiceManager::loadFromJsonFile(const QString &path)
{
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

    m_voiceType = config["voice_type"].toString("xiaoyan");
    m_enableVoiceOutput = config["enable_voice_output"].toBool(true);

    qDebug() << "VoiceManager: AppId:" << m_appId << "ApiKey:" << m_apiKey.left(8) + "...";

    return isConfigured();
}

bool VoiceManager::isConfigured() const
{
    return !m_appId.isEmpty() && !m_apiKey.isEmpty() && !m_apiSecret.isEmpty();
}

void VoiceManager::setVoiceType(const QString &voiceType)
{
    m_voiceType = voiceType;
}

// ==================== 讯飞鉴权 URL 生成 ====================

QString VoiceManager::generateAsrAuthUrl()
{
    return generateAuthUrl(ASR_HOST, ASR_PATH);
}

QString VoiceManager::generateTtsAuthUrl()
{
    return generateAuthUrl(TTS_HOST, TTS_PATH);
}

QString VoiceManager::generateAuthUrl(const QString &host, const QString &path)
{
    // RFC1123 格式时间戳
    QString date = QDateTime::currentDateTimeUtc().toString("ddd, dd MMM yyyy HH:mm:ss") + " GMT";

    // 签名原文: host + date + path
    QString signatureOrigin = "host: " + host + "\ndate: " + date + "\nGET " + path + " HTTP/1.1";

    // HMAC-SHA256 签名
    QByteArray key = m_apiSecret.toUtf8();
    QByteArray data = signatureOrigin.toUtf8();
    QByteArray signature = QMessageAuthenticationCode::hash(data, key, QCryptographicHash::Sha256);

    // Base64 编码签名
    QString signatureBase64 = signature.toBase64();

    // authorization 原文
    QString authorizationOrigin = QString("api_key=\"%1\", algorithm=\"%2\", headers=\"%3\", signature=\"%4\"")
            .arg(m_apiKey)
            .arg("hmac-sha256")
            .arg("host date request-line")
            .arg(signatureBase64);

    // Base64 编码 authorization
    QString authorization = authorizationOrigin.toUtf8().toBase64();

    // 使用 QUrl 正规构建 URL
    QUrl url;
    url.setScheme("wss");
    url.setHost(host);
    url.setPath(path);

    // 添加查询参数
    QUrlQuery query;
    query.addQueryItem("authorization", authorization);
    query.addQueryItem("date", date);
    query.addQueryItem("host", host);
    url.setQuery(query);

    qDebug() << "VoiceManager: Generated auth URL:" << url.toString(QUrl::FullyEncoded).left(100) + "...";

    return url.toString(QUrl::FullyEncoded);
}

// ==================== ASR（语音听写） ====================

void VoiceManager::initAsrWebSocket()
{
    m_asrWebSocket = new QWebSocket();

    // 配置 SSL 以避免认证问题
    QSslConfiguration sslConfig = m_asrWebSocket->sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    m_asrWebSocket->setSslConfiguration(sslConfig);

    // 忽略 SSL 错误
    m_asrWebSocket->ignoreSslErrors();

    connect(m_asrWebSocket, &QWebSocket::connected, this, &VoiceManager::onAsrConnected);
    connect(m_asrWebSocket, &QWebSocket::disconnected, this, &VoiceManager::onAsrDisconnected);
    connect(m_asrWebSocket, &QWebSocket::textMessageReceived, this, &VoiceManager::onAsrTextMessageReceived);
    connect(m_asrWebSocket, &QWebSocket::errorOccurred, this, &VoiceManager::onAsrError);
}

void VoiceManager::startRecording()
{
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

    // 连接 ASR WebSocket
    QString authUrl = generateAsrAuthUrl();
    qDebug() << "VoiceManager: Connecting to ASR:" << authUrl.left(100) + "...";
    m_asrWebSocket->open(QUrl(authUrl));

    // 使用 QAudioSource 进行音频捕获（跨平台支持，包括 Windows）
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

    // 配置目标音频格式：16kHz, 16bit, mono（讯飞要求）
    QAudioFormat desiredFormat;
    desiredFormat.setSampleRate(16000);
    desiredFormat.setChannelCount(1);
    desiredFormat.setSampleFormat(QAudioFormat::Int16);

    // 检查设备是否支持目标格式
    QAudioFormat actualFormat = desiredFormat;
    if (!defaultDevice.isFormatSupported(desiredFormat)) {
        // 获取设备支持的格式中最接近的
        actualFormat = defaultDevice.preferredFormat();
        qDebug() << "VoiceManager: 16kHz not directly supported, using device preferred format:"
                 << actualFormat.sampleRate() << "Hz,"
                 << actualFormat.channelCount() << "channels,"
                 << "will resample later";

        // 确保使用 16-bit 整数格式（便于处理）
        if (actualFormat.sampleFormat() != QAudioFormat::Int16) {
            actualFormat.setSampleFormat(QAudioFormat::Int16);
            if (!defaultDevice.isFormatSupported(actualFormat)) {
                // 如果仍然不支持，尝试 Float 格式
                actualFormat.setSampleFormat(QAudioFormat::Float);
                qDebug() << "VoiceManager: Using Float format instead of Int16";
            }
        }
    } else {
        qDebug() << "VoiceManager: Using 16kHz audio format directly";
    }

    m_audioFormat = actualFormat;
    m_actualSampleRate = actualFormat.sampleRate();

    // 创建 QAudioSource
    m_audioSource = new QAudioSource(defaultDevice, actualFormat, this);
    m_audioIODevice = m_audioSource->start();

    if (!m_audioIODevice) {
        qDebug() << "VoiceManager: Failed to start audio source";
        emit asrError(GTr::voiceNotConfigured());
        m_audioSource->deleteLater();
        m_audioSource = nullptr;
        m_isRecording = false;
        return;
    }

    // 连接 readyRead 信号以接收音频数据
    connect(m_audioIODevice, &QIODevice::readyRead, this, &VoiceManager::onAudioDataReady);

    qDebug() << "VoiceManager: Recording started with QAudioSource"
             << "- sample rate:" << m_actualSampleRate
             << "- format:" << (m_audioFormat.sampleFormat() == QAudioFormat::Int16 ? "Int16" : "Float");
}

void VoiceManager::stopRecording()
{
    if (!m_isRecording) {
        return;
    }

    m_isRecording = false;

    // 停止音频源
    if (m_audioSource) {
        m_audioSource->stop();
        // 读取剩余数据
        if (m_audioIODevice && m_audioIODevice->bytesAvailable() > 0) {
            onAudioDataReady();
        }
    }

    emit recordingStopped();
    emit statusChanged(GTr::recognizing());

    qDebug() << "VoiceManager: Recording stopped, total audio buffer size:" << m_audioBuffer.size();

    // 处理并发送累积的音频数据
    processAndSendAudioData();
}

void VoiceManager::onAsrConnected()
{
    m_asrConnected = true;
    emit statusChanged(GTr::startSpeaking());

    // 发送初始帧（业务参数）
    QJsonObject frame;
    QJsonObject common;
    common["app_id"] = m_appId;
    frame["common"] = common;

    QJsonObject business;
    business["language"] = "zh_cn";
    business["domain"] = "iat";
    business["accent"] = "mandarin";
    business["vad_eos"] = 5000;      // VAD超时从2秒增加到5秒，允许更长的说话停顿
    business["dwa"] = "wpgs";        // 动态修正
    business["ptt"] = 1;             // 标点预测
    // 注意：rhxd 参数已被讯飞API移除，不再支持
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

void VoiceManager::onAsrDisconnected()
{
    m_asrConnected = false;
    qDebug() << "VoiceManager: ASR WebSocket disconnected";
}

void VoiceManager::onAsrTextMessageReceived(const QString &message)
{
    qDebug() << "VoiceManager: ASR received message:" << message.left(200);
    parseAsrResponse(message);
}

void VoiceManager::onAsrError(QAbstractSocket::SocketError error)
{
    QString errorMsg = GTr::asrConnectionError(m_asrWebSocket->errorString());
    qDebug() << "VoiceManager:" << errorMsg;
    emit asrError(errorMsg);
    emit statusChanged(errorMsg);
}

void VoiceManager::sendAudioFrame(const QByteArray &audioData)
{
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

void VoiceManager::sendAsrEndSignal()
{
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

void VoiceManager::parseAsrResponse(const QString &jsonResponse)
{
    QJsonDocument doc = QJsonDocument::fromJson(jsonResponse.toUtf8());
    if (!doc.isObject()) {
        qDebug() << "VoiceManager: ASR response is not JSON object";
        return;
    }

    QJsonObject response = doc.object();

    // 检查错误
    if (response.contains("code") && response["code"].toInt() != 0) {
        QString error = GTr::asrConnectionError(QString("[%1]: %2")
                .arg(response["code"].toInt())
                .arg(response["message"].toString()));
        emit asrError(error);
        return;
    }

    // 解析识别结果
    QJsonObject data = response["data"].toObject();
    if (data.isEmpty()) {
        qDebug() << "VoiceManager: ASR data is empty";
        return;
    }

    int status = data["status"].toInt();

    // result 是对象，包含 ws 数组
    QJsonObject resultObj = data["result"].toObject();
    QJsonArray wsArray = resultObj["ws"].toArray();

    QString resultText;
    for (const QJsonValue &wsVal : wsArray) {
        QJsonObject ws = wsVal.toObject();
        QJsonArray cwArray = ws["cw"].toArray();
        for (const QJsonValue &cwVal : cwArray) {
            QJsonObject cw = cwVal.toObject();
            resultText += cw["w"].toString();
        }
    }

    qDebug() << "VoiceManager: ASR parsed text:" << resultText << "status:" << status;

    // 处理结果
    if (!resultText.isEmpty()) {
        // 检查 pgs (pgs="apd" 表示追加, pgs="rpl" 表示替换)
        QString pgs = resultObj["pgs"].toString();

        if (pgs == "rpl") {
            // 替换之前的结果
            m_asrPartialText = resultText;
        } else {
            // 追加结果
            m_asrPartialText += resultText;
        }

        emit asrPartialResult(m_asrPartialText);

        // status=2 表示最终结果
        if (status == 2) {
            m_asrFinalText = m_asrPartialText;
            emit asrFinalResult(m_asrFinalText);
            emit statusChanged(GTr::recognitionComplete());

            qDebug() << "VoiceManager: ASR final result:" << m_asrFinalText;
        }
    }
}

void VoiceManager::processAndSendAudioData()
{
    // 检查 WebSocket 是否已连接
    if (!m_asrConnected) {
        qDebug() << "VoiceManager: ASR WebSocket not connected yet, waiting...";
        // 等待连接建立（最多等待 3 秒）
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

    // 处理 Float 格式转换（如果需要）
    if (m_audioFormat.sampleFormat() == QAudioFormat::Float) {
        qDebug() << "VoiceManager: Converting Float to Int16";
        // Float (32-bit) to Int16 (16-bit) conversion
        int sampleCount = audioData.size() / 4;  // 4 bytes per float sample
        QByteArray int16Data;
        int16Data.reserve(sampleCount * 2);

        const float *floatData = reinterpret_cast<const float*>(audioData.constData());
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

    // 如果采样率不是 16kHz，需要重采样
    if (m_actualSampleRate != 16000 && m_actualSampleRate > 16000) {
        int ratio = m_actualSampleRate / 16000;
        qDebug() << "VoiceManager: Resampling from" << m_actualSampleRate << "Hz to 16000 Hz (ratio:" << ratio << ")";

        // 简单降采样：每隔 ratio 个采样取一个
        // 16-bit PCM: 每个采样 2 bytes
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

    // 音频增益处理 - 提高音量1.5倍（解决录音音量小的问题）
    // 16-bit PCM: 每个采样是 signed short
    int bytesPerSample = 2;  // 16-bit = 2 bytes
    int sampleCount = audioData.size() / bytesPerSample;
    float gainFactor = 1.5f;  // 增益因子

    for (int i = 0; i < sampleCount; i++) {
        int offset = i * bytesPerSample;
        if (offset + 2 <= audioData.size()) {
            // 读取16-bit采样值
            short sample = *reinterpret_cast<const short*>(audioData.constData() + offset);
            // 应用增益
            float amplified = sample * gainFactor;
            // 防止溢出（clip到-32768到32767范围）
            if (amplified > 32767) amplified = 32767;
            if (amplified < -32768) amplified = -32768;
            // 写回
            short newSample = static_cast<short>(amplified);
            audioData[offset] = reinterpret_cast<char*>(&newSample)[0];
            audioData[offset + 1] = reinterpret_cast<char*>(&newSample)[1];
        }
    }
    qDebug() << "VoiceManager: Applied audio gain factor:" << gainFactor;

    // 讯飞要求的格式：16kHz, 16bit, mono
    QString formatStr = "audio/L16;rate=16000";

    // 分帧发送（16kHz: 1280 bytes = 40ms）
    int frameSize = 1280;
    int offset = 0;
    int framesSent = 0;
    while (offset < audioData.size()) {
        QByteArray frameData = audioData.mid(offset, frameSize);

        // 发送音频帧
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

    // 发送结束帧
    sendAsrEndSignal();

    // 清理音频源
    if (m_audioSource) {
        m_audioSource->deleteLater();
        m_audioSource = nullptr;
        m_audioIODevice = nullptr;
    }
}

void VoiceManager::onAudioDataReady()
{
    if (!m_audioIODevice || !m_isRecording) {
        return;
    }

    // 读取所有可用的音频数据
    QByteArray newData = m_audioIODevice->readAll();
    if (newData.isEmpty()) {
        return;
    }

    // 累积到缓冲区
    m_audioBuffer.append(newData);

    qDebug() << "VoiceManager: Read" << newData.size() << "bytes, total buffer:" << m_audioBuffer.size();
}

// ==================== TTS（语音合成） ====================

void VoiceManager::initTtsWebSocket()
{
    m_ttsWebSocket = new QWebSocket();

    // 配置 SSL
    QSslConfiguration sslConfig = m_ttsWebSocket->sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    m_ttsWebSocket->setSslConfiguration(sslConfig);
    m_ttsWebSocket->ignoreSslErrors();

    connect(m_ttsWebSocket, &QWebSocket::connected, this, &VoiceManager::onTtsConnected);
    connect(m_ttsWebSocket, &QWebSocket::disconnected, this, &VoiceManager::onTtsDisconnected);
    connect(m_ttsWebSocket, &QWebSocket::binaryMessageReceived, this, &VoiceManager::onTtsBinaryMessageReceived);
    connect(m_ttsWebSocket, &QWebSocket::textMessageReceived, this, &VoiceManager::onTtsTextMessageReceived);
    connect(m_ttsWebSocket, &QWebSocket::errorOccurred, this, &VoiceManager::onTtsError);
}

void VoiceManager::speak(const QString &text)
{
    if (!isConfigured()) {
        emit ttsError(GTr::xunfeiCredentialsNotConfigured());
        return;
    }

    if (text.isEmpty()) {
        return;
    }

    // 如果正在播放，先停止
    if (m_isSpeaking) {
        stopSpeaking();
    }

    // 不在这里设置 m_isSpeaking，等实际播放时再设置
    m_ttsAudioBuffer.clear();
    m_ttsText = text;  // 保存要合成的文本

    emit statusChanged(GTr::synthesizingVoice());

    // 连接 TTS WebSocket
    QString authUrl = generateTtsAuthUrl();
    qDebug() << "VoiceManager: Connecting to TTS:" << authUrl.left(100) + "...";
    m_ttsWebSocket->open(QUrl(authUrl));
}

void VoiceManager::stopSpeaking()
{
    if (!m_isSpeaking) {
        return;
    }

    m_isSpeaking = false;

    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
    }

    if (m_ttsWebSocket && m_ttsConnected) {
        m_ttsWebSocket->close();
    }

    emit speakingFinished();
    emit statusChanged(GTr::voiceStopped());
}

void VoiceManager::onTtsConnected()
{
    m_ttsConnected = true;

    // 发送 TTS 请求（使用保存的文本）
    sendTtsRequest(m_ttsText);
}

void VoiceManager::onTtsDisconnected()
{
    m_ttsConnected = false;
    qDebug() << "VoiceManager: TTS WebSocket disconnected, audio buffer size:" << m_ttsAudioBuffer.size();

    // 不在这里播放，已在 onTtsTextMessageReceived 中处理
    // 清空缓冲区，准备下一次合成
    m_ttsAudioBuffer.clear();
}

void VoiceManager::onTtsBinaryMessageReceived(const QByteArray &message)
{
    // 讯飞 TTS 返回二进制音频数据
    qDebug() << "VoiceManager: TTS received binary data:" << message.size() << "bytes";
    m_ttsAudioBuffer.append(message);
}

void VoiceManager::onTtsTextMessageReceived(const QString &message)
{
    // 解析 TTS JSON 响应（音频数据在 data.audio 字段，base64 编码）
    qDebug() << "VoiceManager: TTS received text message:" << message.left(100);

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        qDebug() << "VoiceManager: TTS response is not JSON object";
        return;
    }

    QJsonObject response = doc.object();
    int code = response["code"].toInt();
    if (code != 0) {
        QString error = GTr::ttsErrorWithCode(code, response["message"].toString());
        emit ttsError(error);
        return;
    }

    // 提取音频数据（base64 编码）
    QJsonObject data = response["data"].toObject();
    QString audioBase64 = data["audio"].toString();

    if (!audioBase64.isEmpty()) {
        // 解码 base64 并追加到音频缓冲区
        QByteArray audioData = QByteArray::fromBase64(audioBase64.toUtf8());
        m_ttsAudioBuffer.append(audioData);
        qDebug() << "VoiceManager: TTS decoded audio:" << audioData.size() << "bytes, total buffer:" << m_ttsAudioBuffer.size();
    }

    // 检查合成状态
    int status = data["status"].toInt();
    if (status == 2) {
        // 合成完成，播放音频
        qDebug() << "VoiceManager: TTS synthesis complete, total audio:" << m_ttsAudioBuffer.size() << "bytes";
        emit statusChanged(GTr::voiceSynthesisComplete());

        if (!m_ttsAudioBuffer.isEmpty()) {
            // 保存音频数据并播放
            QByteArray audioToPlay = m_ttsAudioBuffer;
            m_ttsAudioBuffer.clear();  // 清空缓冲区，避免重复播放
            playTtsAudio(audioToPlay);
        }
    }
}

void VoiceManager::onTtsError(QAbstractSocket::SocketError error)
{
    QString errorMsg = GTr::ttsConnectionError(m_ttsWebSocket->errorString());
    qDebug() << "VoiceManager:" << errorMsg;
    emit ttsError(errorMsg);
    emit statusChanged(errorMsg);
}

void VoiceManager::sendTtsRequest(const QString &text)
{
    if (!m_ttsConnected || text.isEmpty()) {
        return;
    }

    QJsonObject frame;
    QJsonObject common;
    common["app_id"] = m_appId;
    frame["common"] = common;

    QJsonObject business;
    // MP3 格式参数 (讯飞 TTS WebSocket API)
    business["aue"] = "lame";           // 音频编码：lame = MP3
    business["sfl"] = 1;                 // 开启流式返回
    business["auf"] = "audio/L16;rate=16000";  // 音频格式
    business["vcn"] = m_voiceType;       // 发音人
    business["tte"] = "UTF8";            // 文本编码格式，必须与实际编码一致
    business["speed"] = 50;              // 语速 (0-100)
    business["volume"] = 50;             // 音量 (0-100)
    business["pitch"] = 50;              // 音高 (0-100)
    business["bgs"] = 0;                 // 背景音
    business["ent"] = "ent_tts_cn";      // TTS 引擎类型
    frame["business"] = business;

    QJsonObject data;
    data["status"] = 2;                  // 一次性发送全部文本
    // 文本需要 UTF-8 编码后 base64
    QByteArray textBytes = text.toUtf8();
    QString textBase64 = QString(textBytes.toBase64());
    data["text"] = textBase64;
    frame["data"] = data;

    QString jsonFrame = QJsonDocument(frame).toJson(QJsonDocument::Compact);
    m_ttsWebSocket->sendTextMessage(jsonFrame);

    // 详细调试：打印编码信息
    qDebug() << "VoiceManager: Sent TTS request";
    qDebug() << "  - Original text:" << text;
    qDebug() << "  - UTF-8 bytes:" << textBytes.size() << "[" << textBytes.left(20) << "...]";
    qDebug() << "  - Base64:" << textBase64.left(30) << "...";
    qDebug() << "  - Decode test:" << QByteArray::fromBase64(textBase64.toUtf8());
}

void VoiceManager::parseTtsResponse(const QByteArray &binaryData, const QString &jsonMeta)
{
    // 累积音频数据
    if (!binaryData.isEmpty()) {
        m_ttsAudioBuffer.append(binaryData);
    }

    // 解析元数据（如果返回了文本消息）
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
                // 合成完成
                emit statusChanged(GTr::voiceSynthesisComplete());
            }
        }
    }
}

void VoiceManager::playTtsAudio(const QByteArray &audioData)
{
    if (audioData.isEmpty()) {
        qDebug() << "VoiceManager: TTS audio data is empty, cannot play";
        emit ttsError(GTr::audioDataEmpty());
        return;
    }

    // TTS 返回的是 MP3 格式音频（因为 aue = "lame"），直接保存为 MP3 文件
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempFile = tempPath + "/tts_output_" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss") + ".mp3";

    QFile file(tempFile);
    if (!file.open(QIODevice::WriteOnly)) {
        emit ttsError(GTr::cannotCreateAudioFile());
        return;
    }

    // 直接写入 MP3 数据
    file.write(audioData);
    file.flush();
    file.close();

    // 验证文件存在
    if (!QFile::exists(tempFile)) {
        qDebug() << "VoiceManager: MP3 file not found after write:" << tempFile;
        emit ttsError(GTr::audioFileCreateFailed());
        return;
    }

    qDebug() << "VoiceManager: MP3 file created:" << tempFile << "size:" << audioData.size();

    // 清理旧的临时文件
    if (m_ttsTempFile) {
        m_ttsTempFile->remove();
        m_ttsTempFile->deleteLater();
        m_ttsTempFile = nullptr;
    }

    // 设置播放状态
    m_isSpeaking = true;
    emit speakingStarted();

    // 播放前确保使用当前默认输出设备（双重保障）
    QAudioDevice currentDefault = QMediaDevices::defaultAudioOutput();
    if (!currentDefault.isNull() && m_audioOutput->device() != currentDefault) {
        qDebug() << "VoiceManager: Switching to current default output:" << currentDefault.description();
        m_audioOutput->setDevice(currentDefault);
    }

    // 使用 QMediaPlayer 播放
    m_ttsTempFile = new QFile(tempFile, this);
    QUrl audioUrl = QUrl::fromLocalFile(tempFile);

    qDebug() << "VoiceManager: Playing WAV from:" << audioUrl.toString();

    m_mediaPlayer->setSource(audioUrl);
    m_mediaPlayer->play();

    emit statusChanged(GTr::playingVoice());
}

void VoiceManager::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::StoppedState) {
        m_isSpeaking = false;
        emit speakingFinished();
        emit statusChanged(GTr::playbackComplete());

        // 清理临时文件
        if (m_ttsTempFile) {
            m_ttsTempFile->remove();
            m_ttsTempFile->deleteLater();
            m_ttsTempFile = nullptr;
        }
    }
}

void VoiceManager::onAudioOutputsChanged()
{
    // 音频输出设备变化（如 AirPods 连接/断开），切换到新的默认输出设备
    QAudioDevice newDefault = QMediaDevices::defaultAudioOutput();
    if (!newDefault.isNull()) {
        qDebug() << "VoiceManager: Audio output device changed, switching to:" << newDefault.description();
        m_audioOutput->setDevice(newDefault);
    } else {
        qDebug() << "VoiceManager: No audio output device available after change";
    }
}