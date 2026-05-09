/**
 * @file taskengine.h
 * @brief AI response parsing, TASK_PLAN extraction, and multi-step plan execution orchestration.
 */
#pragma once

#ifndef TASKENGINE_H
#define TASKENGINE_H

#include <QJsonObject>
#include <QObject>

#include "commandexecutor.h"
#include "operationplan.h"
#include "operationundo.h"
#include "safetychecker.h"

class TaskEngine : public QObject {
    Q_OBJECT

public:
    static TaskEngine* instance();

    // Task tag constants — single source of truth for all agent loop tags
    static const QString kTagTaskPlan;
    static const QString kTagTaskPlanClose;
    static const QString kTagTaskComplete;
    static const QString kTagTaskFinished;

    // Parse operation plan JSON from AI response
    // Expected AI response format: [TASK_PLAN] {...json...} [/TASK_PLAN]
    OperationPlan parsePlanFromAIResponse(const QString& aiResponse) const;
    OperationPlan parsePlanFromJson(const QJsonObject& json) const;

    // Get the prompt template for AI task plan generation
    QString taskPromptTemplate() const;

    // Run safety validation on the plan
    SafetyChecker::Result validatePlan(const OperationPlan& plan);

    // Execute (call after user confirmation)
    QVector<CommandResult> executePlan(const OperationPlan& plan);

    // Undo last plan
    bool canUndo() const;
    QVector<CommandResult> undoLast();

    // Access the executor and safety checker
    CommandExecutor* executor();
    SafetyChecker& safetyChecker();

signals:
    void planReady(const OperationPlan& plan);
    void executionFinished(const QVector<CommandResult>& results);
    void undoFinished(const QVector<CommandResult>& results);

private:
    explicit TaskEngine(QObject* parent = nullptr);
    static TaskEngine* s_instance;

    SafetyChecker m_safetyChecker;
    CommandExecutor m_executor;
    OperationUndo m_undo;
};

#endif  // TASKENGINE_H
