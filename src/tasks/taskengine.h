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

    // 从 AI 回复中解析操作计划 JSON
    // 期望 AI 返回格式: [TASK_PLAN] {...json...} [/TASK_PLAN]
    OperationPlan parsePlanFromAIResponse(const QString& aiResponse) const;
    OperationPlan parsePlanFromJson(const QJsonObject& json) const;

    // 获取用于让 AI 生成任务计划的 prompt 模板
    QString taskPromptTemplate() const;

    // 安全校验
    SafetyChecker::Result validatePlan(const OperationPlan& plan);

    // 执行（确认后调用）
    QVector<CommandResult> executePlan(const OperationPlan& plan);

    // 撤销
    bool canUndo() const;
    QVector<CommandResult> undoLast();

    // 获取执行器和安全检查器
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
