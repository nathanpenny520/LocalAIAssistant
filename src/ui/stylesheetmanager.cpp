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
        /* 搜索栏样式 */
        QFrame#searchBar {
            background-color: #f5f5f5;
            border: 2px solid #d0d0d0;
            border-radius: 10px;
        }
        QLineEdit#searchInput {
            padding: 8px 14px;
            border: 2px solid #c0c0c0;
            border-radius: 8px;
            background-color: #ffffff;
            font-size: 14px;
        }
        QLineEdit#searchInput:focus {
            border-color: #007aff;
        }
        QLabel#searchResultLabel {
            color: #666666;
            font-size: 13px;
            font-weight: bold;
        }
        QPushButton#searchPrevBtn, QPushButton#searchNextBtn {
            padding: 6px;
            border: 2px solid #c0c0c0;
            border-radius: 8px;
            background-color: #e8e8e8;
            font-size: 18px;
            font-weight: bold;
            color: #333333;
            min-width: 36px;
            min-height: 32px;
        }
        QPushButton#searchPrevBtn:hover, QPushButton#searchNextBtn:hover {
            background-color: #d8d8d8;
            border-color: #999999;
        }
        QPushButton#searchCloseBtn {
            padding: 6px;
            border: 2px solid #dc3545;
            border-radius: 8px;
            background-color: #dc3545;
            font-size: 18px;
            font-weight: bold;
            color: #ffffff;
            min-width: 36px;
            min-height: 32px;
        }
        QPushButton#searchCloseBtn:hover {
            background-color: #c82333;
            border-color: #c82333;
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
        QCheckBox {
            color: #333333;
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 2px solid #e0e0e0;
            border-radius: 4px;
            background-color: #ffffff;
        }
        QCheckBox::indicator:checked {
            background-color: #007aff;
            border-color: #007aff;
        }
        QCheckBox::indicator:hover {
            border-color: #007aff;
        }
        QGroupBox {
            color: #333333;
            border: 1px solid #e0e0e0;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 8px;
            font-weight: bold;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 8px;
            color: #333333;
        }
        QComboBox {
            padding: 6px 12px;
            border: 2px solid #e0e0e0;
            border-radius: 6px;
            background-color: #ffffff;
            color: #333333;
        }
        QComboBox:hover {
            border-color: #007aff;
        }
        QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox QAbstractItemView {
            background-color: #ffffff;
            color: #333333;
            border: 1px solid #e0e0e0;
            selection-background-color: #007aff;
            selection-color: #ffffff;
        }
        QRadioButton {
            color: #333333;
            spacing: 8px;
        }
        QRadioButton::indicator {
            width: 18px;
            height: 18px;
            border: 2px solid #e0e0e0;
            border-radius: 9px;
            background-color: #ffffff;
        }
        QRadioButton::indicator:checked {
            background-color: #007aff;
            border-color: #007aff;
        }
        QLabel {
            color: #333333;
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
        /* 搜索栏样式 */
        QFrame#searchBar {
            background-color: #2d2d2d;
            border: 2px solid #4d4d4d;
            border-radius: 10px;
        }
        QLineEdit#searchInput {
            padding: 8px 14px;
            border: 2px solid #4d4d4d;
            border-radius: 8px;
            background-color: #3a3a3a;
            font-size: 14px;
            color: #e0e0e0;
        }
        QLineEdit#searchInput:focus {
            border-color: #0a84ff;
        }
        QLabel#searchResultLabel {
            color: #888888;
            font-size: 13px;
            font-weight: bold;
        }
        QPushButton#searchPrevBtn, QPushButton#searchNextBtn {
            padding: 6px;
            border: 2px solid #4d4d4d;
            border-radius: 8px;
            background-color: #3d3d3d;
            font-size: 18px;
            font-weight: bold;
            color: #e0e0e0;
            min-width: 36px;
            min-height: 32px;
        }
        QPushButton#searchPrevBtn:hover, QPushButton#searchNextBtn:hover {
            background-color: #4d4d4d;
            border-color: #5d5d5d;
        }
        QPushButton#searchCloseBtn {
            padding: 6px;
            border: 2px solid #dc3545;
            border-radius: 8px;
            background-color: #dc3545;
            font-size: 18px;
            font-weight: bold;
            color: #ffffff;
            min-width: 36px;
            min-height: 32px;
        }
        QPushButton#searchCloseBtn:hover {
            background-color: #c82333;
            border-color: #c82333;
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
        QCheckBox {
            color: #e0e0e0;
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 2px solid #3d3d3d;
            border-radius: 4px;
            background-color: #2d2d2d;
        }
        QCheckBox::indicator:checked {
            background-color: #0a84ff;
            border-color: #0a84ff;
        }
        QCheckBox::indicator:hover {
            border-color: #0a84ff;
        }
        QGroupBox {
            color: #e0e0e0;
            border: 1px solid #3d3d3d;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 8px;
            font-weight: bold;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 8px;
            color: #e0e0e0;
        }
        QComboBox {
            padding: 6px 12px;
            border: 2px solid #3d3d3d;
            border-radius: 6px;
            background-color: #2d2d2d;
            color: #e0e0e0;
        }
        QComboBox:hover {
            border-color: #0a84ff;
        }
        QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox QAbstractItemView {
            background-color: #2d2d2d;
            color: #e0e0e0;
            border: 1px solid #3d3d3d;
            selection-background-color: #0a84ff;
            selection-color: #ffffff;
        }
        QRadioButton {
            color: #e0e0e0;
            spacing: 8px;
        }
        QRadioButton::indicator {
            width: 18px;
            height: 18px;
            border: 2px solid #3d3d3d;
            border-radius: 9px;
            background-color: #2d2d2d;
        }
        QRadioButton::indicator:checked {
            background-color: #0a84ff;
            border-color: #0a84ff;
        }
        QLabel {
            color: #e0e0e0;
        }
    )";
}
