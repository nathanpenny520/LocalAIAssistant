#pragma once

#ifndef AGENTLOOP_H
#define AGENTLOOP_H

#include <QObject>
#include <QString>
#include <QVector>

#include "commandexecutor.h"
#include "operationplan.h"
#include "safetychecker.h"

class AgentLoop : public QObject {
    Q_OBJECT

public:
    enum State { Idle, Running, AwaitingUserConfirm, Completed, Stopped, MaxIterations };

    static AgentLoop* instance();

    void start(const QString& aiResponse, const QString& sessionId);
    void continueWithResponse(const QString& response);
    void confirmPlan();
    void cancelPlan();
    void stop();

    State state() const;
    int iterationCount() const;
    void setMaxIterations(int max);

signals:
    void executionResultReady(const QString& feedbackMessage, const QString& sessionId);
    void loopFinished(const QString& summary, const QString& sessionId);
    void planRequiresConfirmation(const OperationPlan& plan,
                                  const QVector<PathViolation>& violations);
    void stateChanged(AgentLoop::State newState);

private:
    explicit AgentLoop(QObject* parent = nullptr);
    static AgentLoop* s_instance;

    void processNextIteration(const QString& response);
    QString buildResultFeedback(const QVector<CommandResult>& results) const;
    bool isTaskComplete(const QString& response) const;
    void executeAndContinue(const OperationPlan& plan);

    State m_state = Idle;
    int m_iterationCount = 0;
    int m_maxIterations = 10;
    OperationPlan m_pendingPlan;
    QString m_sessionId;
};

#endif  // AGENTLOOP_H
