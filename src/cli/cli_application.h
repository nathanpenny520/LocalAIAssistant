#ifndef CLI_APPLICATION_H
#define CLI_APPLICATION_H

#include <QObject>
#include <QCommandLineParser>

class NetworkManager;

class CLIApplication : public QObject
{
    Q_OBJECT

public:
    explicit CLIApplication(QObject *parent = nullptr);
    int run(int argc, char *argv[]);

private:
    void printUsage();
    int runInteractiveMode(QCoreApplication &app, const QCommandLineParser &parser);
    void readInput();
    void handleCommand(const QString &command);
    void showHelp();
    void listSessions();
    void showConfig();
    int runSingleQuery(QCoreApplication &app, const QString &query, const QCommandLineParser &parser);
    int handleSessionsCommand(const QCommandLineParser &parser);
    void showSessionContent(const QString &sessionId);
    int handleConfigCommand(const QCommandLineParser &parser);
    void quit();

private slots:
    void onResponseReceived(const QString &response);
    void onErrorOccurred(const QString &error);
    void onStreamChunkReceived(const QString &chunk);
    void onStreamFinished(const QString &fullContent);

private:
    NetworkManager *m_networkManager;
    bool m_running;
    bool m_interactiveMode;
    bool m_isStreaming;
    QString m_streamingContent;
};

#endif
