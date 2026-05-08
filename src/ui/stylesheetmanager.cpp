#include "stylesheetmanager.h"
#include "apptheme.h"
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

// ── QSS generation from AppTheme tokens ───────────────────────

static QString buildStyleSheet(const AppTheme& t)
{
    auto c = [](const QColor& color) { return color.name(); };

    return QStringLiteral(
        "QMainWindow {"
        "  background-color: %1;"
        "}"
        "QWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "  font-family: -apple-system, \"SF Pro Text\", \"Helvetica Neue\", sans-serif;"
        "  font-size: 14px;"
        "}"
        "QListWidget {"
        "  background-color: %3;"
        "  border: 1px solid %4;"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "  outline: none;"
        "}"
        "QListWidget::item {"
        "  padding: 10px 12px;"
        "  border-radius: 6px;"
        "  margin: 2px 0;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: %5;"
        "  color: %6;"
        "}"
        "QListWidget::item:hover:!selected {"
        "  background-color: %7;"
        "}"
        "QTextBrowser {"
        "  background-color: %1;"
        "  border: 1px solid %4;"
        "  border-radius: 8px;"
        "  padding: 16px;"
        "  font-size: 15px;"
        "}"
        "QLineEdit {"
        "  padding: 10px 16px;"
        "  border: 2px solid %4;"
        "  border-radius: 20px;"
        "  background-color: %3;"
        "  font-size: 14px;"
        "}"
        "QLineEdit:focus {"
        "  border-color: %5;"
        "  background-color: %1;"
        "}"
        "QLineEdit:disabled {"
        "  background-color: %3;"
        "  color: %8;"
        "}"
        "QPlainTextEdit {"
        "  padding: 10px 16px;"
        "  border: 2px solid %4;"
        "  border-radius: 20px;"
        "  background-color: %3;"
        "  font-size: 14px;"
        "}"
        "QPlainTextEdit:focus {"
        "  border-color: %5;"
        "  background-color: %1;"
        "}"
        "QPlainTextEdit[readOnly=\"true\"] {"
        "  background-color: %3;"
        "  color: %8;"
        "}"
        "QPushButton {"
        "  padding: 8px 20px;"
        "  border: none;"
        "  border-radius: 16px;"
        "  background-color: %5;"
        "  color: %6;"
        "  font-size: 14px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "  background-color: %9;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %10;"
        "}"
        "QPushButton:disabled {"
        "  background-color: %11;"
        "  color: %12;"
        "}"
        "QPushButton#newChatButton {"
        "  background-color: %3;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "}"
        "QPushButton#newChatButton:hover {"
        "  background-color: %7;"
        "}"
        // Search bar
        "QFrame#searchBar {"
        "  background-color: %3;"
        "  border: 2px solid %4;"
        "  border-radius: 10px;"
        "}"
        "QLineEdit#searchInput {"
        "  padding: 8px 14px;"
        "  border: 2px solid %4;"
        "  border-radius: 8px;"
        "  background-color: %1;"
        "  font-size: 14px;"
        "}"
        "QLineEdit#searchInput:focus {"
        "  border-color: %5;"
        "}"
        "QLabel#searchResultLabel {"
        "  color: %13;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QPushButton#searchPrevBtn, QPushButton#searchNextBtn {"
        "  padding: 6px;"
        "  border: 2px solid %4;"
        "  border-radius: 8px;"
        "  background-color: %7;"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  color: %2;"
        "  min-width: 36px;"
        "  min-height: 32px;"
        "}"
        "QPushButton#searchPrevBtn:hover, QPushButton#searchNextBtn:hover {"
        "  background-color: %7;"
        "  border-color: %14;"
        "}"
        "QPushButton#searchCloseBtn {"
        "  padding: 6px;"
        "  border: 2px solid %15;"
        "  border-radius: 8px;"
        "  background-color: %15;"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  color: %6;"
        "  min-width: 36px;"
        "  min-height: 32px;"
        "}"
        "QPushButton#searchCloseBtn:hover {"
        "  background-color: %16;"
        "  border-color: %16;"
        "}"
        "QMenuBar {"
        "  background-color: %3;"
        "  border-bottom: 1px solid %4;"
        "}"
        "QMenuBar::item:selected {"
        "  background-color: %5;"
        "  color: %6;"
        "  border-radius: 4px;"
        "}"
        "QMenu {"
        "  background-color: %1;"
        "  border: 1px solid %4;"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "}"
        "QMenu::item:selected {"
        "  background-color: %5;"
        "  color: %6;"
        "  border-radius: 4px;"
        "}"
        "QSplitter::handle {"
        "  background-color: %4;"
        "  width: 1px;"
        "}"
        "QCheckBox {"
        "  color: %2;"
        "  spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "  border: 2px solid %4;"
        "  border-radius: 4px;"
        "  background-color: %1;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: %5;"
        "  border-color: %5;"
        "}"
        "QCheckBox::indicator:hover {"
        "  border-color: %5;"
        "}"
        "QGroupBox {"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  border-radius: 8px;"
        "  margin-top: 12px;"
        "  padding-top: 8px;"
        "  font-weight: bold;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 12px;"
        "  padding: 0 8px;"
        "  color: %2;"
        "}"
        "QComboBox {"
        "  padding: 6px 12px;"
        "  border: 2px solid %4;"
        "  border-radius: 6px;"
        "  background-color: %1;"
        "  color: %2;"
        "}"
        "QComboBox:hover {"
        "  border-color: %5;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 24px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  selection-background-color: %5;"
        "  selection-color: %6;"
        "}"
        "QRadioButton {"
        "  color: %2;"
        "  spacing: 8px;"
        "}"
        "QRadioButton::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "  border: 2px solid %4;"
        "  border-radius: 9px;"
        "  background-color: %1;"
        "}"
        "QRadioButton::indicator:checked {"
        "  background-color: %5;"
        "  border-color: %5;"
        "}"
        "QLabel {"
        "  color: %2;"
        "}"
        "QLabel#warningLabel {"
        "  color: %17;"
        "}"
    )
    // ── Token mapping: %1 … %17 ──────────────────────────────
    .arg(c(t.windowBg),      // %1
         c(t.textPrimary),   // %2
         c(t.surfaceBg),     // %3
         c(t.border),        // %4
         c(t.accent),        // %5
         c(t.textOnAccent),  // %6
         c(t.hoverBg),       // %7
         c(t.textDisabled),  // %8
         c(t.accentHover),   // %9
         c(t.accentPressed), // %10
         c(t.disabledButtonBg),  // %11 — disabled button bg
         c(t.textSecondary), // %12 — disabled button text (readable contrast)
         c(t.textSecondary), // %13 — search result label
         c(t.textDisabled),  // %14 — search button hover border
         c(t.danger),        // %15 — close/danger button
         c(t.dangerHover),   // %16 — close/danger hover
         c(t.warning));      // %17 — warningLabel
}

QString StyleSheetManager::lightStyleSheet()
{
    return buildStyleSheet(AppTheme::light());
}

QString StyleSheetManager::darkStyleSheet()
{
    return buildStyleSheet(AppTheme::dark());
}
