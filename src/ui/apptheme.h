#pragma once

#ifndef APPTHEME_H
#define APPTHEME_H

#include <QColor>

struct AppTheme {
    // ── Background hierarchy
    QColor windowBg;
    QColor surfaceBg;
    QColor elevatedBg;

    // ── Text hierarchy
    QColor textPrimary;
    QColor textSecondary;
    QColor textDisabled;
    QColor textOnAccent;

    // ── Accent
    QColor accent;
    QColor accentHover;
    QColor accentPressed;

    // ── Semantic
    QColor warning;
    QColor danger;
    QColor dangerHover;
    QColor success;

    // ── Borders
    QColor border;
    QColor borderFocus;

    // ── Interactive states
    QColor hoverBg;
    QColor selectedBg;
    QColor selectedText;

    // ── Code / Markdown display
    QColor codeBg;
    QColor inlineCodeBg;
    QColor quoteBg;
    QColor quoteBorder;
    QColor quoteText;
    QColor tableBorder;
    QColor tableHeaderBg;

    // ── Syntax highlighting
    QColor syntaxKeyword;
    QColor syntaxString;
    QColor syntaxComment;
    QColor syntaxNumber;
    QColor syntaxFunction;
    QColor syntaxType;
    QColor syntaxOperator;
    QColor syntaxPreprocessor;
    QColor syntaxVariable;

    // ── Widget-specific
    QColor disabledButtonBg;  // washed-out accent for disabled buttons

    // ── Girlfriend-specific (theme-independent pink accent)
    QColor girlfriendAccent;
    QColor girlfriendAccentHover;
    QColor girlfriendAccentPressed;

    // ── Factory methods
    static AppTheme light();
    static AppTheme dark();
    static const AppTheme& current();
};

#endif
