/*
 * Simple Markdown to HTML Renderer
 * Supports: headers, bold, italic, lists, code blocks, blockquotes, tables
 * Theme-aware rendering for light and dark modes
 */

#include "markdownrenderer.h"
#include <QRegularExpression>
#include <QDebug>

MarkdownColors MarkdownRenderer::lightColors()
{
    return {
        "#333333",  // text
        "#555555",  // secondary
        "#f5f5f5",  // codeBg
        "#f0f0f0",  // inlineCodeBg
        "#f8f8f8",  // quoteBg
        "#cccccc",  // quoteBorder
        "#666666",  // quoteText
        "#e0e0e0",  // tableBorder
        "#f5f5f5"   // tableHeaderBg
    };
}

MarkdownColors MarkdownRenderer::darkColors()
{
    return {
        "#e0e0e0",  // text
        "#a0a0a0",  // secondary
        "#2d2d2d",  // codeBg
        "#3d3d3d",  // inlineCodeBg
        "#2a2a2a",  // quoteBg
        "#555555",  // quoteBorder
        "#a0a0a0",  // quoteText
        "#3d3d3d",  // tableBorder
        "#2d2d2d"   // tableHeaderBg
    };
}

MarkdownColors MarkdownRenderer::getColors(bool isDarkTheme)
{
    return isDarkTheme ? darkColors() : lightColors();
}

QString MarkdownRenderer::toHtml(const QString &markdown, bool isDarkTheme)
{
    MarkdownColors colors = getColors(isDarkTheme);

    QString result;
    QStringList lines = markdown.split('\n');

    bool inCodeBlock = false;
    QString codeBlockContent;
    bool inList = false;
    QString listHtml;
    bool inTable = false;
    QStringList tableLines;

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
                result += QString("<pre style='background-color: %1; padding: 12px; border-radius: 8px; overflow-x: auto; font-family: monospace; font-size: 13px; color: %2;'>")
                          .arg(colors.codeBg, colors.text)
                          + escapeHtml(codeBlockContent) + "</pre>\n";
                continue;
            }
        }

        if (inCodeBlock) {
            codeBlockContent += line + "\n";
            continue;
        }

        // Handle tables
        if (isTableLine(line)) {
            if (!inTable) {
                inTable = true;
                tableLines.clear();
            }
            tableLines.append(line);
            continue;
        } else if (inTable && line.trimmed().isEmpty()) {
            // End of table
            result += processTable(tableLines, colors);
            tableLines.clear();
            inTable = false;
            continue;
        } else if (inTable) {
            // Table ended, process it and continue with current line
            result += processTable(tableLines, colors);
            tableLines.clear();
            inTable = false;
        }

        // Close list if we're not in a list item
        if (inList && !line.trimmed().startsWith("-") && !line.trimmed().isEmpty()) {
            result += QString("<ul style='margin: 8px 0; padding-left: 20px; color: %1;'>").arg(colors.text) + listHtml + "</ul>\n";
            listHtml.clear();
            inList = false;
        }

        // Handle empty lines
        if (line.trimmed().isEmpty()) {
            if (inList) {
                result += QString("<ul style='margin: 8px 0; padding-left: 20px; color: %1;'>").arg(colors.text) + listHtml + "</ul>\n";
                listHtml.clear();
                inList = false;
            }
            continue;
        }

        // Handle blockquotes (thinking process)
        if (line.trimmed().startsWith(">")) {
            QString quoteContent = line.trimmed().mid(1).trimmed();
            quoteContent = processBoldItalic(quoteContent, colors);
            result += QString("<blockquote style='background-color: %1; border-left: 4px solid %2; padding: 8px 12px; margin: 8px 0; color: %3;'>")
                      .arg(colors.quoteBg, colors.quoteBorder, colors.quoteText)
                      + quoteContent + "</blockquote>\n";
            continue;
        }

        // Handle horizontal rules
        if (line.trimmed() == "---" || line.trimmed() == "***" || line.trimmed() == "- - -") {
            result += QString("<hr style='border: none; border-top: 1px solid %1; margin: 16px 0;'>\n").arg(colors.tableBorder);
            continue;
        }

        // Handle headers
        if (line.trimmed().startsWith("#")) {
            result += processHeaders(line.trimmed(), colors) + "\n";
            continue;
        }

        // Handle lists
        if (line.trimmed().startsWith("- ")) {
            inList = true;
            QString itemContent = line.trimmed().mid(2);
            itemContent = processBoldItalic(itemContent, colors);
            listHtml += QString("<li style='margin: 4px 0;'>%1</li>").arg(itemContent);
            continue;
        }

        // Handle bold label at start (like "**AI:**")
        if (line.trimmed().startsWith("**") && line.contains(":**")) {
            int colonPos = line.indexOf(":**");
            QString label = line.mid(2, colonPos - 2);
            QString rest = line.mid(colonPos + 3).trimmed();
            rest = processBoldItalic(rest, colors);
            result += QString("<p style='margin: 12px 0; color: %1;'><b style='color: #007aff;'>").arg(colors.text)
                      + escapeHtml(label) + ":</b> " + rest + "</p>\n";
            continue;
        }

        // Regular paragraph
        QString processedLine = processBoldItalic(line, colors);
        result += QString("<p style='margin: 8px 0; line-height: 1.6; color: %1;'>").arg(colors.text) + processedLine + "</p>\n";
    }

    // Close any remaining open elements
    if (inList) {
        result += QString("<ul style='margin: 8px 0; padding-left: 20px; color: %1;'>").arg(colors.text) + listHtml + "</ul>\n";
    }
    if (inTable) {
        result += processTable(tableLines, colors);
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

QString MarkdownRenderer::processHeaders(const QString &line, const MarkdownColors &colors)
{
    int level = 0;
    int pos = 0;

    while (pos < line.size() && line[pos] == '#') {
        level++;
        pos++;
    }

    QString content = line.mid(pos).trimmed();
    content = processBoldItalic(content, colors);

    level = qMin(level, 6); // Max header level is 6

    QString style;
    QString headerColor;
    switch (level) {
    case 1:
        style = "font-size: 24px; font-weight: bold; margin: 20px 0 12px 0;";
        headerColor = colors.text;
        break;
    case 2:
        style = "font-size: 20px; font-weight: bold; margin: 16px 0 10px 0;";
        headerColor = colors.text;
        break;
    case 3:
        style = "font-size: 18px; font-weight: bold; margin: 14px 0 8px 0;";
        headerColor = colors.text;
        break;
    default:
        style = "font-size: 16px; font-weight: bold; margin: 12px 0 6px 0;";
        headerColor = colors.secondary;
        break;
    }

    return QString("<h%1 style='%2 color: %3;'>%4</h%1>").arg(level).arg(style).arg(headerColor).arg(content);
}

QString MarkdownRenderer::processBoldItalic(const QString &text, const MarkdownColors &colors)
{
    QString result = text;

    // Handle bold **text**
    QRegularExpression boldRegex("\\*\\*(.+?)\\*\\*");
    result.replace(boldRegex, QString("<b style='color: %1;'>\\1</b>").arg(colors.text));

    // Handle italic *text* (single asterisk, not part of bold)
    QRegularExpression italicRegex("\\*(.+?)\\*");
    result.replace(italicRegex, "<i>\\1</i>");

    // Handle inline code `code`
    QRegularExpression codeRegex("`(.+?)`");
    result.replace(codeRegex, QString("<code style='background-color: %1; padding: 2px 6px; border-radius: 4px; font-family: monospace; font-size: 13px; color: %2;'>\\1</code>")
                   .arg(colors.inlineCodeBg, colors.text));

    return result;
}

bool MarkdownRenderer::isTableLine(const QString &line)
{
    QString trimmed = line.trimmed();
    return trimmed.startsWith("|") && trimmed.endsWith("|");
}

bool MarkdownRenderer::isTableSeparatorLine(const QString &line)
{
    QString trimmed = line.trimmed();
    if (!trimmed.startsWith("|") || !trimmed.endsWith("|")) {
        return false;
    }
    // Check if content is only dashes, colons, and pipes
    QString content = trimmed.mid(1, trimmed.length() - 2);
    static QRegularExpression sepRegex("^[:\\-\\s|]+$");
    return sepRegex.match(content).hasMatch();
}

QString MarkdownRenderer::processTable(const QStringList &tableLines, const MarkdownColors &colors)
{
    if (tableLines.isEmpty()) {
        return "";
    }

    QString result;
    result += QString("<table style='border-collapse: collapse; width: 100%; margin: 12px 0;'>\n");

    bool hasHeader = false;
    QStringList alignmentRules;
    int dataStartIndex = 0;

    // Check for separator line (indicates header)
    for (int i = 0; i < tableLines.size(); ++i) {
        if (isTableSeparatorLine(tableLines[i])) {
            hasHeader = true;
            alignmentRules = parseAlignmentFromSeparator(tableLines[i]);
            dataStartIndex = i + 1;
            break;
        }
    }

    // Process header row if exists
    if (hasHeader && tableLines.size() > 0) {
        QStringList headerCells = parseTableRow(tableLines[0]);
        result += QString("<thead><tr style='background-color: %1;'>\n").arg(colors.tableHeaderBg);
        for (int i = 0; i < headerCells.size(); ++i) {
            QString align = getAlignment(alignmentRules, i);
            QString cellContent = processBoldItalic(headerCells[i].trimmed(), colors);
            result += QString("<th style='border: 1px solid %1; padding: 8px; %2 color: %3;'>%4</th>\n")
                      .arg(colors.tableBorder, align, colors.text, cellContent);
        }
        result += "</tr></thead>\n<tbody>\n";
    } else {
        dataStartIndex = 0;
        result += "<tbody>\n";
    }

    // Process data rows
    for (int i = dataStartIndex; i < tableLines.size(); ++i) {
        if (isTableSeparatorLine(tableLines[i])) {
            continue;
        }
        QStringList cells = parseTableRow(tableLines[i]);
        result += "<tr>\n";
        for (int j = 0; j < cells.size(); ++j) {
            QString align = hasHeader ? getAlignment(alignmentRules, j) : "";
            QString cellContent = processBoldItalic(cells[j].trimmed(), colors);
            result += QString("<td style='border: 1px solid %1; padding: 8px; %2 color: %3;'>%4</td>\n")
                      .arg(colors.tableBorder, align, colors.text, cellContent);
        }
        result += "</tr>\n";
    }

    result += "</tbody>\n</table>\n";
    return result;
}

QStringList MarkdownRenderer::parseTableRow(const QString &line)
{
    QString trimmed = line.trimmed();
    // Remove leading and trailing pipes
    if (trimmed.startsWith("|")) {
        trimmed = trimmed.mid(1);
    }
    if (trimmed.endsWith("|")) {
        trimmed = trimmed.mid(0, trimmed.length() - 1);
    }
    // Split by pipe
    return trimmed.split("|");
}

QStringList MarkdownRenderer::parseAlignmentFromSeparator(const QString &line)
{
    QStringList cells = parseTableRow(line);
    QStringList alignments;
    for (const QString &cell : cells) {
        QString trimmed = cell.trimmed();
        if (trimmed.startsWith(":") && trimmed.endsWith(":")) {
            alignments.append("text-align: center;");
        } else if (trimmed.endsWith(":")) {
            alignments.append("text-align: right;");
        } else {
            alignments.append("text-align: left;");
        }
    }
    return alignments;
}

QString MarkdownRenderer::getAlignment(const QStringList &rules, int index)
{
    if (index >= 0 && index < rules.size()) {
        return rules[index];
    }
    return "text-align: left;";
}