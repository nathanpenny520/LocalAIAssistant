#ifndef VOICEMANAGER_H
#define VOICEMANAGER_H

#include <QObject>
#include <QString>
#include <QWebSocket>
#include <QAudioSource>
#include <QIODevice>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QBuffer>
#include <QFile>
#include <QAudioOutput>
#include <QAudioFormat>

/**
 * 语音管理器
 *
 * 支持讯飞语音服务：
 * - ASR（语音转文字）：WebSocket 流式听写
 * - TTS（文字转语音）：WebSocket 流式合成
 *
 * 凭证通过环境变量或配置文件管理，不硬编码
 */
class VoiceManager : public QObject
{
    Q_OBJECT

public:
    explicit VoiceManager(QObject *parent = nullptr);
    ~VoiceManager();

    // 加载配置（从文件或环境变量）
    bool loadConfig();

    // 检查凭证是否已配置
    bool isConfigured() const;

    // ASR：开始/停止录音
    void startRecording();
    void stopRecording();
    bool isRecording() const { return m_isRecording; }

    // TTS：合成并播放
    void speak(const QString &text);
    void stopSpeaking();
    bool isSpeaking() const { return m_isSpeaking; }

    // 流式 TTS：分段发送文本
    void startStreamingTts();
    void sendStreamingText(const QString &text);
    void finishStreamingTts();
    bool isStreamingTts() const { return m_ttsStreaming; }

    // 设置
    void setVoiceType(const QString &voiceType);
    void setEnableVoiceOutput(bool enable) { m_enableVoiceOutput = enable; }
    bool enableVoiceOutput() const { return m_enableVoiceOutput; }

signals:
    // ASR 信号
    void recordingStarted();
    void recordingStopped();
    void asrPartialResult(const QString &text);   // 中间结果（实时显示）
    void asrFinalResult(const QString &text);     // 最终结果
    void asrError(const QString &error);

    // TTS 信号
    void speakingStarted();
    void speakingFinished();
    void ttsError(const QString &error);

    // 状态信号
    void statusChanged(const QString &status);

private slots:
    // WebSocket 回调
    void onAsrConnected();
    void onAsrDisconnected();
    void onAsrTextMessageReceived(const QString &message);
    void onAsrError(QAbstractSocket::SocketError error);

    void onTtsConnected();
    void onTtsDisconnected();
    void onTtsBinaryMessageReceived(const QByteArray &message);
    void onTtsTextMessageReceived(const QString &message);
    void onTtsError(QAbstractSocket::SocketError error);

    // 音频录制回调
    void onAudioDataReady();            // 从 QAudioSource 读取音频数据

    // 播放回调
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

    // 音频设备变化回调
    void onAudioOutputsChanged();

private:
    // 初始化 WebSocket
    void initAsrWebSocket();
    void initTtsWebSocket();

    // 讯飞鉴权 URL 生成
    QString generateAsrAuthUrl();
    QString generateTtsAuthUrl();
    QString generateAuthUrl(const QString &host, const QString &path);
    QByteArray generateSignature(const QString &date, const QString &host, const QString &path);

    // 发送音频数据到 ASR
    void sendAudioFrame(const QByteArray &audioData);
    void sendAsrEndSignal();
    void processAndSendAudioData();     // 处理累积的音频数据并发送到 ASR

    // 发送文本到 TTS
    void sendTtsRequest(const QString &text);

    // 流式 TTS：发送第一帧（status=0）
    void sendStreamingFirstFrame();

    // 解析讯飞响应
    void parseAsrResponse(const QString &jsonResponse);
    void parseTtsResponse(const QByteArray &binaryData, const QString &jsonMeta);

    // 保存音频到临时文件并播放
    void playTtsAudio(const QByteArray &audioData);

    // 查找配置文件路径
    QString findConfigFilePath() const;

    // 从不同格式加载配置
    bool loadFromEnvFile(const QString &path);
    bool loadFromJsonFile(const QString &path);

private:
    // 讯飞凭证（从环境变量/配置文件加载）
    QString m_appId;
    QString m_apiKey;
    QString m_apiSecret;
    QString m_asrUrl;
    QString m_ttsUrl;

    // 语音设置
    QString m_voiceType;          // 超拟人默认发音人 x6_wumeinv_pro
    bool m_enableVoiceOutput;     // 是否自动播放TTS

    // ASR WebSocket
    QWebSocket *m_asrWebSocket;
    bool m_asrConnected;

    // TTS WebSocket
    QWebSocket *m_ttsWebSocket;
    bool m_ttsConnected;

    // 音频录制 (Qt 6 QAudioSource - 跨平台纯音频捕获)
    QAudioSource *m_audioSource;
    QIODevice *m_audioIODevice;         // 用于读取音频数据
    QByteArray m_audioBuffer;           // 累积音频数据
    QAudioFormat m_audioFormat;         // 实际使用的音频格式
    int m_actualSampleRate;             // 实际采样率（用于重采样）
    bool m_isRecording;

    // TTS 音频播放
    QMediaPlayer *m_mediaPlayer;
    QAudioOutput *m_audioOutput;
    QByteArray m_ttsAudioBuffer;
    QFile *m_ttsTempFile;
    bool m_isSpeaking;

    // TTS 待合成文本
    QString m_ttsText;

    // ASR 状态
    QString m_asrPartialText;
    QString m_asrFinalText;
    int m_asrFrameIndex;

    // 流式 TTS 状态
    int m_ttsSeq;                   // 流式文本序号
    bool m_ttsStreaming;            // 是否正在流式合成
    QString m_ttsStreamingBuffer;   // 流式文本缓冲区
};

#endif // VOICEMANAGER_H