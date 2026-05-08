#pragma once

/*
 * Simple Markdown to HTML Renderer
 * Supports: headers, bold, italic, lists, code blocks, blockquotes, tables
 * Theme-aware rendering for light and dark modes
 * Built-in syntax highlighting for common languages
 * MIT License
 */

#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <QString>
#include <QStringList>
#include "apptheme.h"

class MarkdownRenderer
{
public:
    // Convert markdown to HTML with theme-aware styling
    static QString toHtml(const QString &markdown);

private:
    // Syntax highlighting
    static QString highlightCode(const QString &code, const QString &language, const AppTheme &t);
    static QString highlightCpp(const QString &code, const AppTheme &t);
    static QString highlightPython(const QString &code, const AppTheme &t);
    static QString highlightJs(const QString &code, const AppTheme &t);
    static QString highlightJson(const QString &code, const AppTheme &t);
    static QString highlightBash(const QString &code, const AppTheme &t);
    static QString highlightGeneric(const QString &code, const AppTheme &t);

    static QString escapeHtml(const QString &text);
    static QString processHeaders(const QString &line, const AppTheme &t);
    static QString processBoldItalic(const QString &text, const AppTheme &t);
    static QString processTable(const QStringList &tableLines, const AppTheme &t);
    static bool isTableLine(const QString &line);
    static bool isTableSeparatorLine(const QString &line);
    static QStringList parseTableRow(const QString &line);
    static QStringList parseAlignmentFromSeparator(const QString &line);
    static QString getAlignment(const QStringList &rules, int index);
};

#endif // MARKDOWNRENDERER_H
