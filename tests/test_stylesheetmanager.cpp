#include <QtTest>
#include <QApplication>
#include "stylesheetmanager.h"

class TestStyleSheetManager : public QObject
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

    void testSingleton()
    {
        StyleSheetManager* a = StyleSheetManager::instance();
        StyleSheetManager* b = StyleSheetManager::instance();
        QVERIFY(a != nullptr);
        QCOMPARE(a, b);
    }

    void testSetTheme()
    {
        StyleSheetManager::instance()->setTheme(StyleSheetManager::LightTheme);
        QCOMPARE(StyleSheetManager::instance()->currentTheme(), StyleSheetManager::LightTheme);

        StyleSheetManager::instance()->setTheme(StyleSheetManager::DarkTheme);
        QCOMPARE(StyleSheetManager::instance()->currentTheme(), StyleSheetManager::DarkTheme);
    }

    void testLightDarkStyleSheetsDiffer()
    {
        QString light = StyleSheetManager::lightStyleSheet();
        QString dark = StyleSheetManager::darkStyleSheet();
        QVERIFY(!light.isEmpty());
        QVERIFY(!dark.isEmpty());
        QVERIFY(light != dark);
    }

    void testStyleSheetContainsSelectors()
    {
        QString qss = StyleSheetManager::lightStyleSheet();
        QVERIFY(qss.contains("QListWidget"));
        QVERIFY(qss.contains("QPushButton"));
        QVERIFY(qss.contains("QTextBrowser"));
        QVERIFY(qss.contains("QLineEdit"));
        QVERIFY(qss.contains("QComboBox"));
    }

    void testThemeChangedSignal()
    {
        bool emitted = false;
        connect(StyleSheetManager::instance(), &StyleSheetManager::themeChanged,
                [&emitted](int) { emitted = true; });

        StyleSheetManager::instance()->setTheme(StyleSheetManager::DarkTheme);
        QVERIFY(emitted);
    }
};

QTEST_MAIN(TestStyleSheetManager)
#include "test_stylesheetmanager.moc"
