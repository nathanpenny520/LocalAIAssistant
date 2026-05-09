#pragma once

#ifndef GIRLFRIEND_TRANSLATIONS_H
#define GIRLFRIEND_TRANSLATIONS_H

#include <QCoreApplication>
#include <QString>

/**
 * AI女友模块翻译辅助类
 * 统一管理所有需要翻译的字符串
 */
class GTr {
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
    static QString asrConnectionError(const QString& details) {
        return tr("ASR connection error: %1").arg(details);
    }

    static QString ttsErrorWithCode(int code, const QString& msg) {
        return tr("TTS error [%1]: %2").arg(code).arg(msg);
    }

    static QString ttsConnectionError(const QString& details) {
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

    static QString configureVoice() {
        return tr("Configure Voice...");
    }

    static QString clearHistory() {
        return tr("Clear History");
    }

    // Voice configuration dialog
    static QString voiceConfigTitle() {
        return tr("Voice Configuration");
    }
    static QString voiceConfigDescription() {
        return tr(
                "Configure Xunfei (iFlytek) voice service credentials. Register at "
                "https://www.xfyun.cn to get your APP ID, API Key, and API Secret.");
    }
    static QString voiceConfigAppId() {
        return tr("APP ID");
    }
    static QString voiceConfigApiKey() {
        return tr("API Key");
    }
    static QString voiceConfigApiSecret() {
        return tr("API Secret");
    }
    static QString voiceConfigAsrUrl() {
        return tr("ASR URL (optional)");
    }
    static QString voiceConfigTtsUrl() {
        return tr("TTS URL (optional)");
    }
    static QString voiceConfigVoiceType() {
        return tr("Voice Type (optional)");
    }
    static QString voiceConfigSave() {
        return tr("Save");
    }
    static QString voiceConfigCancel() {
        return tr("Cancel");
    }
    static QString voiceConfigSaved() {
        return tr("Voice credentials saved successfully.");
    }
    static QString voiceConfigMissingFields() {
        return tr("APP ID, API Key, and API Secret are required.");
    }
    static QString voiceConfigTestHint() {
        return tr("After saving, restart voice interaction to apply new credentials.");
    }
    static QString voiceConfigOptionalHint() {
        return tr("URL and Voice Type are optional — leave blank to use built-in defaults.");
    }

    // Session management
    static QString sessionsLabel() {
        return tr("Sessions");
    }
    static QString newSession() {
        return tr("New Session");
    }
    static QString deleteSession() {
        return tr("Delete");
    }
    static QString deleteSessionConfirmTitle() {
        return tr("Delete Session");
    }
    static QString deleteSessionConfirmMessage(const QString& name) {
        return tr("Delete \"%1\"? This cannot be undone.").arg(name);
    }
    static QString currentSessionLabel() {
        return tr("current");
    }
    static QString sessionDefaultName(int n) {
        return tr("Session %1").arg(n);
    }
    static QString selectSessionToDelete() {
        return tr("Select session to delete:");
    }
    static QString cannotDeleteOnlySession() {
        return tr("Cannot delete the only session.");
    }
    static QString cancelButton() {
        return tr("Cancel");
    }

    // Level & Mood
    static QString avatarLevelLabel() {
        return tr("Avatar Level");
    }
    static QString moodInfluenceLabel() {
        return tr("Mood Influence");
    }
    static QString moodLow() {
        return tr("Low");
    }
    static QString moodMedium() {
        return tr("Med");
    }
    static QString moodHigh() {
        return tr("High");
    }

    // Video
    static QString videoSoundLabel() {
        return tr("Video Sound");
    }
    static QString videoSoundOn() {
        return tr("On");
    }
    static QString videoSoundOff() {
        return tr("Off");
    }

    // Mood display
    static QString moodLabel() {
        return tr("Mood");
    }

    // New emotions
    static QString emotionCrying() {
        return tr("😢 Crying");
    }
    static QString emotionTravelling() {
        return tr("✈️ Travelling");
    }

    // Manage conversations dialog
    static QString manageConversations() {
        return tr("Manage Conversations");
    }
    static QString selectSessionToManage() {
        return tr("Select session to manage:");
    }
    static QString renameLabel() {
        return tr("Rename");
    }
    static QString pinLabel() {
        return tr("Pin");
    }
    static QString unpinLabel() {
        return tr("Unpin");
    }
    static QString deleteLabel() {
        return tr("Delete");
    }
    static QString closeButton() {
        return tr("Close");
    }

private:
    static QString tr(const char* text) {
        return QCoreApplication::translate("GTr", text);
    }
};

#endif  // GIRLFRIEND_TRANSLATIONS_H