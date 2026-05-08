#include "knowledgebase.h"
#include "../prompts/promptmanager.h"
#include "embedder.h"
#include "docimporter.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QtConcurrent>

KnowledgeBase *KnowledgeBase::s_instance = nullptr;

KnowledgeBase *KnowledgeBase::instance()
{
    if (!s_instance)
        s_instance = new KnowledgeBase();
    return s_instance;
}

KnowledgeBase::KnowledgeBase(QObject *parent)
    : QObject(parent)
    , m_embedder(new Embedder())
    , m_vectorDB(new VectorDB())
{
}

KnowledgeBase::~KnowledgeBase()
{
    delete m_importer;
    delete m_vectorDB;
    delete m_embedder;
}

bool KnowledgeBase::init()
{
    if (m_ready)
        return true;

    QString dir = storageDir();

    // 初始化 VectorDB
    if (!m_vectorDB->init(m_embedder->dimension(), dir))
        return false;

    // 加载已有数据
    m_vectorDB->load();

    // 初始化 Embedder（尝试加载 ONNX 模型）
    QString modelPath = Embedder::findModelPath();
    if (!modelPath.isEmpty())
        m_embedder->loadModel(modelPath);
    else
        m_embedder->loadModel({}); // 占位模式

    // 初始化 DocImporter
    m_importer = new DocImporter(m_embedder, m_vectorDB, this);

    m_ready = true;
    return true;
}

bool KnowledgeBase::importDocument(const QString &filePath)
{
    if (!m_ready || !m_importer)
        return false;

    ImportResult result = m_importer->importDocument(filePath);
    if (result.success) {
        emit documentImported(filePath, result.chunkCount);
        return true;
    } else {
        emit importFailed(filePath, result.errorMessage);
        return false;
    }
}

QVector<ImportResult> KnowledgeBase::importDocuments(const QStringList &filePaths)
{
    if (!m_ready || !m_importer)
        return {};

    QVector<ImportResult> results = m_importer->importDocuments(filePaths);
    for (const auto &r : results) {
        if (r.success)
            emit documentImported(r.documentPath, r.chunkCount);
        else
            emit importFailed(r.documentPath, r.errorMessage);
    }
    return results;
}

void KnowledgeBase::importDocumentAsync(const QString &filePath)
{
    QThreadPool::globalInstance()->start([this, filePath]() {
        if (!m_ready || !m_importer)
            return;
        ImportResult result = m_importer->importDocument(filePath);
        QMetaObject::invokeMethod(this, [this, result]() {
            if (result.success)
                emit documentImported(result.documentPath, result.chunkCount);
            else
                emit importFailed(result.documentPath, result.errorMessage);
        }, Qt::QueuedConnection);
    });
}

void KnowledgeBase::importDocumentsAsync(const QStringList &filePaths)
{
    QThreadPool::globalInstance()->start([this, filePaths]() {
        if (!m_ready || !m_importer)
            return;
        for (const auto &path : filePaths) {
            ImportResult result = m_importer->importDocument(path);
            QMetaObject::invokeMethod(this, [this, result]() {
                if (result.success)
                    emit documentImported(result.documentPath, result.chunkCount);
                else
                    emit importFailed(result.documentPath, result.errorMessage);
            }, Qt::QueuedConnection);
        }
    });
}

bool KnowledgeBase::removeDocument(const QString &filePath)
{
    if (!m_ready || !m_vectorDB)
        return false;

    int removed = m_vectorDB->removeDocument(filePath);
    if (removed > 0) {
        emit documentRemoved(filePath);
        return true;
    }
    return false;
}

QStringList KnowledgeBase::allDocuments() const
{
    if (!m_vectorDB)
        return {};
    return m_vectorDB->allDocuments();
}

QVector<SearchResult> KnowledgeBase::search(const QString &query, int topK) const
{
    if (!m_ready || !m_embedder || !m_vectorDB)
        return {};

    QVector<float> queryVec = m_embedder->embed(query);
    return m_vectorDB->search(queryVec, topK);
}

QString KnowledgeBase::generateContext(const QString &query, int topK) const
{
    QVector<SearchResult> results = search(query, topK);

    if (results.isEmpty()) {
        // No semantic match — tell AI what documents exist so it can guide the user
        QStringList docs = allDocuments();
        if (docs.isEmpty())
            return {};

        QString context;
        context += tr("用户的知识库中有以下文档，但当前查询未匹配到具体内容：\n");
        for (const auto &doc : docs) {
            QFileInfo fi(doc);
            context += QStringLiteral("- %1\n").arg(fi.fileName());
        }
        context += tr("\n如果用户的问题涉及这些文档，请告知用户相关文档名称，并建议具体提问方向。");
        return context;
    }

    // 构建检索到的文本块
    QString chunks;
    for (int i = 0; i < results.size(); ++i) {
        const auto &sr = results[i];
        chunks += tr("--- 来源: %1 (相关度: %2%) ---\n")
                      .arg(sr.chunk.documentPath)
                      .arg(static_cast<int>(sr.similarity * 100));
        chunks += sr.chunk.content;
        chunks += QStringLiteral("\n\n");
    }

    // 使用提示词模板包装
    QString template_ = PromptManager::instance()->knowledgePrompt();
    QString context = template_;
    context.replace(QStringLiteral("{{chunks}}"), chunks.trimmed());

    return context;
}

bool KnowledgeBase::isKnowledgeQuery(const QString &message)
{
    // Only trigger if knowledge base actually has content
    if (instance()->totalDocuments() == 0)
        return false;

    static const QStringList queryKeywords = {
        // Chinese — explicit knowledge base references
        QStringLiteral("知识库"), QStringLiteral("导入的文档"), QStringLiteral("我的文档"),
        QStringLiteral("文档里"), QStringLiteral("我导入"), QStringLiteral("之前上传"),
        // Chinese — search / look up in documents
        QStringLiteral("查一下文档"), QStringLiteral("搜一下"), QStringLiteral("搜索文档"),
        QStringLiteral("找一下文档"), QStringLiteral("有没有关于"), QStringLiteral("资料里"),
        // Chinese — document-based reasoning
        QStringLiteral("根据文档"), QStringLiteral("根据资料"), QStringLiteral("参考文档"),
        // English
        QStringLiteral("knowledge base"), QStringLiteral("my documents"),
        QStringLiteral("search my files"), QStringLiteral("look up in"),
        QStringLiteral("based on the document"), QStringLiteral("imported doc"),
    };

    QString lower = message.toLower();
    for (const auto &kw : queryKeywords) {
        if (lower.contains(kw))
            return true;
    }

    return false;
}

bool KnowledgeBase::isReady() const
{
    return m_ready;
}

int KnowledgeBase::totalChunks() const
{
    return m_vectorDB ? m_vectorDB->totalChunks() : 0;
}

int KnowledgeBase::totalDocuments() const
{
    return m_vectorDB ? m_vectorDB->totalDocuments() : 0;
}

Embedder *KnowledgeBase::embedder() const
{
    return m_embedder;
}

VectorDB *KnowledgeBase::vectorDB() const
{
    return m_vectorDB;
}

QString KnowledgeBase::storageDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/knowledge");
}
