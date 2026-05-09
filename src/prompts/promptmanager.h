#pragma once

#ifndef PROMPTMANAGER_H
#define PROMPTMANAGER_H

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>

class PromptManager : public QObject {
    Q_OBJECT

public:
    static PromptManager* instance();

    /// 加载指定名称的 prompt 文件（自动应用 OS 模板变量）
    QString loadPrompt(const QString& name) const;

    /// ── 便捷访问器 ──────────────────────────────────────
    QString systemPrompt() const;
    QString taskPrompt() const;
    QString girlfriendPrompt() const;
    QString girlfriendConfigValue(const QString& key, const QString& fallback = {}) const;
    QString knowledgePrompt() const;

    /// 解析 girlfriend.md 中的 CONFIG 块
    QMap<QString, QString> girlfriendConfig() const;

    /// 提示词目录路径（用于用户自定义）
    static QString promptsDir();

    /// ── 语言切换 ────────────────────────────────────────
    QString currentLanguage() const;
    void setLanguage(const QString& locale);

    /// ── 操作系统检测 ────────────────────────────────────
    static QString detectOS();                         // "macos" / "linux" / "windows"
    static QMap<QString, QString> osTemplateValues();  // 当前 OS 的模板变量表

signals:
    void promptsReloaded();

private:
    explicit PromptManager(QObject* parent = nullptr);
    static PromptManager* s_instance;

    QString findPromptPath(const QString& name) const;
    QString readFileContent(const QString& path) const;
    QString applyTemplateVariables(const QString& content) const;

    // 缓存
    mutable QMap<QString, QString> m_cache;
    mutable QMap<QString, QString> m_girlfriendConfigCache;
    mutable bool m_configParsed = false;
    mutable QString m_cachedLanguage;  // 用于检测语言切换时清缓存
};

#endif  // PROMPTMANAGER_H
