#include <QApplication>
#include <QLocale>
#include <QSettings>
#include "mainwindow.h"
#include "translationmanager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("LocalAIAssistant");
    QApplication::setApplicationVersion("1.1.0");

    QSettings settings("LocalAIAssistant", "Settings");
    QString language = settings.value("language", "system").toString();

    QString locale;
    if (language == "system") {
        locale = QLocale::system().name();
        if (locale.startsWith("zh")) {
            locale = "zh_CN";
        } else {
            locale = "en";
        }
    } else {
        locale = language;
    }

    TranslationManager::instance()->loadTranslation(locale);

    MainWindow window;
    window.resize(900, 600);
    window.show();

    return app.exec();
}
