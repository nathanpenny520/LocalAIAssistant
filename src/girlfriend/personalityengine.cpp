#include "personalityengine.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QTime>

#include "../prompts/promptmanager.h"

PersonalityEngine::PersonalityEngine(QObject* parent)
        : QObject(parent), m_personalityPrompt(), m_userNickname("你") {
    loadFromFile();
}

void PersonalityEngine::loadFromFile() {
    m_personalityPrompt = PromptManager::instance()->girlfriendPrompt();

    if (m_personalityPrompt.isEmpty()) {
        m_personalityPrompt = QStringLiteral("你是小清，一个温柔体贴、有知性陪伴感的AI女友。");
    } else {
        parseTemplateConfig();
    }
}

void PersonalityEngine::parseTemplateConfig() {
    m_templateConfig = PromptManager::instance()->girlfriendConfig();
}

QString PersonalityEngine::templateValue(const QString& key, const QString& fallback) const {
    return m_templateConfig.value(key, fallback);
}

QString PersonalityEngine::loadPersonalityPrompt() {
    if (m_personalityPrompt.isEmpty()) {
        loadFromFile();
    }
    return m_personalityPrompt;
}

QString PersonalityEngine::buildSystemPrompt(const QString& memoryContent) {
    QString prompt = m_personalityPrompt;

    prompt.replace(QStringLiteral("{{user_nickname}}"), m_userNickname);

    // Mood hint — read from personality.md config; fall back to code defaults if missing
    QString moodHint = getMoodHint();
    if (moodHint.isEmpty()) {
        prompt.remove(QStringLiteral("\n{{mood_hint}}"));
    } else {
        prompt.replace(QStringLiteral("{{mood_hint}}"),
                       templateValue(QStringLiteral("mood_prefix"), QStringLiteral("当前心情：")) +
                               moodHint + QLatin1Char('\n'));
    }

    // Time-of-day context
    QTime now = QTime::currentTime();
    int hour = now.hour();
    QString timeKey;
    if (hour >= 6 && hour < 10)
        timeKey = QStringLiteral("time_morning");
    else if (hour >= 10 && hour < 14)
        timeKey = QStringLiteral("time_noon");
    else if (hour >= 18 && hour < 22)
        timeKey = QStringLiteral("time_evening");
    else if (hour >= 22 || hour < 2)
        timeKey = QStringLiteral("time_night");
    else
        timeKey = QStringLiteral("time_afternoon");

    int displayHour = (hour == 0) ? 12 : (hour > 12 ? hour - 12 : hour);
    QString timeContext =
            templateValue(timeKey,
                          // Built-in defaults (Chinese, used only when personality.md lacks the key)
                          (timeKey == QStringLiteral("time_morning"))   ? QStringLiteral("早上%"
                                                                                       "1点，用户刚"
                                                                                       "起床，可以"
                                                                                       "说早安、元"
                                                                                       "气满满的话")
                          : (timeKey == QStringLiteral("time_noon"))    ? QStringLiteral("中午%"
                                                                                      "1点，该吃午"
                                                                                      "饭了")
                          : (timeKey == QStringLiteral("time_evening")) ? QStringLiteral("晚上%"
                                                                                         "1点，用户"
                                                                                         "可能在休"
                                                                                         "息")
                          : (timeKey == QStringLiteral("time_night"))   ? QStringLiteral("深夜%"
                                                                                       "1点，用户该"
                                                                                       "睡觉了，语"
                                                                                       "气要温柔哄"
                                                                                       "睡")
                                                                      : QStringLiteral("下午%1点"))
                    .arg(displayHour);

    prompt.replace(QStringLiteral("{{time_context}}"),
                   templateValue(QStringLiteral("time_prefix"), QStringLiteral("当前时间：")) +
                           timeContext + QLatin1Char('\n'));

    // User memory injection
    if (!memoryContent.isEmpty()) {
        QString memHeader = templateValue(QStringLiteral("memory_header"), QStringLiteral("## "
                                                                                          "关于用户"
                                                                                          "的记忆"
                                                                                          "\n"));
        prompt.replace(QStringLiteral("{{user_memories}}"),
                       memHeader + memoryContent + QLatin1Char('\n'));
    } else {
        prompt.remove(QStringLiteral("{{user_memories}}"));
    }

    return prompt;
}

QString PersonalityEngine::detectEmotion(const QString& text, double mood) const {
    // Get mood influence level from settings
    MoodInfluenceLevel influenceLevel = GirlfriendSettings::instance()->moodInfluence();

    // Strong emotion keywords → emotion mapping (these have clear emotional bias, unaffected by mood)
    static QMap<QString, QString> strongEmotionKeywords = {// Happy
                                                           {"哈哈", "happy"},
                                                           {"太好了", "happy"},
                                                           {"开心", "happy"},
                                                           {"好棒", "happy"},
                                                           {"嘻嘻", "happy"},
                                                           {"呵呵", "happy"},
                                                           {"耶", "happy"},
                                                           // Hate/Annoyed
                                                           {"哼", "hate"},
                                                           {"讨厌", "hate"},
                                                           // Shy
                                                           {"害羞", "shy"},
                                                           {"不好意思", "shy"},
                                                           {" blush", "shy"},
                                                           {"脸红", "shy"},
                                                           // Love
                                                           {"喜欢", "love"},
                                                           {"想你", "love"},
                                                           {"爱你", "love"},
                                                           {"亲亲", "love"},
                                                           {"抱抱", "love"},
                                                           {"宝贝", "love"},
                                                           // Worried/Caring
                                                           {"担心", "worried"},
                                                           {"别累着", "worried"},
                                                           {"休息", "worried"},
                                                           {"辛苦", "worried"},
                                                           {"关心", "worried"},
                                                           {"照顾", "worried"},
                                                           {"注意身体", "worried"},
                                                           // Sad
                                                           {"难过", "sad"},
                                                           {"不开心", "sad"},
                                                           {"伤心", "sad"},
                                                           {"呜呜", "sad"},
                                                           {"哭", "crying"},
                                                           {"流泪", "crying"},
                                                           {"眼泪", "crying"},
                                                           // Angry
                                                           {"生气", "angry"},
                                                           {"气死", "angry"},
                                                           {"火大", "angry"},
                                                           // Studying
                                                           {"学习", "studying"},
                                                           {"思考", "studying"},
                                                           {"工作", "studying"},
                                                           {"代码", "studying"},
                                                           {"编程", "studying"},
                                                           // Travelling
                                                           {"旅行", "travelling"},
                                                           {"出门", "travelling"},
                                                           {"旅游", "travelling"},
                                                           {"出去玩", "travelling"}};

    // Check for strong emotion keywords first (high priority)
    for (const QString& keyword : strongEmotionKeywords.keys()) {
        if (text.contains(keyword)) {
            return strongEmotionKeywords[keyword];
        }
    }

    // Neutral keywords — affected by mood
    static QMap<QString, QString> neutralKeywords = {{"还好", "neutral"}, {"没事", "neutral"},
                                                     {"好吧", "neutral"}, {"行吧", "neutral"},
                                                     {"嗯", "neutral"},   {"好的", "neutral"},
                                                     {"行", "neutral"},   {"可以", "neutral"},
                                                     {"哦", "neutral"},   {"啊", "neutral"}};

    // Check for neutral keywords and apply mood influence
    for (const QString& keyword : neutralKeywords.keys()) {
        if (text.contains(keyword)) {
            // Apply mood influence based on level
            if (influenceLevel == MoodInfluenceLevel::Low) {
                // Low influence: keyword matching only, no mood effect
                return "default";
            } else if (influenceLevel == MoodInfluenceLevel::High) {
                // High influence: mood dominates, determines emotion
                if (mood < 0.4) {
                    return "sad";
                } else if (mood > 0.7) {
                    return "happy";
                }
                return "default";
            } else {
                // Medium influence (default): mood affects neutral words
                if (mood < 0.4) {
                    // Low mood: neutral words → negative emotions
                    return "sad";
                } else if (mood > 0.7) {
                    // High mood: neutral words → positive emotions
                    return "happy";
                }
                return "default";
            }
        }
    }

    // Awaiting keywords (questions, expecting response)
    static QStringList awaitingKeywords = {"怎么样", "呢~", "呢？", "在吗", "在不在", "吗？", "呢"};

    for (const QString& keyword : awaitingKeywords) {
        if (text.contains(keyword)) {
            return "awaiting";
        }
    }

    // No keywords matched - randomly pick from mood-appropriate pool
    // This ensures the avatar doesn't always show the same "default" image
    QStringList pool;
    if (mood > 0.7) {
        pool = {"happy", "love", "shy", "awaiting", "default"};
    } else if (mood >= 0.3) {
        pool = {"default", "default", "studying", "awaiting", "default"};
    } else {
        pool = {"sad", "worried", "crying", "angry", "afraid", "default"};
    }

    int idx = QRandomGenerator::global()->bounded(pool.size());
    QString selected = pool[idx];

    // Avoid repeating the same non-default emotion twice in a row
    // (use a static last-emotion per instance, but since this is const, use mutable)
    static thread_local QString s_lastFallback;
    if (selected != "default" && selected == s_lastFallback && pool.size() > 1) {
        idx = (idx + 1) % pool.size();
        selected = pool[idx];
    }
    s_lastFallback = selected;

    return selected;
}

QString PersonalityEngine::emotionToDisplayName(const QString& emotion) const {
    static QMap<QString, QString> displayNames = {{"default", "默认"},    {"happy", "开心"},
                                                  {"shy", "害羞"},        {"love", "爱意"},
                                                  {"hate", "嫌弃"},       {"sad", "难过"},
                                                  {"angry", "生气"},      {"afraid", "关心"},
                                                  {"worried", "关心"},    {"awaiting", "期待"},
                                                  {"speaking", "说话中"}, {"studying", "思考"},
                                                  {"crying", "哭泣"},     {"travelling", "旅行"}};

    return displayNames.value(emotion, "默认");
}

void PersonalityEngine::setUserNickname(const QString& nickname) {
    m_userNickname = nickname;
}

void PersonalityEngine::updateMood(const QString& userInput) {
    // Negative keywords — decrease mood
    static QStringList negativeWords = {"滚", "烦", "别理我", "讨厌你", "不想说话", "闭嘴", "无语"};

    // Positive keywords — increase mood
    static QStringList positiveWords = {"爱你", "抱抱", "乖",   "喜欢你",
                                        "想你", "亲亲", "宝贝", "谢谢"};

    bool hasNegativeInput = false;
    bool hasPositiveInput = false;

    for (const QString& word : negativeWords) {
        if (userInput.contains(word)) {
            m_mood -= 0.3;
            hasNegativeInput = true;
            break;
        }
    }

    for (const QString& word : positiveWords) {
        if (userInput.contains(word)) {
            m_mood += 0.2;
            hasPositiveInput = true;
            break;
        }
    }

    // Apply natural decay only when no positive input was detected.
    // Positive input boosts mood — should not simultaneously decay.
    if (!hasPositiveInput) {
        m_mood -= m_moodDecay;
    }

    m_mood = qBound(0.0, m_mood, 1.0);

    emit moodChanged(m_mood);
}

QString PersonalityEngine::getMoodHint() const {
    if (m_mood < 0.3) {
        return templateValue(QStringLiteral("mood_low"), QStringLiteral("心情很差，说话带着哭腔，可"
                                                                        "能会说'呜...'"));
    } else if (m_mood < 0.5) {
        return templateValue(QStringLiteral("mood_mid"), QStringLiteral("有点不开心，说话简短，偶尔"
                                                                        "撒娇说'哼'"));
    } else if (m_mood > 0.8) {
        return templateValue(QStringLiteral("mood_high"), QStringLiteral("开开心心，语气特别甜，会"
                                                                         "说'嘻嘻~'"));
    } else {
        return QString();  // normal mood — no hint needed
    }
}

// Chinese emotion words → English emotion ID mapping
static QMap<QString, QString> chineseToEnglishEmotion() {
    static QMap<QString, QString> map = {
            {"开心", "happy"},      {"害羞", "shy"},        {"爱意", "love"},
            {"关心", "worried"},    {"担心", "worried"},    {"期待", "awaiting"},
            {"难过", "sad"},        {"嫌弃", "hate"},       {"生气", "angry"},
            {"害怕", "afraid"},     {"思考", "studying"},   {"哭泣", "crying"},
            {"旅行", "travelling"}, {"说话中", "speaking"}, {"默认", "default"}};
    return map;
}

PersonalityEngine::EmotionResult PersonalityEngine::parseEmotionFromResponse(
        const QString& text) const {
    EmotionResult result;
    result.emotion = "default";
    result.cleanText = text;

    // Match emotion tag: [情绪:xxx] or [emotion:xxx]
    QRegularExpression emotionRegex(R"(\[(?:情绪|emotion):([^\]]+)\])");
    QRegularExpressionMatch match = emotionRegex.match(text);

    if (match.hasMatch()) {
        QString emotionName = match.captured(1).trimmed();  // captured emotion word
        QString fullTag = match.captured(0);                // full tag

        // Map to English emotion ID, or use directly if already an English ID
        QMap<QString, QString> emotionMap = chineseToEnglishEmotion();
        if (emotionMap.contains(emotionName)) {
            // Chinese emotion word → English ID
            result.emotion = emotionMap[emotionName];
        } else if (emotionMap.values().contains(emotionName)) {
            // Already an English ID — use directly
            result.emotion = emotionName;
        }

        // Remove emotion tag and return cleaned text
        result.cleanText = text;
        result.cleanText.remove(fullTag);
        result.cleanText = result.cleanText.trimmed();
    } else {
        // No emotion tag found — fall back to keyword detection with current mood
        result.emotion = detectEmotion(text, m_mood);
    }

    return result;
}

PersonalityEngine::AffectionResult PersonalityEngine::parseAffectionFromResponse(
        const QString& text, const QString& userInput) const {
    AffectionResult result;
    result.change = 0.0;       // default: no change
    result.hasTag = false;     // marker: whether AI provided affection tag
    result.cleanText = text;

    // Match affection tag: [好感度:+0.1] or [好感度:-0.2] or [affection:+0.1]
    QRegularExpression affectionRegex(R"(\[(?:好感度|affection):([+-]?[\d.]+)\])");
    QRegularExpressionMatch match = affectionRegex.match(text);

    if (match.hasMatch()) {
        result.hasTag = true;
        double parsedChange = match.captured(1).toDouble();
        result.change = qBound(-0.3, parsedChange, 0.3);

        // Remove affection tag from text
        QString fullTag = match.captured(0);
        result.cleanText = text;
        result.cleanText.remove(fullTag);
        result.cleanText = result.cleanText.trimmed();
    } else {
        // Fallback: use keyword-based detection when AI didn't provide tag
        result.change = detectMoodChangeFromKeywords(userInput);
    }

    return result;
}

double PersonalityEngine::detectMoodChangeFromKeywords(const QString& userInput) const {
    static QStringList negativeWords = {"滚", "烦", "别理我", "讨厌你", "不想说话", "闭嘴", "无语"};
    static QStringList positiveWords = {"爱你", "抱抱", "乖", "喜欢你", "想你", "亲亲", "宝贝", "谢谢"};

    for (const QString& word : negativeWords) {
        if (userInput.contains(word)) {
            return -0.3;
        }
    }
    for (const QString& word : positiveWords) {
        if (userInput.contains(word)) {
            return 0.2;
        }
    }
    // No keyword matched: natural decay
    return -m_moodDecay;
}

void PersonalityEngine::updateMoodFromAI(double change) {
    m_mood += change;
    m_mood = qBound(0.0, m_mood, 1.0);
    emit moodChanged(m_mood);
}