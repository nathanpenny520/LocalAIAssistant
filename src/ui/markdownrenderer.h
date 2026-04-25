/*
 * Simple Markdown to HTML Renderer
 * Supports: headers, bold, italic, lists, code blocks, blockquotes, tables
 * Theme-aware rendering for light and dark modes
 * MIT License
 */

#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <QString>
#include <QStringList>

// Color configuration for markdown rendering
struct MarkdownColors {
    QString text;           // 正文颜色
    QString secondary;      // 次要文字（标题 h4-h6）
    QString codeBg;         // 代码块背景
    QString inlineCodeBg;   // 行内代码背景
    QString quoteBg;        // 引用块背景
    QString quoteBorder;    // 引用块边框
    QString quoteText;      // 引用块文字
    QString tableBorder;    // 表格边框
    QString tableHeaderBg;  // 表头背景
};

class MarkdownRenderer
{
public:
    // Get colors for the specified theme
    static MarkdownColors getColors(bool isDarkTheme);

    // Convert markdown to HTML with theme-aware styling
    static QString toHtml(const QString &markdown, bool isDarkTheme = false);

private:
    // Color constants
    static MarkdownColors lightColors();
    static MarkdownColors darkColors();

    static QString escapeHtml(const QString &text);
    static QString processHeaders(const QString &line, const MarkdownColors &colors);
    static QString processBoldItalic(const QString &text, const MarkdownColors &colors);
    static QString processTable(const QStringList &tableLines, const MarkdownColors &colors);
    static bool isTableLine(const QString &line);
    static bool isTableSeparatorLine(const QString &line);
};

#endif // MARKDOWNRENDERER_H