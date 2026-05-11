#pragma once

#ifndef VOICEMANAGER_H
#define VOICEMANAGER_H

#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioSource>
#include <QBuffer>
#include <QFile>
#include <QIODevice>
#include <QMap>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QWebSocket>

/**
 * @brief Voice manager for Xunfei (iFlytek) speech services.
 *
 * Provides:
 * - ASR (speech-to-text): WebSocket streaming dictation
 * - TTS (text-to-speech): WebSocket streaming synthesis
 *
 * Credentials are loaded from environment variables or config files — never hardcoded.
 */
class VoiceManager : public QObject {
    Q_OBJECT

public:
    explicit VoiceManager(QObject* parent = nullptr);
    ~VoiceManager();

    bool loadConfig();
    bool isConfigured() const;

    void startRecording();
    void stopRecording();
    bool isRecording() const {
        return m_isRecording;
    }

    void speak(const QString& text);
    void stopSpeaking();
    bool isSpeaking() const {
        return m_isSpeaking;
    }

    void startStreamingTts();
    void sendStreamingText(const QString& text);
    void finishStreamingTts();
    bool isStreamingTts() const {
        return m_ttsStreaming;
    }

    void setVoiceType(const QString& voiceType);
    void setEnableVoiceOutput(bool enable) {
        m_enableVoiceOutput = enable;
    }
    bool enableVoiceOutput() const {
        return m_enableVoiceOutput;
    }

signals:
    // ASR signals
    void recordingStarted();
    void recordingStopped();
    void asrPartialResult(const QString& text);  // intermediate result (real-time display)
    void asrFinalResult(const QString& text);    // final result
    void asrError(const QString& error);

    // TTS signals
    void speakingStarted();
    void speakingFinished();
    void ttsError(const QString& error);

    // Status signal
    void statusChanged(const QString& status);

private slots:
    // WebSocket callbacks
    void onAsrConnected();
    void onAsrDisconnected();
    void onAsrTextMessageReceived(const QString& message);
    void onAsrError(QAbstractSocket::SocketError error);

    void onTtsConnected();
    void onTtsDisconnected();
    void onTtsBinaryMessageReceived(const QByteArray& message);
    void onTtsTextMessageReceived(const QString& message);
    void onTtsError(QAbstractSocket::SocketError error);

    // Audio recording callbacks
    void onAudioDataReady();
    void onAudioPollTimeout();  // timer-based polling (Windows compatibility)

    // Playback callback
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);

    // Audio device change callback
    void onAudioOutputsChanged();

private:
    void initAsrWebSocket();
    void initTtsWebSocket();

    // Xunfei auth URL generation
    QString generateAsrAuthUrl();
    QString generateTtsAuthUrl();
    QString generateAuthUrl(const QString& host, const QString& path);
    QByteArray generateSignature(const QString& date, const QString& host, const QString& path);

    void sendAudioFrame(const QByteArray& audioData);
    void sendAsrEndSignal();
    void processAndSendAudioData();

    void sendTtsRequest(const QString& text);

    // Streaming TTS: send initial frame (status=0)
    void sendStreamingFirstFrame();

    void parseAsrResponse(const QString& jsonResponse);
    void parseTtsResponse(const QByteArray& binaryData, const QString& jsonMeta);

    // Save audio to temp file and play
    void playTtsAudio(const QByteArray& audioData);

    void applyEnvVars(const QMap<QString, QString>& vars);

    bool loadFromJsonFile(const QString& path);

private:
    // Xunfei credentials (loaded from env vars / config file)
    QString m_appId;
    QString m_apiKey;
    QString m_apiSecret;
    QString m_asrUrl;
    QString m_ttsUrl;

    // Voice settings
    QString m_voiceType;       // default super-realistic voice: x6_lingxiaoxuan_pro
    bool m_enableVoiceOutput;  // whether to auto-play TTS audio

    // ASR WebSocket
    QWebSocket* m_asrWebSocket;
    bool m_asrConnected;

    // TTS WebSocket
    QWebSocket* m_ttsWebSocket;
    bool m_ttsConnected;

    // Audio recording (Qt 6 QAudioSource — cross-platform audio capture)
    QAudioSource* m_audioSource;
    QIODevice* m_audioIODevice;  // for reading audio data
    QByteArray m_audioBuffer;    // accumulated audio data
    QAudioFormat m_audioFormat;  // actual audio format in use
    int m_actualSampleRate;      // actual sample rate (for resampling)
    int m_actualChannelCount;    // actual channel count (for channel conversion)
    QTimer* m_audioPollTimer;    // audio polling timer (Windows compatibility)
    bool m_isRecording;

    // TTS audio playback
    QMediaPlayer* m_mediaPlayer;
    QAudioOutput* m_audioOutput;
    QByteArray m_ttsAudioBuffer;
    QFile* m_ttsTempFile;
    bool m_isSpeaking;

    // Pending TTS text
    QString m_ttsText;

    // ASR state
    QString m_asrPartialText;
    QString m_asrFinalText;
    int m_asrFrameIndex;

    // Streaming TTS state
    int m_ttsSeq;                  // streaming text sequence number
    bool m_ttsStreaming;           // whether streaming synthesis is active
    QString m_ttsStreamingBuffer;  // streaming text buffer
};

#endif  // VOICEMANAGER_H