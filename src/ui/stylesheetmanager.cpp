#include "stylesheetmanager.h"
#include <QSettings>
#include <QWidget>
#include <QApplication>

StyleSheetManager::StyleSheetManager(QObject *parent)
    : QObject(parent)
    , m_currentTheme(SystemTheme)
{
    loadThemeFromSettings();
}

StyleSheetManager* StyleSheetManager::instance()
{
    static StyleSheetManager manager;
    return &manager;
}

StyleSheetManager::Theme StyleSheetManager::currentTheme() const
{
    return m_currentTheme;
}

QString StyleSheetManager::currentStyleSheet() const
{
    return m_styleSheet;
}

void StyleSheetManager::setTheme(Theme theme)
{
    m_currentTheme = theme;

    switch (theme) {
    case LightTheme:
        m_styleSheet = lightStyleSheet();
        break;
    case DarkTheme:
        m_styleSheet = darkStyleSheet();
        break;
    case SystemTheme:
    default:
        QString systemTheme = detectSystemTheme();
        if (systemTheme == "dark") {
            m_styleSheet = darkStyleSheet();
        } else {
            m_styleSheet = lightStyleSheet();
        }
        break;
    }

    saveThemeToSettings();
    emit themeChanged(m_currentTheme);
}

void StyleSheetManager::applyTheme(QWidget *rootWidget)
{
    setTheme(m_currentTheme);
    rootWidget->setStyleSheet(m_styleSheet);
}

void StyleSheetManager::loadThemeFromSettings()
{
    QSettings settings("LocalAIAssistant", "Settings");
    int themeValue = settings.value("theme", static_cast<int>(SystemTheme)).toInt();
    m_currentTheme = static_cast<Theme>(themeValue);
    setTheme(m_currentTheme);
}

void StyleSheetManager::saveThemeToSettings()
{
    QSettings settings("LocalAIAssistant", "Settings");
    settings.setValue("theme", static_cast<int>(m_currentTheme));
}

QString StyleSheetManager::detectSystemTheme() const
{
    QPalette palette = QApplication::palette();
    QColor windowColor = palette.color(QPalette::Window);
    int brightness = (windowColor.red() * 299 + windowColor.green() * 587 + windowColor.blue() * 114) / 1000;
    return brightness < 128 ? "dark" : "light";
}

QString StyleSheetManager::lightStyleSheet()
{
    return R"(
        QMainWindow {
            background-color: #ffffff;
        }
        QWidget {
            background-color: #ffffff;
            color: #333333;
            font-family: -apple-system, "SF Pro Text", "Helvetica Neue", sans-serif;
            font-size: 14px;
        }
        QListWidget {
            background-color: #f5f5f5;
            border: 1px solid #e0e0e0;
            border-radius: 8px;
            padding: 4px;
            outline: none;
        }
        QListWidget::item {
            padding: 10px 12px;
            border-radius: 6px;
            margin: 2px 0;
        }
        QListWidget::item:selected {
            background-color: #007aff;
            color: #ffffff;
        }
        QListWidget::item:hover:!selected {
            background-color: #e8e8e8;
        }
        QTextBrowser {
            background-color: #ffffff;
            border: 1px solid #e0e0e0;
            border-radius: 8px;
            padding: 16px;
            font-size: 15px;
        }
        QLineEdit {
            padding: 10px 16px;
            border: 2px solid #e0e0e0;
            border-radius: 20px;
            background-color: #f9f9f9;
            font-size: 14px;
        }
        QLineEdit:focus {
            border-color: #007aff;
            background-color: #ffffff;
        }
        QLineEdit:disabled {
            background-color: #f0f0f0;
            color: #999999;
        }
        QPushButton {
            padding: 8px 20px;
            border: none;
            border-radius: 16px;
            background-color: #007aff;
            color: #ffffff;
            font-size: 14px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #0062cc;
        }
        QPushButton:pressed {
            background-color: #0050a0;
        }
        QPushButton:disabled {
            background-color: #b0d0ff;
            color: #e0e0e0;
        }
        QPushButton#newChatButton {
            background-color: #f0f0f0;
            color: #333333;
            border: 1px solid #e0e0e0;
        }
        QPushButton#newChatButton:hover {
            background-color: #e0e0e0;
        }
        QMenuBar {
            background-color: #f5f5f5;
            border-bottom: 1px solid #e0e0e0;
        }
        QMenuBar::item:selected {
            background-color: #007aff;
            color: #ffffff;
            border-radius: 4px;
        }
        QMenu {
            background-color: #ffffff;
            border: 1px solid #e0e0e0;
            border-radius: 8px;
            padding: 4px;
        }
        QMenu::item:selected {
            background-color: #007aff;
            color: #ffffff;
            border-radius: 4px;
        }
        QSplitter::handle {
            background-color: #e0e0e0;
            width: 1px;
        }
    )";
}

QString StyleSheetManager::darkStyleSheet()
{
    return R"(
        QMainWindow {
            background-color: #1e1e1e;
        }
        QWidget {
            background-color: #1e1e1e;
            color: #e0e0e0;
            font-family: -apple-system, "SF Pro Text", "Helvetica Neue", sans-serif;
            font-size: 14px;
        }
        QListWidget {
            background-color: #2d2d2d;
            border: 1px solid #3d3d3d;
            border-radius: 8px;
            padding: 4px;
            outline: none;
        }
        QListWidget::item {
            padding: 10px 12px;
            border-radius: 6px;
            margin: 2px 0;
        }
        QListWidget::item:selected {
            background-color: #0a84ff;
            color: #ffffff;
        }
        QListWidget::item:hover:!selected {
            background-color: #3d3d3d;
        }
        QTextBrowser {
            background-color: #252525;
            border: 1px solid #3d3d3d;
            border-radius: 8px;
            padding: 16px;
            font-size: 15px;
            color: #e0e0e0;
        }
        QLineEdit {
            padding: 10px 16px;
            border: 2px solid #3d3d3d;
            border-radius: 20px;
            background-color: #2d2d2d;
            color: #e0e0e0;
            font-size: 14px;
        }
        QLineEdit:focus {
            border-color: #0a84ff;
            background-color: #333333;
        }
        QLineEdit:disabled {
            background-color: #252525;
            color: #666666;
        }
        QPushButton {
            padding: 8px 20px;
            border: none;
            border-radius: 16px;
            background-color: #0a84ff;
            color: #ffffff;
            font-size: 14px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #006ecc;
        }
        QPushButton:pressed {
            background-color: #005599;
        }
        QPushButton:disabled {
            background-color: #1a3a5c;
            color: #555555;
        }
        QPushButton#newChatButton {
            background-color: #2d2d2d;
            color: #e0e0e0;
            border: 1px solid #3d3d3d;
        }
        QPushButton#newChatButton:hover {
            background-color: #3d3d3d;
        }
        QMenuBar {
            background-color: #2d2d2d;
            border-bottom: 1px solid #3d3d3d;
            color: #e0e0e0;
        }
        QMenuBar::item:selected {
            background-color: #0a84ff;
            color: #ffffff;
            border-radius: 4px;
        }
        QMenu {
            background-color: #2d2d2d;
            border: 1px solid #3d3d3d;
            border-radius: 8px;
            padding: 4px;
            color: #e0e0e0;
        }
        QMenu::item:selected {
            background-color: #0a84ff;
            color: #ffffff;
            border-radius: 4px;
        }
        QSplitter::handle {
            background-color: #3d3d3d;
            width: 1px;
        }
    )";
}
