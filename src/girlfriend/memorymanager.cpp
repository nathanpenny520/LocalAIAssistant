#include "memorymanager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>

MemoryManager::MemoryManager(QObject* parent) : QObject(parent), m_memoryContent() {
    loadMemory();
}

QStringList MemoryManager::findPossiblePaths() const {
    QStringList paths;

    QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_MACOS
    // macOS app bundle structure
    paths << QDir::cleanPath(appDir + "/../Resources/girlfriend/memory.md");
#elif defined(Q_OS_WIN)
    // Windows: resources in same directory as executable
    paths << QDir::cleanPath(appDir + "/girlfriend/memory.md");
#else
    // Linux
    paths << QDir::cleanPath(appDir + "/girlfriend/memory.md");
#endif

    // Generic fallback paths
    paths << "src/girlfriend/memory.md";
    paths << "sourcecode-ai-assistant/src/girlfriend/memory.md";

    // User data directory
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    paths << QDir::cleanPath(dataDir + "/girlfriend/memory.md");

    return paths;
}

QString MemoryManager::memoryFilePath() {
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

QString MemoryManager::loadMemory() {
    // Try user data directory first (writable)
    QString userPath = memoryFilePath();
    QFile userFile(userPath);
    if (userFile.exists() && userFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_memoryContent = QString::fromUtf8(userFile.readAll());
        userFile.close();
        qDebug() << "MemoryManager: Loaded memory from user data:" << userPath;
        return m_memoryContent;
    }

    // Then try application built-in paths (read-only)
    QStringList paths = findPossiblePaths();
    for (const QString& path : paths) {
        QFile file(path);
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_memoryContent = QString::fromUtf8(file.readAll());
            file.close();
            qDebug() << "MemoryManager: Loaded memory from:" << path;

            // Copy to user data directory so it becomes writable
            saveToFile();
            return m_memoryContent;
        }
    }

    // Default empty memory
    m_memoryContent = "# 用户记忆档案\n\n待记录";
    qDebug() << "MemoryManager: Using default empty memory";
    return m_memoryContent;
}

QString MemoryManager::getMemoryContent() {
    if (m_memoryContent.isEmpty()) {
        loadMemory();
    }
    return m_memoryContent;
}

void MemoryManager::saveToFile() {
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

void MemoryManager::updateMemory(const QString& newInfo) {
    // Append new info to the conversation summary section
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd");

    // Find conversation summary section and append
    QRegularExpression summaryRegex("(## 对话摘要\n.*?)(## 特别提醒|$)",
                                    QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = summaryRegex.match(m_memoryContent);

    if (match.hasMatch()) {
        QString summarySection = match.captured(1);
        QString nextSection = match.captured(2);

        // Replace "pending" placeholder if present
        if (summarySection.contains("待记录")) {
            summarySection = summarySection.replace("待记录", "- " + timestamp + ": " + newInfo);
        } else {
            // Append new record
            summarySection = summarySection.trimmed() + "\n- " + timestamp + ": " + newInfo + "\n";
        }

        m_memoryContent = m_memoryContent.replace(match.captured(0), summarySection + nextSection);
    } else {
        // No summary section found, append to end
        m_memoryContent += "\n\n- " + timestamp + ": " + newInfo;
    }

    saveToFile();
}

QList<MemoryManager::MemoryUpdate> MemoryManager::parseMemoryUpdates(const QString& response) {
    QList<MemoryUpdate> updates;

    // Parse format: [更新记忆:category|content] or [memory:category|content]
    QRegularExpression regex(R"(\[(?:更新记忆|memory):([^\|]+)\|([^\]]+)\])");
    QRegularExpressionMatchIterator it = regex.globalMatch(response);

    // Map English category names to Chinese section headers
    QMap<QString, QString> categoryMap = {{"basic_info", "基本信息"}, {"preferences", "喜好偏好"},
                                          {"events", "重要事件"},     {"reminders", "特别提醒"},
                                          {"基本信息", "基本信息"},   {"喜好偏好", "喜好偏好"},
                                          {"重要事件", "重要事件"},   {"特别提醒", "特别提醒"}};

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

void MemoryManager::applyMemoryUpdates(const QList<MemoryUpdate>& updates) {
    if (updates.isEmpty()) {
        return;
    }

    for (const MemoryUpdate& update : updates) {
        QString category = update.category;
        QString content = update.content;

        // Skip duplicates
        if (m_memoryContent.contains(content)) {
            qDebug() << "MemoryManager: 内容已存在，跳过重复记录 - " << content;
            continue;
        }

        // Update the corresponding section using lookahead to avoid consuming next section title
        QRegularExpression sectionRegex("(## " + category + "\\n)([\\s\\S]*?)(?=##|$)",
                                        QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatch match = sectionRegex.match(m_memoryContent);

        if (match.hasMatch()) {
            QString sectionHeader = match.captured(1);   // "## Category\n"
            QString sectionContent = match.captured(2);  // content under that category

            // Replace first "待记录" placeholder
            int pos = sectionContent.indexOf("待记录");
            if (pos != -1) {
                sectionContent =
                        sectionContent.replace(pos, QString("待记录").length(), "- " + content);
            } else {
                // Append new record
                sectionContent = sectionContent.trimmed() + "\n- " + content + "\n";
            }

            m_memoryContent =
                    m_memoryContent.replace(match.captured(0), sectionHeader + sectionContent);
        } else {
            qDebug() << "MemoryManager: Category not found:" << category;
        }
    }

    saveToFile();
    qDebug() << "MemoryManager: Applied" << updates.size() << "memory updates";
}