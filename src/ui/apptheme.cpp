#include "apptheme.h"

#include <QApplication>
#include <QPalette>

#include "stylesheetmanager.h"

// ── Light theme token values ──────────────────────────────────

AppTheme AppTheme::light() {
    AppTheme t;

    // Background
    t.windowBg = QColor(QStringLiteral("#ffffff"));
    t.surfaceBg = QColor(QStringLiteral("#f5f5f5"));
    t.elevatedBg = QColor(QStringLiteral("#ffffff"));

    // Text
    t.textPrimary = QColor(QStringLiteral("#333333"));
    t.textSecondary = QColor(QStringLiteral("#666666"));
    t.textDisabled = QColor(QStringLiteral("#999999"));
    t.textOnAccent = QColor(QStringLiteral("#ffffff"));

    // Accent
    t.accent = QColor(QStringLiteral("#007aff"));
    t.accentHover = QColor(QStringLiteral("#0062cc"));
    t.accentPressed = QColor(QStringLiteral("#0050a0"));

    // Semantic
    t.warning = QColor(QStringLiteral("#cc5500"));
    t.danger = QColor(QStringLiteral("#dc3545"));
    t.dangerHover = QColor(QStringLiteral("#c82333"));
    t.success = QColor(QStringLiteral("#34c759"));

    // Borders
    t.border = QColor(QStringLiteral("#e0e0e0"));
    t.borderFocus = QColor(QStringLiteral("#007aff"));

    // Interactive
    t.hoverBg = QColor(QStringLiteral("#e8e8e8"));
    t.selectedBg = QColor(QStringLiteral("#007aff"));
    t.selectedText = QColor(QStringLiteral("#ffffff"));

    // Markdown
    t.codeBg = QColor(QStringLiteral("#f5f5f5"));
    t.inlineCodeBg = QColor(QStringLiteral("#f0f0f0"));
    t.quoteBg = QColor(QStringLiteral("#f8f8f8"));
    t.quoteBorder = QColor(QStringLiteral("#cccccc"));
    t.quoteText = QColor(QStringLiteral("#666666"));
    t.tableBorder = QColor(QStringLiteral("#e0e0e0"));
    t.tableHeaderBg = QColor(QStringLiteral("#f5f5f5"));

    // Syntax (light)
    t.syntaxKeyword = QColor(QStringLiteral("#0000ff"));
    t.syntaxString = QColor(QStringLiteral("#a31515"));
    t.syntaxComment = QColor(QStringLiteral("#008000"));
    t.syntaxNumber = QColor(QStringLiteral("#098658"));
    t.syntaxFunction = QColor(QStringLiteral("#795e26"));
    t.syntaxType = QColor(QStringLiteral("#267f99"));
    t.syntaxOperator = QColor(QStringLiteral("#666666"));
    t.syntaxPreprocessor = QColor(QStringLiteral("#af00db"));
    t.syntaxVariable = QColor(QStringLiteral("#001080"));

    // Widget-specific
    t.disabledButtonBg = QColor(QStringLiteral("#b0d0ff"));

    // Girlfriend (theme-independent pink)
    t.girlfriendAccent = QColor(QStringLiteral("#e91e63"));
    t.girlfriendAccentHover = QColor(QStringLiteral("#c2185b"));
    t.girlfriendAccentPressed = QColor(QStringLiteral("#d81b60"));

    return t;
}

// ── Dark theme token values ───────────────────────────────────

AppTheme AppTheme::dark() {
    AppTheme t;

    // Background
    t.windowBg = QColor(QStringLiteral("#1e1e1e"));
    t.surfaceBg = QColor(QStringLiteral("#2d2d2d"));
    t.elevatedBg = QColor(QStringLiteral("#2d2d2d"));

    // Text
    t.textPrimary = QColor(QStringLiteral("#e0e0e0"));
    t.textSecondary = QColor(QStringLiteral("#a0a0a0"));
    t.textDisabled = QColor(QStringLiteral("#666666"));
    t.textOnAccent = QColor(QStringLiteral("#ffffff"));

    // Accent
    t.accent = QColor(QStringLiteral("#0a84ff"));
    t.accentHover = QColor(QStringLiteral("#006ecc"));
    t.accentPressed = QColor(QStringLiteral("#005599"));

    // Semantic
    t.warning = QColor(QStringLiteral("#ffaa33"));
    t.danger = QColor(QStringLiteral("#dc3545"));
    t.dangerHover = QColor(QStringLiteral("#c82333"));
    t.success = QColor(QStringLiteral("#34c759"));

    // Borders
    t.border = QColor(QStringLiteral("#3d3d3d"));
    t.borderFocus = QColor(QStringLiteral("#0a84ff"));

    // Interactive
    t.hoverBg = QColor(QStringLiteral("#3d3d3d"));
    t.selectedBg = QColor(QStringLiteral("#0a84ff"));
    t.selectedText = QColor(QStringLiteral("#ffffff"));

    // Markdown
    t.codeBg = QColor(QStringLiteral("#2d2d2d"));
    t.inlineCodeBg = QColor(QStringLiteral("#3d3d3d"));
    t.quoteBg = QColor(QStringLiteral("#2a2a2a"));
    t.quoteBorder = QColor(QStringLiteral("#555555"));
    t.quoteText = QColor(QStringLiteral("#a0a0a0"));
    t.tableBorder = QColor(QStringLiteral("#3d3d3d"));
    t.tableHeaderBg = QColor(QStringLiteral("#2d2d2d"));

    // Syntax (dark — VS Code-inspired)
    t.syntaxKeyword = QColor(QStringLiteral("#c586c0"));
    t.syntaxString = QColor(QStringLiteral("#ce9178"));
    t.syntaxComment = QColor(QStringLiteral("#6a9955"));
    t.syntaxNumber = QColor(QStringLiteral("#b5cea8"));
    t.syntaxFunction = QColor(QStringLiteral("#dcdcaa"));
    t.syntaxType = QColor(QStringLiteral("#4ec9b0"));
    t.syntaxOperator = QColor(QStringLiteral("#d4d4d4"));
    t.syntaxPreprocessor = QColor(QStringLiteral("#c586c0"));
    t.syntaxVariable = QColor(QStringLiteral("#9cdcfe"));

    // Widget-specific
    t.disabledButtonBg = QColor(QStringLiteral("#1a3a5c"));

    // Girlfriend (theme-independent pink — same as light)
    t.girlfriendAccent = QColor(QStringLiteral("#e91e63"));
    t.girlfriendAccentHover = QColor(QStringLiteral("#c2185b"));
    t.girlfriendAccentPressed = QColor(QStringLiteral("#d81b60"));

    return t;
}

// ── Current theme accessor ────────────────────────────────────

const AppTheme& AppTheme::current() {
    static AppTheme cacheLight;
    static AppTheme cacheDark;
    static int cachedTheme = -1;

    StyleSheetManager::Theme current = StyleSheetManager::instance()->currentTheme();
    int themeKey;
    if (current == StyleSheetManager::DarkTheme) {
        themeKey = 1;
    } else if (current == StyleSheetManager::LightTheme) {
        themeKey = 0;
    } else {
        // SystemTheme — detect from OS palette
        QPalette palette = QApplication::palette();
        QColor windowColor = palette.color(QPalette::Window);
        int brightness =
                (windowColor.red() * 299 + windowColor.green() * 587 + windowColor.blue() * 114) /
                1000;
        themeKey = (brightness < 128) ? 1 : 0;
    }

    if (cachedTheme != themeKey) {
        if (themeKey == 1) {
            cacheDark = dark();
        } else {
            cacheLight = light();
        }
        cachedTheme = themeKey;
    }

    return (themeKey == 1) ? cacheDark : cacheLight;
}
