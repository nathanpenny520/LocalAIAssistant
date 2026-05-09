#pragma once

#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <QMap>
#include <QObject>
#include <QString>

class MemoryManager : public QObject {
    Q_OBJECT

public:
    explicit MemoryManager(QObject* parent = nullptr);

    /**
     * @brief Load memory file from disk.
     */
    QString loadMemory();

    /**
     * @brief Get memory content (for injecting into system prompt).
     */
    QString getMemoryContent();

    /**
     * @brief Update memory file by appending new info.
     */
    void updateMemory(const QString& newInfo);

    /**
     * @brief Parse AI-received memory update requests.
     *
     * Format: [更新记忆:category|content] or [memory:category|content]
     */
    struct MemoryUpdate {
        QString category;  // category name (basic_info, preferences, events, reminders, etc.)
        QString content;   // content to record
    };
    QList<MemoryUpdate> parseMemoryUpdates(const QString& response);

    /**
     * @brief Apply parsed memory updates to the memory file.
     */
    void applyMemoryUpdates(const QList<MemoryUpdate>& updates);

    /**
     * @brief Get the memory file path.
     */
    static QString memoryFilePath();

private:
    QString m_memoryContent;

    void saveToFile();
    QStringList findPossiblePaths() const;
};

#endif  // MEMORYMANAGER_H