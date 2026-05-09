#pragma once

#ifndef CLI_APPLICATION_H
#define CLI_APPLICATION_H

#include <QCommandLineParser>
#include <QObject>

#include "commandexecutor.h"
#include "operationplan.h"
#include "safetychecker.h"

class AgentLoop;
class NetworkManager;
class FileManager;
class TaskEngine;

class CLIApplication : public QObject {
    Q_OBJECT

public:
    explicit CLIApplication(QObject* parent = nullptr);
    int run(int argc, char* argv[]);

private:
    void printUsage();
    int runInteractiveMode(QCoreApplication& app, const QCommandLineParser& parser);
    void readInput();
    void handleCommand(const QString& command);
    void showHelp();
    void listSessions();
    void showConfig();
    int runSingleQuery(QCoreApplication& app, const QString& query,
                       const QCommandLineParser& parser);
    int handleSessionsCommand(const QCommandLineParser& parser);
    void showSessionContent(const QString& sessionId);
    int handleConfigCommand(const QCommandLineParser& parser);
    void quit();

    // Task execution
    bool extractAndHandleTaskPlan(const QString& response);
    void showPlanPreview(const OperationPlan& plan);
    void executePlanNow(const OperationPlan& plan);
    void executeConfirmedPlan();
    QString formatCommandResult(int index, const CommandResult& result) const;

    // Path violation toggling (interactive mode)
    void renderPathViolationToggles() const;
    void clearPendingPlan();

    // File commands
    void handleFileCommand(const QString& command);
    void listFiles();

    // Search
    void searchMessages(const QString& keyword);

private slots:
    void onResponseReceived(const QString& response);
    void onErrorOccurred(const QString& error);
    void onStreamChunkReceived(const QString& chunk);
    void onStreamFinished(const QString& fullContent);

    // AgentLoop slots
    void onAgentLoopResultReady(const QString& feedbackMessage, const QString& sessionId);
    void onAgentLoopPlanConfirm(const OperationPlan& plan,
                                const QVector<PathViolation>& violations);
    void onAgentLoopFinished(const QString& summary, const QString& sessionId);

private:
    NetworkManager* m_networkManager;
    FileManager* m_fileManager;
    TaskEngine* m_taskEngine;
    bool m_running;
    bool m_interactiveMode;
    bool m_isStreaming;
    QString m_streamingContent;

    // Pending task plan (awaiting confirmation)
    OperationPlan m_pendingPlan;
    bool m_hasPendingPlan = false;
    bool m_autoConfirm = false;

    // Path violation toggle state (interactive mode)
    QVector<PathViolation> m_pendingViolations;
    QVector<int> m_pendingViolationResponses; // 0=Deny, 1=Allow Once, 2=Always Allow
};

#endif
