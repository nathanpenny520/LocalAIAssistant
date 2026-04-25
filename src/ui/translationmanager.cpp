#include "translationmanager.h"
#include <QApplication>
#include <QDir>
#include <QDebug>

TranslationManager* TranslationManager::m_instance = nullptr;

TranslationManager::TranslationManager(QObject *parent)
    : QObject(parent)
{
}

TranslationManager* TranslationManager::instance()
{
    if (!m_instance) {
        m_instance = new TranslationManager();
    }
    return m_instance;
}

QString TranslationManager::findQmFile(const QString &locale)
{
    QString qmFile = QString("localai_%1.qm").arg(locale);

    QStringList searchPaths = {
#ifdef TRANSLATIONS_DIR
        QString(TRANSLATIONS_DIR),
#endif
        QApplication::applicationDirPath() + "/../Resources/translations",
        QApplication::applicationDirPath() + "/translations",
        QDir::currentPath() + "/translations",
        QDir::currentPath() + "/../translations"
    };

    for (const QString &path : searchPaths) {
        QString fullPath = path + "/" + qmFile;
        if (QFile::exists(fullPath)) {
            return path;
        }
    }

    return QString();
}

bool TranslationManager::loadTranslation(const QString &locale)
{
    if (locale == m_currentLocale && m_translator && !locale.isEmpty()) {
        return true;
    }

    if (m_translator) {
        qApp->removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }
    m_currentLocale.clear();

    m_translator = new QTranslator(this);
    QString qmFile = QString("localai_%1.qm").arg(locale);
    QString searchPath = findQmFile(locale);

    bool loaded = false;

    if (!searchPath.isEmpty()) {
        loaded = m_translator->load(qmFile, searchPath);
    }

    if (!loaded) {
        QStringList searchPaths = {
#ifdef TRANSLATIONS_DIR
            QString(TRANSLATIONS_DIR),
#endif
            QApplication::applicationDirPath() + "/../Resources/translations",
            QApplication::applicationDirPath() + "/translations",
            QDir::currentPath() + "/translations",
            QDir::currentPath() + "/../translations"
        };

        for (const QString &path : searchPaths) {
            loaded = m_translator->load(qmFile, path);
            if (loaded) {
                break;
            }
        }
    }

    if (loaded) {
        qApp->installTranslator(m_translator);
        m_currentLocale = locale;
        emit languageChanged();
        return true;
    } else {
        delete m_translator;
        m_translator = nullptr;
        return false;
    }
}
