#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>

class MemoryManager : public QObject
{
    Q_OBJECT

public:
    explicit MemoryManager(QObject *parent = nullptr);

    // 加载记忆文件
    QString loadMemory();

    // 获取记忆内容（用于注入 system prompt）
    QString getMemoryContent();

    // 更新记忆文件
    void updateMemory(const QString &newInfo);

    // 解析 AI 返回的记忆更新请求
    // 格式: [更新记忆:分类|内容]
    struct MemoryUpdate {
        QString category;   // 基本信息、喜好偏好、重要事件、对话摘要、特别提醒
        QString content;    // 要记录的内容
    };
    QList<MemoryUpdate> parseMemoryUpdates(const QString &response);

    // 应用记忆更新
    void applyMemoryUpdates(const QList<MemoryUpdate> &updates);

    // 记忆文件路径
    static QString memoryFilePath();

private:
    QString m_memoryContent;

    void saveToFile();
    QStringList findPossiblePaths() const;
};

#endif // MEMORYMANAGER_H