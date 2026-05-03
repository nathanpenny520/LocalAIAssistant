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

    QStringList searchPaths;

#ifdef TRANSLATIONS_DIR
    searchPaths << QString(TRANSLATIONS_DIR);
#endif

#ifdef Q_OS_MACOS
    // macOS app bundle structure
    searchPaths << QApplication::applicationDirPath() + "/../Resources/translations";
#elif defined(Q_OS_WIN)
    // Windows: translations in executable directory
    searchPaths << QApplication::applicationDirPath() + "/translations";
#else
    // Linux
    searchPaths << QApplication::applicationDirPath() + "/translations";
#endif

    // 通用备用路径
    searchPaths << QDir::currentPath() + "/translations";
    searchPaths << QDir::currentPath() + "/../translations";

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

    // 移除旧的翻译器
    if (m_translator) {
        qApp->removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }
    // 不加载 Qt 翻译器，让系统对话框（如 QFileDialog）使用系统语言
    m_currentLocale.clear();

    // 加载应用翻译器
    m_translator = new QTranslator(this);
    QString qmFile = QString("localai_%1.qm").arg(locale);
    QString searchPath = findQmFile(locale);

    bool loaded = false;

    if (!searchPath.isEmpty()) {
        loaded = m_translator->load(qmFile, searchPath);
    }

    if (!loaded) {
        QStringList searchPaths;

#ifdef TRANSLATIONS_DIR
        searchPaths << QString(TRANSLATIONS_DIR);
#endif

#ifdef Q_OS_MACOS
        searchPaths << QApplication::applicationDirPath() + "/../Resources/translations";
#elif defined(Q_OS_WIN)
        searchPaths << QApplication::applicationDirPath() + "/translations";
#else
        searchPaths << QApplication::applicationDirPath() + "/translations";
#endif

        searchPaths << QDir::currentPath() + "/translations";
        searchPaths << QDir::currentPath() + "/../translations";

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
