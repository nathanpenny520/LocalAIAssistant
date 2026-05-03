#include "personalityengine.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QTime>

PersonalityEngine::PersonalityEngine(QObject *parent)
    : QObject(parent)
    , m_personalityPrompt()
    , m_userNickname("你")
{
    loadFromFile();
}

void PersonalityEngine::loadFromFile()
{
    QStringList possiblePaths = findPossiblePaths();

    for (const QString &path : possiblePaths) {
        QFile file(path);
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_personalityPrompt = QString::fromUtf8(file.readAll());
            file.close();
            qDebug() << "PersonalityEngine: Loaded prompt from:" << path;
            return;
        }
    }

    // Fallback: 默认 Prompt
    m_personalityPrompt = "你是小清，一个温柔体贴、有知性陪伴感的AI女友。";
    qDebug() << "PersonalityEngine: Using default prompt";
}

QStringList PersonalityEngine::findPossiblePaths() const
{
    QStringList paths;

    QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_MACOS
    // macOS app bundle 结构
    paths << QDir::cleanPath(appDir + "/../Resources/girlfriend/personality.md");
#elif defined(Q_OS_WIN)
    // Windows: 资源在可执行文件同级目录
    paths << QDir::cleanPath(appDir + "/girlfriend/personality.md");
#else
    // Linux
    paths << QDir::cleanPath(appDir + "/girlfriend/personality.md");
#endif

    // 通用备用路径
    paths << "src/girlfriend/personality.md";
    paths << "sourcecode-ai-assistant/src/girlfriend/personality.md";

    return paths;
}

QString PersonalityEngine::loadPersonalityPrompt()
{
    if (m_personalityPrompt.isEmpty()) {
        loadFromFile();
    }
    return m_personalityPrompt;
}

QString PersonalityEngine::buildSystemPrompt()
{
    QString prompt = m_personalityPrompt;

    // 替换用户昵称占位符
    prompt.replace("{{user_nickname}}", m_userNickname);

    // 添加心情提示
    QString moodHint = getMoodHint();
    if (!moodHint.isEmpty()) {
        prompt += "\n\n当前心情：" + moodHint;
    }

    // 添加时间感知
    QTime now = QTime::currentTime();
    int hour = now.hour();
    QString timeContext;

    if (hour >= 6 && hour < 10) {
        timeContext = QString("早上%1点，用户刚起床，可以说早安、元气满满的话").arg(hour);
    } else if (hour >= 10 && hour < 14) {
        timeContext = QString("中午%1点，该吃午饭了").arg(hour);
    } else if (hour >= 18 && hour < 22) {
        timeContext = QString("晚上%1点，用户可能在休息").arg(hour);
    } else if (hour >= 22 || hour < 2) {
        timeContext = QString("深夜%1点，用户该睡觉了，语气要温柔哄睡").arg(hour == 0 ? 12 : (hour > 12 ? hour - 12 : hour));
    } else {
        timeContext = QString("下午%1点").arg(hour);
    }

    prompt += "\n\n当前时间：" + timeContext;

    return prompt;
}

QString PersonalityEngine::detectEmotion(const QString &text) const
{
    // 关键词 → 情绪映射
    static QMap<QString, QString> emotionKeywords = {
        {"哈哈", "happy"},
        {"太好了", "happy"},
        {"开心", "happy"},
        {"好棒", "happy"},
        {"嘻嘻", "happy"},
        {"哼", "hate"},
        {"讨厌", "hate"},
        {"害羞", "shy"},
        {"不好意思", "shy"},
        {" blush", "shy"},
        {"喜欢", "love"},
        {"想你", "love"},
        {"爱你", "love"},
        {"担心", "worried"},
        {"别累着", "worried"},
        {"休息", "worried"},
        {"辛苦", "worried"},
        {"关心", "worried"},
        {"照顾", "worried"},
        {"怎么样", "awaiting"},
        {"呢~", "awaiting"},
        {"呢？", "awaiting"},
        {"难过", "sad"},
        {"不开心", "sad"},
        {"伤心", "sad"},
        {"生气", "angry"},
        {"气死", "angry"},
        {"学习", "studying"},
        {"思考", "studying"},
        {"工作", "studying"},
        {"代码", "studying"},
        {"编程", "studying"}
    };

    for (const QString &keyword : emotionKeywords.keys()) {
        if (text.contains(keyword)) {
            return emotionKeywords[keyword];
        }
    }

    return "default";
}

QString PersonalityEngine::emotionToDisplayName(const QString &emotion) const
{
    static QMap<QString, QString> displayNames = {
        {"default", "默认"},
        {"happy", "开心"},
        {"shy", "害羞"},
        {"love", "爱意"},
        {"hate", "嫌弃"},
        {"sad", "难过"},
        {"angry", "生气"},
        {"afraid", "关心"},
        {"worried", "关心"},
        {"awaiting", "期待"},
        {"speaking", "说话中"},
        {"studying", "思考"}
    };

    return displayNames.value(emotion, "默认");
}

void PersonalityEngine::setUserNickname(const QString &nickname)
{
    m_userNickname = nickname;
}

void PersonalityEngine::updateMood(const QString &userInput)
{
    // 负面关键词降低心情
    static QStringList negativeWords = {
        "滚", "烦", "别理我", "讨厌你", "不想说话", "闭嘴", "无语"
    };

    // 正面关键词提升心情
    static QStringList positiveWords = {
        "爱你", "抱抱", "乖", "喜欢你", "想你", "亲亲", "宝贝", "谢谢"
    };

    for (const QString &word : negativeWords) {
        if (userInput.contains(word)) {
            m_mood -= 0.3;
            break;
        }
    }

    for (const QString &word : positiveWords) {
        if (userInput.contains(word)) {
            m_mood += 0.2;
            break;
        }
    }

    // 应用衰减
    m_mood -= m_moodDecay;

    // 限制范围 [0, 1]
    m_mood = qBound(0.0, m_mood, 1.0);

    emit moodChanged(m_mood);
}

QString PersonalityEngine::getMoodHint() const
{
    if (m_mood < 0.3) {
        return "心情很差，说话带着哭腔，可能会说'呜...'";
    } else if (m_mood < 0.5) {
        return "有点不开心，说话简短，偶尔撒娇说'哼'";
    } else if (m_mood > 0.8) {
        return "开开心心，语气特别甜，会说'嘻嘻~'";
    } else {
        return "";  // 正常状态不添加提示
    }
}

// 中文情绪词 → 英文情绪ID映射
static QMap<QString, QString> chineseToEnglishEmotion() {
    static QMap<QString, QString> map = {
        {"开心", "happy"},
        {"害羞", "shy"},
        {"爱意", "love"},
        {"关心", "worried"},
        {"期待", "awaiting"},
        {"难过", "sad"},
        {"嫌弃", "hate"},
        {"思考", "studying"},
        {"默认", "default"}
    };
    return map;
}

PersonalityEngine::EmotionResult PersonalityEngine::parseEmotionFromResponse(const QString &text) const
{
    EmotionResult result;
    result.emotion = "default";
    result.cleanText = text;

    // 匹配情绪标记: [情绪:xxx]
    QRegularExpression emotionRegex(R"(\[情绪:([^\]]+)\])");
    QRegularExpressionMatch match = emotionRegex.match(text);

    if (match.hasMatch()) {
        QString chineseEmotion = match.captured(1);  // 提取情绪词
        QString fullTag = match.captured(0);         // 完整标记

        // 转换为英文情绪ID
        QMap<QString, QString> emotionMap = chineseToEnglishEmotion();
        if (emotionMap.contains(chineseEmotion)) {
            result.emotion = emotionMap[chineseEmotion];
        }

        // 移除情绪标记，返回清理后的文本
        result.cleanText = text;
        result.cleanText.remove(fullTag);
        result.cleanText = result.cleanText.trimmed();
    } else {
        // 如果没有情绪标记，fallback到关键词检测
        result.emotion = detectEmotion(text);
    }

    return result;
}