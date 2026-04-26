#ifndef TRANSLATIONMANAGER_H
#define TRANSLATIONMANAGER_H

#include <QObject>
#include <QTranslator>
#include <QString>

class TranslationManager : public QObject
{
    Q_OBJECT

public:
    static TranslationManager* instance();

    bool loadTranslation(const QString &locale);
    QString currentLocale() const { return m_currentLocale; }
    bool isLoaded() const { return m_translator != nullptr; }

    // 临时禁用/启用翻译器（用于系统对话框）
    void temporarilyDisable();
    void reEnable();

signals:
    void languageChanged();

private:
    explicit TranslationManager(QObject *parent = nullptr);
    ~TranslationManager() = default;

    static TranslationManager *m_instance;
    QTranslator *m_translator = nullptr;
    QString m_currentLocale;
    bool m_wasEnabled = false;

    QString findQmFile(const QString &locale);
};

#endif
