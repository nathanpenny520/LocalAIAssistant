#include "memoryenhancer.h"
#include "embedder.h"
#include "vectordb.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUuid>
#include <QDateTime>
#include <QDebug>

// ── MemoryEntry JSON serialization ──────────────────────────────

QJsonObject MemoryEntry::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("id")] = id;
    obj[QStringLiteral("type")] = type;
    obj[QStringLiteral("content")] = content;
    obj[QStringLiteral("sourceConversationId")] = sourceConversationId;
    obj[QStringLiteral("timestamp")] = timestamp.toString(Qt::ISODate);
    obj[QStringLiteral("confidence")] = static_cast<double>(confidence);

    QJsonArray entities;
    for (const auto &e : relatedEntities)
        entities.append(e);
    obj[QStringLiteral("relatedEntities")] = entities;

    return obj;
}

MemoryEntry MemoryEntry::fromJson(const QJsonObject &obj)
{
    MemoryEntry entry;
    entry.id = obj.value(QStringLiteral("id")).toString();
    entry.type = obj.value(QStringLiteral("type")).toString();
    entry.content = obj.value(QStringLiteral("content")).toString();
    entry.sourceConversationId = obj.value(QStringLiteral("sourceConversationId")).toString();
    entry.timestamp = QDateTime::fromString(
        obj.value(QStringLiteral("timestamp")).toString(), Qt::ISODate);
    entry.confidence = static_cast<float>(
        obj.value(QStringLiteral("confidence")).toDouble(1.0));

    const QJsonArray entities = obj.value(QStringLiteral("relatedEntities")).toArray();
    for (const auto &e : entities)
        entry.relatedEntities.append(e.toString());

    return entry;
}

// ── MemoryEnhancer ──────────────────────────────────────────────

MemoryEnhancer::MemoryEnhancer(Embedder *embedder, VectorDB *vectorDB, QObject *parent)
    : QObject(parent)
    , m_embedder(embedder)
    , m_vectorDB(vectorDB)
{
}

MemoryEnhancer::~MemoryEnhancer() = default;

QString MemoryEnhancer::storagePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/memories.json");
}

// ── Parse memory-update tags from AI response ───────────────────

static const QMap<QString, QString> &categoryMap()
{
    static const QMap<QString, QString> map = {
        {QStringLiteral("basic_info"), QStringLiteral("fact")},
        {QStringLiteral("preferences"), QStringLiteral("preference")},
        {QStringLiteral("events"), QStringLiteral("event")},
        {QStringLiteral("reminders"), QStringLiteral("reminder")},
        {QStringLiteral("基本信息"), QStringLiteral("fact")},
        {QStringLiteral("喜好偏好"), QStringLiteral("preference")},
        {QStringLiteral("重要事件"), QStringLiteral("event")},
        {QStringLiteral("特别提醒"), QStringLiteral("reminder")},
    };
    return map;
}

QVector<MemoryEntry> MemoryEnhancer::parseFromResponse(const QString &response,
                                                       const QString &conversationId)
{
    QVector<MemoryEntry> entries;

    // Match: [更新记忆:category|content] or [memory:category|content]
    static const QRegularExpression regex(
        QStringLiteral(R"(\[(?:更新记忆|memory):([^\|]+)\|([^\]]+)\])"));

    QRegularExpressionMatchIterator it = regex.globalMatch(response);
    const QDateTime now = QDateTime::currentDateTime();

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        const QString rawCategory = match.captured(1).trimmed();
        const QString content = match.captured(2).trimmed();

        if (content.isEmpty())
            continue;

        MemoryEntry entry;
        entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        entry.type = categoryMap().value(rawCategory, QStringLiteral("summary"));
        entry.content = content;
        entry.sourceConversationId = conversationId;
        entry.timestamp = now;
        entry.confidence = 0.9f;

        entries.append(entry);
    }

    return entries;
}

// ── Semantic search ─────────────────────────────────────────────

QVector<MemoryEntry> MemoryEnhancer::search(const QString &query, int topK) const
{
    if (!m_embedder || !m_vectorDB || m_entries.isEmpty())
        return {};

    QVector<float> queryVec = m_embedder->embed(query);
    QVector<SearchResult> results = m_vectorDB->search(queryVec, topK);

    QVector<MemoryEntry> matched;
    matched.reserve(results.size());

    // Map search results back to MemoryEntry by parsing the documentPath prefix
    for (const auto &sr : results) {
        // documentPath format: "memory:<entry_id>"
        const QString docPath = sr.chunk.documentPath;
        if (!docPath.startsWith(QStringLiteral("memory:")))
            continue;

        const QString entryId = docPath.mid(7);
        for (const auto &entry : m_entries) {
            if (entry.id == entryId) {
                MemoryEntry scored = entry;
                scored.confidence = sr.similarity;
                matched.append(scored);
                break;
            }
        }
    }

    return matched;
}

// ── Add entry ───────────────────────────────────────────────────

void MemoryEnhancer::addEntry(const MemoryEntry &entry)
{
    if (!m_embedder || !m_vectorDB)
        return;

    // Vectorize the content
    QVector<float> vec = m_embedder->embed(entry.content);
    if (vec.isEmpty())
        return;

    // Store in VectorDB as a single-chunk "document"
    TextChunk chunk;
    chunk.content = entry.content;
    chunk.documentPath = QStringLiteral("memory:") + entry.id;
    chunk.chunkIndex = 0;
    chunk.charOffset = 0;
    chunk.estimatedTokens = TextChunker::estimateTokens(entry.content);

    m_vectorDB->addVectors({vec}, {chunk});
    m_entries.append(entry);
}

// ── Remove by conversation ──────────────────────────────────────

void MemoryEnhancer::removeByConversation(const QString &conversationId)
{
    // Collect IDs to remove from VectorDB
    for (int i = m_entries.size() - 1; i >= 0; --i) {
        if (m_entries[i].sourceConversationId == conversationId) {
            const QString docPath = QStringLiteral("memory:") + m_entries[i].id;
            m_vectorDB->removeDocument(docPath);
            m_entries.removeAt(i);
        }
    }
}

// ── Build context for system prompt injection ───────────────────

QString MemoryEnhancer::buildContext(const QString &userQuery)
{
    QVector<MemoryEntry> relevant = search(userQuery, 5);
    if (relevant.isEmpty())
        return {};

    QString ctx;
    ctx += tr("## 相关记忆\n\n");
    for (int i = 0; i < relevant.size(); ++i) {
        const auto &entry = relevant[i];
        QString typeLabel;
        if (entry.type == QStringLiteral("fact")) typeLabel = tr("事实");
        else if (entry.type == QStringLiteral("preference")) typeLabel = tr("偏好");
        else if (entry.type == QStringLiteral("event")) typeLabel = tr("事件");
        else if (entry.type == QStringLiteral("reminder")) typeLabel = tr("提醒");
        else typeLabel = tr("摘要");

        ctx += QStringLiteral("- [%1] %2\n").arg(typeLabel, entry.content);
    }

    return ctx.trimmed();
}

// ── Persistence ─────────────────────────────────────────────────

void MemoryEnhancer::saveToFile()
{
    const QString path = storagePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QJsonArray arr;
    for (const auto &entry : m_entries)
        arr.append(entry.toJson());

    QJsonDocument doc(arr);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    } else {
        qWarning() << "MemoryEnhancer: failed to save to" << path;
    }
}

void MemoryEnhancer::loadFromFile()
{
    const QString path = storagePath();
    QFile file(path);
    if (!file.exists())
        return;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "MemoryEnhancer: failed to open" << path;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) {
        qWarning() << "MemoryEnhancer: invalid JSON format in" << path;
        return;
    }

    m_entries.clear();
    const QJsonArray arr = doc.array();
    for (const auto &val : arr) {
        if (val.isObject())
            m_entries.append(MemoryEntry::fromJson(val.toObject()));
    }
}
