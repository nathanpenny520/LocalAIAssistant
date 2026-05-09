/**
 * @file promptmanager.h
 * @brief System, task, knowledge, and girlfriend prompt loading with locale-aware fallback.
 */
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

    /// Load a prompt file by name (auto-applies OS template variables)
    QString loadPrompt(const QString& name) const;

    /// ── Convenience Accessors ──────────────────────────────
    QString systemPrompt() const;
    QString taskPrompt() const;
    QString girlfriendPrompt() const;
    QString girlfriendConfigValue(const QString& key, const QString& fallback = {}) const;
    QString knowledgePrompt() const;

    /// Parse the CONFIG block embedded in girlfriend.md
    QMap<QString, QString> girlfriendConfig() const;

    /// Prompt directory path (for user customization)
    static QString promptsDir();

    /// ── Language Switching ──────────────────────────────────
    QString currentLanguage() const;
    void setLanguage(const QString& locale);

    /// ── OS Detection ──────────────────────────────────────
    static QString detectOS();                         // "macos" / "linux" / "windows"
    static QMap<QString, QString> osTemplateValues();  // template variable map for the current OS

signals:
    void promptsReloaded();

private:
    explicit PromptManager(QObject* parent = nullptr);
    static PromptManager* s_instance;

    QString findPromptPath(const QString& name) const;
    QString readFileContent(const QString& path) const;
    QString applyTemplateVariables(const QString& content) const;

    mutable QMap<QString, QString> m_cache;
    mutable QMap<QString, QString> m_girlfriendConfigCache;
    mutable bool m_configParsed = false;
    mutable QString m_cachedLanguage;  // track current language to auto-flush cache on switch
};

#endif  // PROMPTMANAGER_H
