/*
 * Simple Markdown to HTML Renderer
 * Supports: headers, bold, italic, lists, code blocks, blockquotes, tables
 * Theme-aware rendering for light and dark modes
 * Built-in syntax highlighting for common languages
 */

#include "markdownrenderer.h"
#include <QRegularExpression>
#include <QDebug>

// Helper struct for replacement tracking
struct ReplacementInfo {
    qsizetype start;
    qsizetype length;
    QString text;
};

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

// Syntax highlighting themes
SyntaxTheme MarkdownRenderer::lightSyntaxTheme()
{
    return {
        "#0000ff",  // keyword - blue
        "#a31515",  // string - dark red
        "#008000",  // comment - green
        "#098658",  // number - teal
        "#795e26",  // function - brown/gold
        "#267f99",  // type - teal-blue
        "#666666",  // operator - gray
        "#af00db",  // preprocessor - purple
        "#001080"   // variable - dark blue
    };
}

SyntaxTheme MarkdownRenderer::darkSyntaxTheme()
{
    return {
        "#c586c0",  // keyword - purple/pink
        "#ce9178",  // string - orange
        "#6a9955",  // comment - green
        "#b5cea8",  // number - light green
        "#dcdcaa",  // function - yellow
        "#4ec9b0",  // type - teal
        "#d4d4d4",  // operator - light gray
        "#c586c0",  // preprocessor - purple
        "#9cdcfe"   // variable - light blue
    };
}

SyntaxTheme MarkdownRenderer::getSyntaxTheme(bool isDarkTheme)
{
    return isDarkTheme ? darkSyntaxTheme() : lightSyntaxTheme();
}

QString MarkdownRenderer::toHtml(const QString &markdown, bool isDarkTheme)
{
    MarkdownColors colors = getColors(isDarkTheme);
    SyntaxTheme syntaxTheme = getSyntaxTheme(isDarkTheme);

    QString result;
    QStringList lines = markdown.split('\n');

    bool inCodeBlock = false;
    QString codeBlockContent;
    QString codeBlockLanguage;
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
                // Extract language identifier
                QString lang = line.trimmed().mid(3).trimmed().toLower();
                // Normalize language names
                if (lang == "c++" || lang == "cpp" || lang == "cc" || lang == "cxx") {
                    codeBlockLanguage = "cpp";
                } else if (lang == "c") {
                    codeBlockLanguage = "c";
                } else if (lang == "python" || lang == "py") {
                    codeBlockLanguage = "python";
                } else if (lang == "javascript" || lang == "js" || lang == "node") {
                    codeBlockLanguage = "javascript";
                } else if (lang == "typescript" || lang == "ts") {
                    codeBlockLanguage = "typescript";
                } else if (lang == "json") {
                    codeBlockLanguage = "json";
                } else if (lang == "bash" || lang == "sh" || lang == "shell" || lang == "zsh") {
                    codeBlockLanguage = "bash";
                } else if (lang.isEmpty()) {
                    codeBlockLanguage = "generic";
                } else {
                    codeBlockLanguage = lang;
                }
                continue;
            } else {
                inCodeBlock = false;
                // Apply syntax highlighting
                QString highlightedCode = highlightCode(codeBlockContent, codeBlockLanguage, syntaxTheme);
                result += QString("<pre style='background-color: %1; padding: 12px; border-radius: 8px; overflow-x: auto; font-family: monospace; font-size: 13px;'><code style='color: %2;'>")
                          .arg(colors.codeBg, colors.text)
                          + highlightedCode + "</code></pre>\n";
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

// ============================================================================
// Syntax Highlighting Implementation
// ============================================================================

QString MarkdownRenderer::highlightCode(const QString &code, const QString &language, const SyntaxTheme &theme)
{
    QString escaped = escapeHtml(code);

    if (language == "cpp" || language == "c") {
        return highlightCpp(escaped, theme);
    } else if (language == "python") {
        return highlightPython(escaped, theme);
    } else if (language == "javascript" || language == "typescript") {
        return highlightJs(escaped, theme);
    } else if (language == "json") {
        return highlightJson(escaped, theme);
    } else if (language == "bash") {
        return highlightBash(escaped, theme);
    } else {
        return highlightGeneric(escaped, theme);
    }
}

QString MarkdownRenderer::highlightCpp(const QString &code, const SyntaxTheme &theme)
{
    QString result = code;

    // Helper lambda for regex replacement with proper handling
    auto replaceWithStyle = [&result, &theme](const QRegularExpression &regex, const QString &color, bool bold = false) {
        QRegularExpressionMatchIterator it = regex.globalMatch(result);
        QList<ReplacementInfo> replacements;

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString matched = match.captured(0);
            QString styled = bold
                ? QString("<span style=\"color: %1; font-weight: bold;\">%2</span>").arg(color, matched)
                : QString("<span style=\"color: %1;\">%2</span>").arg(color, matched);
            replacements.append({match.capturedStart(), match.capturedLength(), styled});
        }

        // Apply replacements from end to start
        for (int i = replacements.size() - 1; i >= 0; --i) {
            result.replace(replacements[i].start, replacements[i].length, replacements[i].text);
        }
    };

    // Process order: strings/preprocessor first, then keywords, comments last
    // This prevents regex from matching content inside already-processed spans

    // Strings (use negative lookbehind for HTML attributes)
    QRegularExpression stringLiteral("(?<!style=)\"(?:\\\\\"|[^\"])+\"");
    QRegularExpression charLiteral("(?<!style=)'(?:\\\\'|[^'])+'");
    replaceWithStyle(stringLiteral, theme.string);
    replaceWithStyle(charLiteral, theme.string);

    // Keywords
    QString cppKeywords = "\\b(alignas|alignof|and|and_eq|asm|auto|bitand|bitor|bool|break|case|catch|char|char8_t|char16_t|char32_t|class|compl|concept|const|consteval|constexpr|const_cast|continue|co_await|co_return|co_yield|decltype|default|delete|do|double|dynamic_cast|else|enum|explicit|export|extern|false|float|for|friend|goto|if|inline|int|long|mutable|namespace|new|noexcept|not|not_eq|nullptr|operator|or|or_eq|private|protected|public|register|reinterpret_cast|requires|return|short|signed|sizeof|static|static_assert|static_cast|struct|switch|template|this|thread_local|throw|true|try|typedef|typeid|typename|union|unsigned|using|virtual|void|volatile|wchar_t|while|xor|xor_eq)\\b";
    QRegularExpression keywordRegex(cppKeywords);
    replaceWithStyle(keywordRegex, theme.keyword, true);

    // Types (common C++ types)
    QString cppTypes = "\\b(std|string|vector|map|set|list|array|deque|queue|stack|pair|tuple|optional|variant|function|unique_ptr|shared_ptr|weak_ptr|make_unique|make_shared|size_t|int8_t|int16_t|int32_t|int64_t|uint8_t|uint16_t|uint32_t|uint64_t|ptrdiff_t|nullptr_t)\\b";
    QRegularExpression typeRegex(cppTypes);
    replaceWithStyle(typeRegex, theme.type);

    // Numbers
    QRegularExpression numberRegex("\\b(\\d+\\.?\\d*|0x[0-9a-fA-F]+|0b[01]+)\\b");
    replaceWithStyle(numberRegex, theme.number);

    // Function calls - match word before parentheses
    QRegularExpression functionRegex("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*(?=\\()");
    QRegularExpressionMatchIterator it = functionRegex.globalMatch(result);
    QList<ReplacementInfo> funcReplacements;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString funcName = match.captured(1);
        QString styled = QString("<span style=\"color: %1;\">%2</span>").arg(theme.function, funcName);
        funcReplacements.append({match.capturedStart(1), match.capturedLength(1), styled});
    }
    for (int i = funcReplacements.size() - 1; i >= 0; --i) {
        result.replace(funcReplacements[i].start, funcReplacements[i].length, funcReplacements[i].text);
    }

    // Preprocessor directives - process before comments
    QRegularExpression preprocessor("#[^\\n]*");
    replaceWithStyle(preprocessor, theme.preprocessor);

    // Comments - process LAST to avoid matching color codes in span tags
    QRegularExpression multiLineComment("/\\*[^*]*\\*+(?:[^/*][^*]*\\*+)*/");
    QRegularExpression singleLineComment("//[^\n]*");
    replaceWithStyle(multiLineComment, theme.comment);
    replaceWithStyle(singleLineComment, theme.comment);

    return result;
}

QString MarkdownRenderer::highlightPython(const QString &code, const SyntaxTheme &theme)
{
    QString result = code;

    // Helper lambda for regex replacement
    auto replaceWithStyle = [&result, &theme](const QRegularExpression &regex, const QString &color, bool bold = false) {
        QRegularExpressionMatchIterator it = regex.globalMatch(result);
        QList<ReplacementInfo> replacements;

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString matched = match.captured(0);
            QString styled = bold
                ? QString("<span style=\"color: %1; font-weight: bold;\">%2</span>").arg(color, matched)
                : QString("<span style=\"color: %1;\">%2</span>").arg(color, matched);
            replacements.append({match.capturedStart(), match.capturedLength(), styled});
        }

        for (int i = replacements.size() - 1; i >= 0; --i) {
            result.replace(replacements[i].start, replacements[i].length, replacements[i].text);
        }
    };

    // Process order: strings first, then keywords, comments last
    // This prevents regex from matching content inside already-processed spans

    // Triple-quoted strings (process first, before regular strings)
    QRegularExpression tripleDouble("\"\"\"[\\s\\S]*?\"\"\"");
    QRegularExpression tripleSingle("'''[\\s\\S]*?'''");
    replaceWithStyle(tripleDouble, theme.string);
    replaceWithStyle(tripleSingle, theme.string);

    // Strings (use negative lookbehind for HTML attributes)
    QRegularExpression stringLiteral("(?<!style=)\"(?:\\\\\"|[^\"])+\"");
    QRegularExpression singleString("(?<!style=)'(?:\\\\'|[^'])+'");
    replaceWithStyle(stringLiteral, theme.string);
    replaceWithStyle(singleString, theme.string);

    // Keywords
    QString pythonKeywords = "\\b(and|as|assert|async|await|break|class|continue|def|del|elif|else|except|finally|for|from|global|if|import|in|is|lambda|nonlocal|not|or|pass|raise|return|try|while|with|yield|True|False|None)\\b";
    QRegularExpression keywordRegex(pythonKeywords);
    replaceWithStyle(keywordRegex, theme.keyword, true);

    // Built-in functions
    QString builtinFuncs = "\\b(print|len|range|str|int|float|list|dict|set|tuple|type|isinstance|hasattr|getattr|setattr|open|input|format|sorted|map|filter|zip|enumerate|any|all|min|max|sum|abs|round|bool|bytes|bytearray|memoryview|chr|ord|hex|oct|bin|dir|help|id|object|super|property|classmethod|staticmethod|iter|next|slice|reversed|exec|eval|compile|globals|locals|vars|__import__)\\b";
    QRegularExpression builtinRegex(builtinFuncs);
    replaceWithStyle(builtinRegex, theme.function);

    // Numbers
    QRegularExpression numberRegex("\\b(\\d+\\.?\\d*|0x[0-9a-fA-F]+|0b[01]+|0o[0-7]+)\\b");
    replaceWithStyle(numberRegex, theme.number);

    // Decorators
    QRegularExpression decoratorRegex("@[a-zA-Z_][a-zA-Z0-9_]*");
    replaceWithStyle(decoratorRegex, theme.preprocessor);

    // Function definitions - highlight function name
    QRegularExpression defRegex("\\bdef\\s+([a-zA-Z_][a-zA-Z0-9_]*)");
    QRegularExpressionMatchIterator defIt = defRegex.globalMatch(result);
    QList<ReplacementInfo> defReplacements;
    while (defIt.hasNext()) {
        QRegularExpressionMatch match = defIt.next();
        QString nameStyled = QString("<span style=\"color: %1;\">%2</span>").arg(theme.function, match.captured(1));
        defReplacements.append({match.capturedStart(1), match.capturedLength(1), nameStyled});
    }
    for (int i = defReplacements.size() - 1; i >= 0; --i) {
        result.replace(defReplacements[i].start, defReplacements[i].length, defReplacements[i].text);
    }

    // Class definitions - highlight class name
    QRegularExpression classRegex("\\bclass\\s+([a-zA-Z_][a-zA-Z0-9_]*)");
    QRegularExpressionMatchIterator classIt = classRegex.globalMatch(result);
    QList<ReplacementInfo> classReplacements;
    while (classIt.hasNext()) {
        QRegularExpressionMatch match = classIt.next();
        QString nameStyled = QString("<span style=\"color: %1;\">%2</span>").arg(theme.type, match.captured(1));
        classReplacements.append({match.capturedStart(1), match.capturedLength(1), nameStyled});
    }
    for (int i = classReplacements.size() - 1; i >= 0; --i) {
        result.replace(classReplacements[i].start, classReplacements[i].length, classReplacements[i].text);
    }

    // Comments - process LAST to avoid matching color codes in span tags
    // Use negative lookahead to exclude hex color codes like #008000, #a31515, etc.
    QRegularExpression commentRegex("#(?![0-9a-fA-F]{6})[^\\n]*");
    replaceWithStyle(commentRegex, theme.comment);

    return result;
}

QString MarkdownRenderer::highlightJs(const QString &code, const SyntaxTheme &theme)
{
    QString result = code;

    // Helper lambda for regex replacement
    auto replaceWithStyle = [&result, &theme](const QRegularExpression &regex, const QString &color, bool bold = false) {
        QRegularExpressionMatchIterator it = regex.globalMatch(result);
        QList<ReplacementInfo> replacements;

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString matched = match.captured(0);
            QString styled = bold
                ? QString("<span style=\"color: %1; font-weight: bold;\">%2</span>").arg(color, matched)
                : QString("<span style=\"color: %1;\">%2</span>").arg(color, matched);
            replacements.append({match.capturedStart(), match.capturedLength(), styled});
        }

        for (int i = replacements.size() - 1; i >= 0; --i) {
            result.replace(replacements[i].start, replacements[i].length, replacements[i].text);
        }
    };

    // Process order: strings first, then keywords, comments last
    // This prevents regex from matching content inside already-processed spans

    // Strings (template literals first, then regular - use negative lookbehind for HTML attributes)
    QRegularExpression templateLiteral("`(?:\\\\`|[^`])+`");
    QRegularExpression doubleString("(?<!style=)\"(?:\\\\\"|[^\"])+\"");
    QRegularExpression singleString("(?<!style=)'(?:\\\\'|[^'])+'");
    replaceWithStyle(templateLiteral, theme.string);
    replaceWithStyle(doubleString, theme.string);
    replaceWithStyle(singleString, theme.string);

    // Keywords
    QString jsKeywords = "\\b(async|await|break|case|catch|class|const|continue|debugger|default|delete|do|else|enum|export|extends|finally|for|function|if|implements|import|in|instanceof|interface|let|new|of|package|private|protected|public|return|static|super|switch|this|throw|try|typeof|var|void|while|with|yield|true|false|null|undefined|NaN|Infinity)\\b";
    QRegularExpression keywordRegex(jsKeywords);
    replaceWithStyle(keywordRegex, theme.keyword, true);

    // Numbers
    QRegularExpression numberRegex("\\b(\\d+\\.?\\d*|0x[0-9a-fA-F]+|0b[01]+|0o[0-7]+)\\b");
    replaceWithStyle(numberRegex, theme.number);

    // Arrow operator
    QRegularExpression arrowRegex("=>");
    replaceWithStyle(arrowRegex, theme.operator_);

    // Function declarations
    QRegularExpression funcDeclRegex("\\bfunction\\s+([a-zA-Z_][a-zA-Z0-9_]*)");
    QRegularExpressionMatchIterator funcIt = funcDeclRegex.globalMatch(result);
    QList<ReplacementInfo> funcReplacements;
    while (funcIt.hasNext()) {
        QRegularExpressionMatch match = funcIt.next();
        QString nameStyled = QString("<span style=\"color: %1;\">%2</span>").arg(theme.function, match.captured(1));
        funcReplacements.append({match.capturedStart(1), match.capturedLength(1), nameStyled});
    }
    for (int i = funcReplacements.size() - 1; i >= 0; --i) {
        result.replace(funcReplacements[i].start, funcReplacements[i].length, funcReplacements[i].text);
    }

    // Arrow function variable
    QRegularExpression arrowFuncRegex("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*(?==>)");
    QRegularExpressionMatchIterator arrowIt = arrowFuncRegex.globalMatch(result);
    QList<ReplacementInfo> arrowReplacements;
    while (arrowIt.hasNext()) {
        QRegularExpressionMatch match = arrowIt.next();
        QString nameStyled = QString("<span style=\"color: %1;\">%2</span>").arg(theme.function, match.captured(1));
        arrowReplacements.append({match.capturedStart(1), match.capturedLength(1), nameStyled});
    }
    for (int i = arrowReplacements.size() - 1; i >= 0; --i) {
        result.replace(arrowReplacements[i].start, arrowReplacements[i].length, arrowReplacements[i].text);
    }

    // Method/function calls
    QRegularExpression methodCallRegex("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*(?=\\()");
    QRegularExpressionMatchIterator methodIt = methodCallRegex.globalMatch(result);
    QList<ReplacementInfo> methodReplacements;
    while (methodIt.hasNext()) {
        QRegularExpressionMatch match = methodIt.next();
        QString nameStyled = QString("<span style=\"color: %1;\">%2</span>").arg(theme.function, match.captured(1));
        methodReplacements.append({match.capturedStart(1), match.capturedLength(1), nameStyled});
    }
    for (int i = methodReplacements.size() - 1; i >= 0; --i) {
        result.replace(methodReplacements[i].start, methodReplacements[i].length, methodReplacements[i].text);
    }

    // Comments - process LAST to avoid matching color codes in span tags
    QRegularExpression multiLineComment("/\\*[^*]*\\*+(?:[^/*][^*]*\\*+)*/");
    QRegularExpression singleLineComment("//[^\n]*");
    replaceWithStyle(multiLineComment, theme.comment);
    replaceWithStyle(singleLineComment, theme.comment);

    return result;
}

QString MarkdownRenderer::highlightJson(const QString &code, const SyntaxTheme &theme)
{
    QString result = code;

    // Helper lambda for regex replacement
    auto replaceWithStyle = [&result](const QRegularExpression &regex, const QString &color, bool bold = false) {
        QRegularExpressionMatchIterator it = regex.globalMatch(result);
        QList<ReplacementInfo> replacements;

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString matched = match.captured(0);
            QString styled = bold
                ? QString("<span style=\"color: %1; font-weight: bold;\">%2</span>").arg(color, matched)
                : QString("<span style=\"color: %1;\">%2</span>").arg(color, matched);
            replacements.append({match.capturedStart(), match.capturedLength(), styled});
        }

        for (int i = replacements.size() - 1; i >= 0; --i) {
            result.replace(replacements[i].start, replacements[i].length, replacements[i].text);
        }
    };

    // Strings (keys and values - use negative lookbehind for HTML attributes)
    QRegularExpression stringRegex("(?<!style=)\"(?:\\\\\"|[^\"])+\"");
    replaceWithStyle(stringRegex, theme.string);

    // Numbers
    QRegularExpression numberRegex("\\b(-?\\d+\\.?\\d*(?:[eE][+-]?\\d+)?)\\b");
    replaceWithStyle(numberRegex, theme.number);

    // Booleans and null
    QRegularExpression boolRegex("\\b(true|false|null)\\b");
    replaceWithStyle(boolRegex, theme.keyword, true);

    // Brackets and braces
    QRegularExpression braceRegex("[{}\\[\\]]");
    replaceWithStyle(braceRegex, theme.operator_);

    return result;
}

QString MarkdownRenderer::highlightBash(const QString &code, const SyntaxTheme &theme)
{
    QString result = code;

    // Helper lambda for regex replacement
    auto replaceWithStyle = [&result, &theme](const QRegularExpression &regex, const QString &color, bool bold = false) {
        QRegularExpressionMatchIterator it = regex.globalMatch(result);
        QList<ReplacementInfo> replacements;

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString matched = match.captured(0);
            QString styled = bold
                ? QString("<span style=\"color: %1; font-weight: bold;\">%2</span>").arg(color, matched)
                : QString("<span style=\"color: %1;\">%2</span>").arg(color, matched);
            replacements.append({match.capturedStart(), match.capturedLength(), styled});
        }

        for (int i = replacements.size() - 1; i >= 0; --i) {
            result.replace(replacements[i].start, replacements[i].length, replacements[i].text);
        }
    };

    // Process order: strings/variables first, then keywords, comments last
    // This prevents regex from matching content inside already-processed spans

    // Strings (use negative lookbehind for HTML attributes)
    QRegularExpression doubleString("(?<!style=)\"(?:\\\\\"|[^\"])+\"");
    QRegularExpression singleString("(?<!style=)'(?:\\\\'|[^'])+'");
    replaceWithStyle(doubleString, theme.string);
    replaceWithStyle(singleString, theme.string);

    // Keywords
    QString bashKeywords = "\\b(if|then|else|elif|fi|for|while|do|done|case|esac|in|function|return|exit|break|continue|local|declare|readonly|export|unset|shift|source|eval|exec|trap|true|false)\\b";
    QRegularExpression keywordRegex(bashKeywords);
    replaceWithStyle(keywordRegex, theme.keyword, true);

    // Variables ${var} and $var
    QRegularExpression varBraceRegex("\\$\\{[^}]+\\}");
    QRegularExpression varRegex("\\$[a-zA-Z_][a-zA-Z0-9_]*");
    replaceWithStyle(varBraceRegex, theme.variable);
    replaceWithStyle(varRegex, theme.variable);

    // Numbers
    QRegularExpression numberRegex("\\b(\\d+)\\b");
    replaceWithStyle(numberRegex, theme.number);

    // Common commands
    QString commonCommands = "\\b(echo|printf|read|cd|pwd|ls|mkdir|rmdir|rm|cp|mv|cat|grep|sed|awk|find|sort|uniq|head|tail|wc|tr|cut|split|diff|chmod|chown|chgrp|ln|touch|file|which|whereis|type|man|info|help|sudo|su|apt|apt-get|yum|dnf|pacman|pip|npm|git|docker|kubectl|systemctl|service|journalctl|ps|kill|killall|top|htop|free|df|du|mount|umount|fdisk|mkfs|fsck|tar|gzip|gunzip|zip|unzip|rsync|scp|ssh|curl|wget|ping|netstat|ss|lsof|ifconfig|ip|route|iptables|ufw|firewall-cmd)\\b";
    QRegularExpression commandRegex(commonCommands);
    replaceWithStyle(commandRegex, theme.function);

    // Comments - process LAST to avoid matching color codes in span tags
    // Use negative lookahead to exclude hex color codes like #008000, #a31515, etc.
    QRegularExpression commentRegex("#(?![0-9a-fA-F]{6})[^\\n]*");
    replaceWithStyle(commentRegex, theme.comment);

    return result;
}

QString MarkdownRenderer::highlightGeneric(const QString &code, const SyntaxTheme &theme)
{
    QString result = code;

    // Helper lambda for regex replacement
    auto replaceWithStyle = [&result, &theme](const QRegularExpression &regex, const QString &color, bool bold = false) {
        QRegularExpressionMatchIterator it = regex.globalMatch(result);
        QList<ReplacementInfo> replacements;

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString matched = match.captured(0);
            QString styled = bold
                ? QString("<span style=\"color: %1; font-weight: bold;\">%2</span>").arg(color, matched)
                : QString("<span style=\"color: %1;\">%2</span>").arg(color, matched);
            replacements.append({match.capturedStart(), match.capturedLength(), styled});
        }

        for (int i = replacements.size() - 1; i >= 0; --i) {
            result.replace(replacements[i].start, replacements[i].length, replacements[i].text);
        }
    };

    // Strings (use negative lookbehind for HTML attributes)
    QRegularExpression doubleString("(?<!style=)\"(?:\\\\\"|[^\"])+\"");
    QRegularExpression singleString("(?<!style=)'(?:\\\\'|[^'])+'");
    replaceWithStyle(doubleString, theme.string);
    replaceWithStyle(singleString, theme.string);

    // Numbers
    QRegularExpression numberRegex("\\b(\\d+\\.?\\d*)\\b");
    replaceWithStyle(numberRegex, theme.number);

    // Common keywords across many languages
    QString commonKeywords = "\\b(if|else|for|while|do|switch|case|break|continue|return|function|class|struct|import|export|from|const|let|var|true|false|null|undefined|void|int|string|bool|float|double|char|long|short|byte|public|private|protected|static|final|abstract|interface|extends|implements|new|this|super|try|catch|finally|throw|throws|async|await|yield)\\b";
    QRegularExpression keywordRegex(commonKeywords);
    replaceWithStyle(keywordRegex, theme.keyword, true);

    return result;
}