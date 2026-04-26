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

// Syntax highlighting color theme
struct SyntaxTheme {
    QString keyword;        // 关键字
    QString string;         // 字符串
    QString comment;        // 注释
    QString number;         // 数字
    QString function;       // 函数名
    QString type;           // 类型名
    QString operator_;      // 操作符
    QString preprocessor;   // 预处理指令
    QString variable;       // 变量
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

    // Syntax highlighting themes
    static SyntaxTheme lightSyntaxTheme();
    static SyntaxTheme darkSyntaxTheme();
    static SyntaxTheme getSyntaxTheme(bool isDarkTheme);

    // Syntax highlighting
    static QString highlightCode(const QString &code, const QString &language, const SyntaxTheme &theme);
    static QString highlightCpp(const QString &code, const SyntaxTheme &theme);
    static QString highlightPython(const QString &code, const SyntaxTheme &theme);
    static QString highlightJs(const QString &code, const SyntaxTheme &theme);
    static QString highlightJson(const QString &code, const SyntaxTheme &theme);
    static QString highlightBash(const QString &code, const SyntaxTheme &theme);
    static QString highlightGeneric(const QString &code, const SyntaxTheme &theme);

    static QString escapeHtml(const QString &text);
    static QString processHeaders(const QString &line, const MarkdownColors &colors);
    static QString processBoldItalic(const QString &text, const MarkdownColors &colors);
    static QString processTable(const QStringList &tableLines, const MarkdownColors &colors);
    static bool isTableLine(const QString &line);
    static bool isTableSeparatorLine(const QString &line);
    static QStringList parseTableRow(const QString &line);
    static QStringList parseAlignmentFromSeparator(const QString &line);
    static QString getAlignment(const QStringList &rules, int index);
};

#endif // MARKDOWNRENDERER_H