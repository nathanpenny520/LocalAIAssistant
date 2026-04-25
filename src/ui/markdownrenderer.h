/*
 * Simple Markdown to HTML Renderer
 * Supports: headers, bold, italic, lists, code blocks, blockquotes
 * MIT License
 */

#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <QString>
#include <QStringList>

class MarkdownRenderer
{
public:
    static QString toHtml(const QString &markdown);

private:
    static QString escapeHtml(const QString &text);
    static QString processCodeBlocks(QString &markdown);
    static QString processHeaders(const QString &line);
    static QString processBoldItalic(const QString &text);
    static QString processLists(QStringList &lines);
    static QString processBlockquote(const QString &line);
};

#endif // MARKDOWNRENDERER_H