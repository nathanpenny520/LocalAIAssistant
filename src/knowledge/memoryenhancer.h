#pragma once

#ifndef MEMORYENHANCER_H
#define MEMORYENHANCER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QJsonArray>

// Structured memory entry — compatible with existing MemoryManager's markdown
// format, but enriched for semantic search and cross-session injection.
struct MemoryEntry
{
    QString id;                     // UUID
    QString type;                   // "fact", "preference", "event", "summary", "reminder"
    QString content;                // the memory text
    QString sourceConversationId;   // which conversation produced it
    QDateTime timestamp;
    float confidence = 1.0f;        // 0.0–1.0
    QStringList relatedEntities;    // people, topics, projects

    QJsonObject toJson() const;
    static MemoryEntry fromJson(const QJsonObject &obj);
};

class Embedder;
class VectorDB;

class MemoryEnhancer : public QObject
{
    Q_OBJECT

public:
    explicit MemoryEnhancer(Embedder *embedder, VectorDB *vectorDB,
                            QObject *parent = nullptr);
    ~MemoryEnhancer() override;

    /// Parse memory-update tags from an AI response (compatible with the
    /// existing [更新记忆:category|content] format) into structured entries.
    QVector<MemoryEntry> parseFromResponse(const QString &response,
                                           const QString &conversationId);

    /// Search stored memories semantically, using the embedder + vector DB.
    QVector<MemoryEntry> search(const QString &query, int topK = 5) const;

    /// Add an entry manually and index it.
    void addEntry(const MemoryEntry &entry);

    /// Remove entries belonging to a conversation (e.g. when deleting a session).
    void removeByConversation(const QString &conversationId);

    /// Build a context snippet for injecting into the system prompt.
    /// Returns an empty string when no relevant memories are found.
    QString buildContext(const QString &userQuery);

    /// Persistence (JSON file next to the vector DB).
    void saveToFile();
    void loadFromFile();

    int entryCount() const { return m_entries.size(); }

private:
    Embedder *m_embedder;
    VectorDB *m_vectorDB;
    QVector<MemoryEntry> m_entries;

    QString storagePath() const;
};

#endif // MEMORYENHANCER_H
