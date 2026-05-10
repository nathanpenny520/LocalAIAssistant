#include <QApplication>
#include <QLocale>
#include <QSettings>

#include "mainwindow.h"
#include "translationmanager.h"

#ifdef Q_OS_WIN
#include <cstring>

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <windows.h>

// Create a debug console window on Windows
void attachDebugConsole() {
    // Check if a console already exists (e.g. launched from command line)
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        // Console exists — redirect output to it
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        setvbuf(stdout, nullptr, _IONBF, 0);
        setvbuf(stderr, nullptr, _IONBF, 0);
    } else {
        // No console — create a new one
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        setvbuf(stdout, nullptr, _IONBF, 0);
        setvbuf(stderr, nullptr, _IONBF, 0);

        // Set console title
        SetConsoleTitleW(L"LocalAIAssistant - Debug Console");

    }
}
#endif

int main(int argc, char* argv[]) {
#ifdef Q_OS_WIN
    // Check for --debug or -d command-line flags
    bool showDebugConsole = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0) {
            showDebugConsole = true;
            break;
        }
    }

    // Can also be enabled via environment variable
    if (qEnvironmentVariableIsSet("LOCALAI_DEBUG")) {
        showDebugConsole = true;
    }

    if (showDebugConsole) {
        attachDebugConsole();
    }
#endif

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
