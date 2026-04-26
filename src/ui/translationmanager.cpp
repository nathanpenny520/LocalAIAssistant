#include "translationmanager.h"
#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QLibraryInfo>

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

    // 移除旧的翻译器
    if (m_translator) {
        qApp->removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }
    if (m_qtTranslator) {
        qApp->removeTranslator(m_qtTranslator);
        delete m_qtTranslator;
        m_qtTranslator = nullptr;
    }
    m_currentLocale.clear();

    // 加载 Qt 内置翻译器（用于标准对话框如 QFileDialog）
    if (locale != "en") {
        m_qtTranslator = new QTranslator(this);
        QString qtQmFile = QString("qt_%1.qm").arg(locale);
        // Qt 翻译文件通常在 Qt 安装目录的 translations 子目录下
        if (m_qtTranslator->load(qtQmFile, QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
            qApp->installTranslator(m_qtTranslator);
        } else {
            delete m_qtTranslator;
            m_qtTranslator = nullptr;
        }
    }

    // 加载应用翻译器
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

void TranslationManager::temporarilyDisable()
{
    // 同时移除应用翻译器和 Qt 翻译器，让系统对话框使用系统语言
    if (m_translator) {
        qApp->removeTranslator(m_translator);
        m_wasEnabled = true;
    }
    if (m_qtTranslator) {
        qApp->removeTranslator(m_qtTranslator);
    }
}

void TranslationManager::reEnable()
{
    // 恢复两个翻译器
    if (m_translator && m_wasEnabled) {
        qApp->installTranslator(m_translator);
        m_wasEnabled = false;
    }
    if (m_qtTranslator) {
        qApp->installTranslator(m_qtTranslator);
    }
}
