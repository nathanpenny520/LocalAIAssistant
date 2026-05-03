#include "memorymanager.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDateTime>

MemoryManager::MemoryManager(QObject *parent)
    : QObject(parent)
    , m_memoryContent()
{
    loadMemory();
}

QStringList MemoryManager::findPossiblePaths() const
{
    QStringList paths;

    QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_MACOS
    // macOS app bundle 结构
    paths << QDir::cleanPath(appDir + "/../Resources/girlfriend/memory.md");
#elif defined(Q_OS_WIN)
    // Windows: 资源在可执行文件同级目录
    paths << QDir::cleanPath(appDir + "/girlfriend/memory.md");
#else
    // Linux
    paths << QDir::cleanPath(appDir + "/girlfriend/memory.md");
#endif

    // 通用备用路径
    paths << "src/girlfriend/memory.md";
    paths << "sourcecode-ai-assistant/src/girlfriend/memory.md";

    // 用户数据目录
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    paths << QDir::cleanPath(dataDir + "/girlfriend/memory.md");

    return paths;
}

QString MemoryManager::memoryFilePath()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QString girlfriendDir = dataDir + "/girlfriend";
    QDir gfDir(girlfriendDir);
    if (!gfDir.exists()) {
        gfDir.mkpath(".");
    }
    return girlfriendDir + "/memory.md";
}

QString MemoryManager::loadMemory()
{
    // 首先尝试用户数据目录（可写入）
    QString userPath = memoryFilePath();
    QFile userFile(userPath);
    if (userFile.exists() && userFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_memoryContent = QString::fromUtf8(userFile.readAll());
        userFile.close();
        qDebug() << "MemoryManager: Loaded memory from user data:" << userPath;
        return m_memoryContent;
    }

    // 然后尝试应用内置路径（只读）
    QStringList paths = findPossiblePaths();
    for (const QString &path : paths) {
        QFile file(path);
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_memoryContent = QString::fromUtf8(file.readAll());
            file.close();
            qDebug() << "MemoryManager: Loaded memory from:" << path;

            // 复制到用户数据目录以便后续写入
            saveToFile();
            return m_memoryContent;
        }
    }

    // 默认空记忆
    m_memoryContent = "# 用户记忆档案\n\n待记录";
    qDebug() << "MemoryManager: Using default empty memory";
    return m_memoryContent;
}

QString MemoryManager::getMemoryContent()
{
    if (m_memoryContent.isEmpty()) {
        loadMemory();
    }
    return m_memoryContent;
}

void MemoryManager::saveToFile()
{
    QString path = memoryFilePath();
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(m_memoryContent.toUtf8());
        file.close();
        qDebug() << "MemoryManager: Saved memory to:" << path;
    } else {
        qDebug() << "MemoryManager: Failed to save memory to:" << path;
    }
}

void MemoryManager::updateMemory(const QString &newInfo)
{
    // 简单追加新信息到对话摘要部分
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd");

    // 查找对话摘要部分并追加
    QRegularExpression summaryRegex("(## 对话摘要\n.*?)(## 特别提醒|$)",
                                     QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = summaryRegex.match(m_memoryContent);

    if (match.hasMatch()) {
        QString summarySection = match.captured(1);
        QString nextSection = match.captured(2);

        // 检查是否已有待记录，替换它
        if (summarySection.contains("待记录")) {
            summarySection = summarySection.replace("待记录", "- " + timestamp + ": " + newInfo);
        } else {
            // 追加新记录
            summarySection = summarySection.trimmed() + "\n- " + timestamp + ": " + newInfo + "\n";
        }

        m_memoryContent = m_memoryContent.replace(match.captured(0), summarySection + nextSection);
    } else {
        // 如果没有找到摘要部分，直接追加到末尾
        m_memoryContent += "\n\n- " + timestamp + ": " + newInfo;
    }

    saveToFile();
}

QList<MemoryManager::MemoryUpdate> MemoryManager::parseMemoryUpdates(const QString &response)
{
    QList<MemoryUpdate> updates;

    // 匹配格式: [更新记忆:分类|内容]
    QRegularExpression regex(R"(\[更新记忆:([^\|]+)\|([^\]]+)\])");
    QRegularExpressionMatchIterator it = regex.globalMatch(response);

    // 英文 category 映射到中文分类名
    QMap<QString, QString> categoryMap = {
        {"basic_info", "基本信息"},
        {"preferences", "喜好偏好"},
        {"events", "重要事件"},
        {"reminders", "特别提醒"},
        {"基本信息", "基本信息"},
        {"喜好偏好", "喜好偏好"},
        {"重要事件", "重要事件"},
        {"特别提醒", "特别提醒"}
    };

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        MemoryUpdate update;
        QString rawCategory = match.captured(1).trimmed();
        update.category = categoryMap.value(rawCategory, rawCategory);
        update.content = match.captured(2).trimmed();
        updates.append(update);
    }

    return updates;
}

void MemoryManager::applyMemoryUpdates(const QList<MemoryUpdate> &updates)
{
    if (updates.isEmpty()) {
        return;
    }

    for (const MemoryUpdate &update : updates) {
        QString category = update.category;
        QString content = update.content;

        // 检查是否已存在相同内容（去重）
        if (m_memoryContent.contains(content)) {
            qDebug() << "MemoryManager: 内容已存在，跳过重复记录 - " << content;
            continue;
        }

        // 根据分类更新对应部分，使用 lookahead 避免消耗下一个 section 标题
        QRegularExpression sectionRegex("(## " + category + "\\n)([\\s\\S]*?)(?=##|$)",
                                         QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatch match = sectionRegex.match(m_memoryContent);

        if (match.hasMatch()) {
            QString sectionHeader = match.captured(1);  // ## 分类名\n
            QString sectionContent = match.captured(2); // 该分类下的内容

            // 只替换第一个 "待记录"
            int pos = sectionContent.indexOf("待记录");
            if (pos != -1) {
                sectionContent = sectionContent.replace(pos, QString("待记录").length(), "- " + content);
            } else {
                // 追加新记录
                sectionContent = sectionContent.trimmed() + "\n- " + content + "\n";
            }

            m_memoryContent = m_memoryContent.replace(match.captured(0),
                                                       sectionHeader + sectionContent);
        } else {
            qDebug() << "MemoryManager: Category not found:" << category;
        }
    }

    saveToFile();
    qDebug() << "MemoryManager: Applied" << updates.size() << "memory updates";
}