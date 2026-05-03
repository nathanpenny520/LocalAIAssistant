#ifndef GIRLFRIEND_TRANSLATIONS_H
#define GIRLFRIEND_TRANSLATIONS_H

#include <QString>
#include <QCoreApplication>

/**
 * AI女友模块翻译辅助类
 * 统一管理所有需要翻译的字符串
 */
class GTr
{
public:
    // GirlfriendWindow 界面文本
    static QString windowTitle() {
        return tr("AI Girlfriend - Xiaoqing");
    }

    static QString inputPlaceholder() {
        return tr("Type a message...");
    }

    static QString sendButton() {
        return tr("Send");
    }

    static QString waitingButton() {
        return tr("Waiting...");
    }

    static QString replyingPlaceholder() {
        return tr("Replying...");
    }

    static QString thinking() {
        return tr("Thinking...");
    }

    static QString clearConfirmTitle() {
        return tr("Clear Conversation");
    }

    static QString clearConfirmMessage() {
        return tr("Clear all conversation history?");
    }

    static QString yesButton() {
        return tr("Yes");
    }

    static QString noButton() {
        return tr("No");
    }

    // VoiceManager 状态文本
    static QString connectingVoiceService() {
        return tr("Connecting voice service...");
    }

    static QString voiceServiceConnected() {
        return tr("Voice service connected");
    }

    static QString startSpeaking() {
        return tr("Start speaking...");
    }

    static QString recognizing() {
        return tr("Recognizing...");
    }

    static QString recognitionComplete() {
        return tr("Recognition complete");
    }

    static QString synthesizingVoice() {
        return tr("Synthesizing voice...");
    }

    static QString voiceSynthesisComplete() {
        return tr("Voice synthesis complete");
    }

    static QString playingVoice() {
        return tr("Playing voice...");
    }

    static QString playbackComplete() {
        return tr("Playback complete");
    }

    static QString voiceStopped() {
        return tr("Voice stopped");
    }

    static QString voiceNotConfigured() {
        return tr("Voice not configured");
    }

    static QString xunfeiCredentialsNotConfigured() {
        return tr("Xunfei credentials not configured");
    }

    static QString audioDataEmpty() {
        return tr("Audio data empty");
    }

    static QString cannotCreateAudioFile() {
        return tr("Cannot create audio file");
    }

    static QString audioFileCreateFailed() {
        return tr("Audio file creation failed");
    }

    // AvatarWidget 情绪标签
    static QString emotionDefault() {
        return tr("💕 Default");
    }

    static QString emotionHappy() {
        return tr("💕 Happy");
    }

    static QString emotionShy() {
        return tr("☺️ Shy");
    }

    static QString emotionLove() {
        return tr("💖 Love");
    }

    static QString emotionHate() {
        return tr("😒 Dislike");
    }

    static QString emotionSad() {
        return tr("😢 Sad");
    }

    static QString emotionAngry() {
        return tr("😤 Angry");
    }

    static QString emotionAfraid() {
        return tr("😟 Afraid");
    }

    static QString emotionAwaiting() {
        return tr("😊 Expecting");
    }

    static QString emotionSpeaking() {
        return tr("🫦 Speaking");
    }

    static QString emotionStudying() {
        return tr("📚 Thinking");
    }

    static QString emotionWorried() {
        return tr("😟 Worried");
    }

    // GirlfriendWindow 其他文本
    static QString voiceNotConfiguredTooltip() {
        return tr("Voice not configured, please set environment variables");
    }

    static QString voiceInputTooltip() {
        return tr("Click to start voice input");
    }

    static QString errorPrefix() {
        return tr("Error: ");
    }

    // VoiceManager 错误消息
    static QString asrConnectionError(const QString &details) {
        return tr("ASR connection error: %1").arg(details);
    }

    static QString ttsErrorWithCode(int code, const QString &msg) {
        return tr("TTS error [%1]: %2").arg(code).arg(msg);
    }

    static QString ttsConnectionError(const QString &details) {
        return tr("TTS connection error: %1").arg(details);
    }

    // Settings menu
    static QString settingsButton() {
        return tr("⚙️");
    }

    static QString voiceOutputEnabled() {
        return tr("Voice Output: On");
    }

    static QString voiceOutputDisabled() {
        return tr("Voice Output: Off");
    }

    static QString clearHistory() {
        return tr("Clear History");
    }

private:
    static QString tr(const char *text) {
        return QCoreApplication::translate("GirlfriendModule", text);
    }
};

#endif // GIRLFRIEND_TRANSLATIONS_H