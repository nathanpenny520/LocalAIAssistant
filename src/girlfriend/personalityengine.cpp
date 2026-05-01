#include "personalityengine.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

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
    paths << QDir::cleanPath(appDir + "/../Resources/girlfriend/personality.md");
    paths << QDir::cleanPath(appDir + "/girlfriend/personality.md");
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
        {"担心", "afraid"},
        {"别累着", "afraid"},
        {"休息", "afraid"},
        {"辛苦", "afraid"},
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