#include <QApplication>
#include <QtTest>

#include "apptheme.h"
#include "stylesheetmanager.h"

class TestAppTheme : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        if (!QApplication::instance()) new QApplication(argc, argv);
        StyleSheetManager::instance()->setTheme(StyleSheetManager::LightTheme);
    }

    void testLightFactory_AllTokensSet() {
        const AppTheme& t = AppTheme::light();
        QVERIFY(t.windowBg.isValid());
        QVERIFY(t.surfaceBg.isValid());
        QVERIFY(t.elevatedBg.isValid());
        QVERIFY(t.textPrimary.isValid());
        QVERIFY(t.textSecondary.isValid());
        QVERIFY(t.textDisabled.isValid());
        QVERIFY(t.textOnAccent.isValid());
        QVERIFY(t.accent.isValid());
        QVERIFY(t.accentHover.isValid());
        QVERIFY(t.accentPressed.isValid());
        QVERIFY(t.warning.isValid());
        QVERIFY(t.danger.isValid());
        QVERIFY(t.dangerHover.isValid());
        QVERIFY(t.success.isValid());
        QVERIFY(t.border.isValid());
        QVERIFY(t.borderFocus.isValid());
        QVERIFY(t.hoverBg.isValid());
        QVERIFY(t.selectedBg.isValid());
        QVERIFY(t.selectedText.isValid());
        QVERIFY(t.codeBg.isValid());
        QVERIFY(t.inlineCodeBg.isValid());
        QVERIFY(t.quoteBg.isValid());
        QVERIFY(t.quoteBorder.isValid());
        QVERIFY(t.quoteText.isValid());
        QVERIFY(t.tableBorder.isValid());
        QVERIFY(t.tableHeaderBg.isValid());
        QVERIFY(t.syntaxKeyword.isValid());
        QVERIFY(t.syntaxString.isValid());
        QVERIFY(t.syntaxComment.isValid());
        QVERIFY(t.syntaxNumber.isValid());
        QVERIFY(t.syntaxFunction.isValid());
        QVERIFY(t.syntaxType.isValid());
        QVERIFY(t.syntaxOperator.isValid());
        QVERIFY(t.syntaxPreprocessor.isValid());
        QVERIFY(t.syntaxVariable.isValid());
        QVERIFY(t.disabledButtonBg.isValid());
        QVERIFY(t.girlfriendAccent.isValid());
        QVERIFY(t.girlfriendAccentHover.isValid());
        QVERIFY(t.girlfriendAccentPressed.isValid());
    }

    void testDarkFactory_AllTokensSet() {
        const AppTheme& t = AppTheme::dark();
        QVERIFY(t.windowBg.isValid());
        QVERIFY(t.surfaceBg.isValid());
        QVERIFY(t.textPrimary.isValid());
        QVERIFY(t.accent.isValid());
        QVERIFY(t.syntaxKeyword.isValid());
    }

    void testDarkVsLight_Differ() {
        const AppTheme& light = AppTheme::light();
        const AppTheme& dark = AppTheme::dark();
        QVERIFY(light.windowBg != dark.windowBg);
        QVERIFY(light.textPrimary != dark.textPrimary);
        QVERIFY(light.accent != dark.accent);
        QVERIFY(light.border != dark.border);
    }

    void testGirlfriendAccent_SameBothThemes() {
        const AppTheme& light = AppTheme::light();
        const AppTheme& dark = AppTheme::dark();
        QCOMPARE(light.girlfriendAccent, dark.girlfriendAccent);
        QCOMPARE(light.girlfriendAccent.name(), QStringLiteral("#e91e63"));
    }

    void testCurrent_Cached() {
        const AppTheme& a = AppTheme::current();
        const AppTheme& b = AppTheme::current();
        QCOMPARE(&a, &b);
    }

    void testCurrent_ChangesWithTheme() {
        StyleSheetManager::instance()->setTheme(StyleSheetManager::LightTheme);
        QColor lightBg = AppTheme::current().windowBg;

        StyleSheetManager::instance()->setTheme(StyleSheetManager::DarkTheme);
        QColor darkBg = AppTheme::current().windowBg;

        QVERIFY(lightBg != darkBg);
        QCOMPARE(lightBg, AppTheme::light().windowBg);
        QCOMPARE(darkBg, AppTheme::dark().windowBg);
    }
};

QTEST_MAIN(TestAppTheme)
#include "test_apptheme.moc"
