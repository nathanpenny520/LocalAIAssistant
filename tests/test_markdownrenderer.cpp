#include <QtTest>
#include <QApplication>
#include "markdownrenderer.h"
#include "stylesheetmanager.h"

class TestMarkdownRenderer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        static int argc = 0;
        static char *argv[] = {nullptr};
        if (!QApplication::instance())
            new QApplication(argc, argv);
        StyleSheetManager::instance()->setTheme(StyleSheetManager::LightTheme);
    }

    void testToHtml_PlainText()
    {
        QString html = MarkdownRenderer::toHtml("Hello world");
        QVERIFY(html.contains("<p"));
        QVERIFY(html.contains("Hello world"));
    }

    void testToHtml_Headers()
    {
        QString html = MarkdownRenderer::toHtml("# Title");
        QVERIFY(html.contains("<h1"));
        QVERIFY(html.contains("Title"));
        QVERIFY(html.contains("font-size: 24px"));
    }

    void testToHtml_Bold()
    {
        QString html = MarkdownRenderer::toHtml("this is **bold** text");
        QVERIFY(html.contains("<b style="));
        QVERIFY(html.contains("bold"));
    }

    void testToHtml_CodeBlock()
    {
        QString html = MarkdownRenderer::toHtml("```\ncode\n```");
        QVERIFY(html.contains("<pre"));
        QVERIFY(html.contains("<code"));
        QVERIFY(html.contains("code"));
    }

    void testToHtml_CodeBlock_WithLanguage()
    {
        QString html = MarkdownRenderer::toHtml("```cpp\nint x = 1;\n```");
        QVERIFY(html.contains("<pre"));
        QVERIFY(html.contains("int"));
    }

    void testToHtml_InlineCode()
    {
        QString html = MarkdownRenderer::toHtml("use `printf` function");
        QVERIFY(html.contains("<code style="));
        QVERIFY(html.contains("printf"));
    }

    void testToHtml_Blockquote()
    {
        QString html = MarkdownRenderer::toHtml("> quoted text");
        QVERIFY(html.contains("<blockquote"));
        QVERIFY(html.contains("quoted text"));
    }

    void testToHtml_Table()
    {
        QString html = MarkdownRenderer::toHtml(
            "| A | B |\n"
            "|---|---|\n"
            "| 1 | 2 |\n");
        QVERIFY(html.contains("<table"));
        QVERIFY(html.contains("<thead"));
        QVERIFY(html.contains("<tbody"));
        QVERIFY(html.contains("A"));
        QVERIFY(html.contains("B"));
        QVERIFY(html.contains("1"));
        QVERIFY(html.contains("2"));
    }

    void testToHtml_UnorderedList()
    {
        QString html = MarkdownRenderer::toHtml("- item 1\n- item 2");
        QVERIFY(html.contains("<ul"));
        QVERIFY(html.contains("<li"));
        QVERIFY(html.contains("item 1"));
    }

    void testToHtml_ThemeColors()
    {
        QString html = MarkdownRenderer::toHtml("Hello world");
        // Should contain theme's textPrimary color
        QVERIFY(html.contains("#333333"));
    }

    void testToHtml_EmptyInput()
    {
        QString html = MarkdownRenderer::toHtml("");
        QCOMPARE(html, QString(""));
    }

    void testToHtml_BoldLabel()
    {
        QString html = MarkdownRenderer::toHtml("**AI:** hello");
        // Bold label uses accent color
        QVERIFY(html.contains("#007aff"));
        QVERIFY(html.contains("<b"));
    }

    void testToHtml_EscapedHtml()
    {
        // HTML in code blocks is escaped (user-provided content)
        QString html = MarkdownRenderer::toHtml("```\nuse <script> tag\n```");
        QVERIFY(html.contains("&lt;script&gt;"));
        QVERIFY(!html.contains("<script>"));
    }

    void testToHtml_HorizontalRule()
    {
        QString html = MarkdownRenderer::toHtml("---");
        QVERIFY(html.contains("&nbsp;"));
    }
};

QTEST_MAIN(TestMarkdownRenderer)
#include "test_markdownrenderer.moc"
