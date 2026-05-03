#include <QApplication>
#include <QLocale>
#include <QSettings>
#include <QDebug>
#include "mainwindow.h"
#include "translationmanager.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <cstring>

// 在 Windows 上创建调试控制台窗口
void attachDebugConsole()
{
    // 检查是否已经有控制台（从命令行启动时）
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        // 已有控制台，重定向输出
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        setvbuf(stdout, nullptr, _IONBF, 0);
        setvbuf(stderr, nullptr, _IONBF, 0);
    } else {
        // 没有控制台，创建一个新的
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        setvbuf(stdout, nullptr, _IONBF, 0);
        setvbuf(stderr, nullptr, _IONBF, 0);

        // 设置控制台标题
        SetConsoleTitleW(L"LocalAIAssistant - Debug Console");

        qDebug() << "Debug console created. Close this window to hide logs.";
    }
}
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // 检查命令行参数，如果有 --debug 则显示控制台
    bool showDebugConsole = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0) {
            showDebugConsole = true;
            break;
        }
    }

    // 也可以通过环境变量启用
    if (qEnvironmentVariableIsSet("LOCALAI_DEBUG")) {
        showDebugConsole = true;
    }

    if (showDebugConsole) {
        attachDebugConsole();
        qDebug() << "=== LocalAIAssistant Debug Mode ===";
        qDebug() << "Application starting...";
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
