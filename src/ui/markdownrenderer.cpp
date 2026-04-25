/*
 * Simple Markdown to HTML Renderer
 * Supports: headers, bold, italic, lists, code blocks, blockquotes, horizontal rules
 */

#include "markdownrenderer.h"
#include <QRegularExpression>
#include <QDebug>

QString MarkdownRenderer::toHtml(const QString &markdown)
{
    QString result;
    QStringList lines = markdown.split('\n');

    bool inCodeBlock = false;
    QString codeBlockContent;
    bool inList = false;
    QString listHtml;

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];

        // Handle code blocks
        if (line.trimmed().startsWith("```")) {
            if (!inCodeBlock) {
                inCodeBlock = true;
                codeBlockContent.clear();
                continue;
            } else {
                inCodeBlock = false;
                result += "<pre style='background-color: #f5f5f5; padding: 12px; border-radius: 8px; overflow-x: auto; font-family: monospace; font-size: 13px;'>"
                          + escapeHtml(codeBlockContent) + "</pre>\n";
                continue;
            }
        }

        if (inCodeBlock) {
            codeBlockContent += line + "\n";
            continue;
        }

        // Close list if we're not in a list item
        if (inList && !line.trimmed().startsWith("-") && !line.trimmed().isEmpty()) {
            result += "<ul style='margin: 8px 0; padding-left: 20px;'>" + listHtml + "</ul>\n";
            listHtml.clear();
            inList = false;
        }

        // Handle empty lines
        if (line.trimmed().isEmpty()) {
            if (inList) {
                result += "<ul style='margin: 8px 0; padding-left: 20px;'>" + listHtml + "</ul>\n";
                listHtml.clear();
                inList = false;
            }
            continue;
        }

        // Handle blockquotes (thinking process)
        if (line.trimmed().startsWith(">")) {
            QString quoteContent = line.trimmed().mid(1).trimmed();
            quoteContent = processBoldItalic(quoteContent);
            result += "<blockquote style='background-color: #f8f8f8; border-left: 4px solid #ccc; padding: 8px 12px; margin: 8px 0; color: #666;'>"
                      + quoteContent + "</blockquote>\n";
            continue;
        }

        // Handle horizontal rules
        if (line.trimmed() == "---" || line.trimmed() == "***" || line.trimmed() == "- - -") {
            result += "<hr style='border: none; border-top: 1px solid #e0e0e0; margin: 16px 0;'>\n";
            continue;
        }

        // Handle headers
        if (line.trimmed().startsWith("#")) {
            result += processHeaders(line.trimmed()) + "\n";
            continue;
        }

        // Handle lists
        if (line.trimmed().startsWith("- ")) {
            inList = true;
            QString itemContent = line.trimmed().mid(2);
            itemContent = processBoldItalic(itemContent);
            listHtml += "<li style='margin: 4px 0;'>" + itemContent + "</li>";
            continue;
        }

        // Handle bold label at start (like "**AI:**")
        if (line.trimmed().startsWith("**") && line.contains(":**")) {
            int colonPos = line.indexOf(":**");
            QString label = line.mid(2, colonPos - 2);
            QString rest = line.mid(colonPos + 3).trimmed();
            rest = processBoldItalic(rest);
            result += "<p style='margin: 12px 0;'><b style='color: #007aff;'>" + escapeHtml(label) + ":</b> " + rest + "</p>\n";
            continue;
        }

        // Regular paragraph
        QString processedLine = processBoldItalic(line);
        result += "<p style='margin: 8px 0; line-height: 1.6;'>" + processedLine + "</p>\n";
    }

    // Close any remaining list
    if (inList) {
        result += "<ul style='margin: 8px 0; padding-left: 20px;'>" + listHtml + "</ul>\n";
    }

    return result;
}

QString MarkdownRenderer::escapeHtml(const QString &text)
{
    QString result = text;
    result.replace("&", "&amp;");
    result.replace("<", "&lt;");
    result.replace(">", "&gt;");
    return result;
}

QString MarkdownRenderer::processHeaders(const QString &line)
{
    int level = 0;
    int pos = 0;

    while (pos < line.size() && line[pos] == '#') {
        level++;
        pos++;
    }

    QString content = line.mid(pos).trimmed();
    content = processBoldItalic(content);

    level = qMin(level, 6); // Max header level is 6

    QString style;
    switch (level) {
    case 1:
        style = "font-size: 24px; font-weight: bold; margin: 20px 0 12px 0; color: #333;";
        break;
    case 2:
        style = "font-size: 20px; font-weight: bold; margin: 16px 0 10px 0; color: #333;";
        break;
    case 3:
        style = "font-size: 18px; font-weight: bold; margin: 14px 0 8px 0; color: #444;";
        break;
    default:
        style = "font-size: 16px; font-weight: bold; margin: 12px 0 6px 0; color: #555;";
        break;
    }

    return QString("<h%1 style='%2'>%3</h%1>").arg(level).arg(style).arg(content);
}

QString MarkdownRenderer::processBoldItalic(const QString &text)
{
    QString result = text;

    // Handle bold **text**
    QRegularExpression boldRegex("\\*\\*(.+?)\\*\\*");
    result.replace(boldRegex, "<b>\\1</b>");

    // Handle italic *text* (single asterisk, not part of bold)
    QRegularExpression italicRegex("\\*(.+?)\\*");
    result.replace(italicRegex, "<i>\\1</i>");

    // Handle inline code `code`
    QRegularExpression codeRegex("`(.+?)`");
    result.replace(codeRegex, "<code style='background-color: #f0f0f0; padding: 2px 6px; border-radius: 4px; font-family: monospace; font-size: 13px;'>\\1</code>");

    return result;
}