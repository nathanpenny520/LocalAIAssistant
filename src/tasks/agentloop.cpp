#include "agentloop.h"

#include <QRegularExpression>

#include "../core/sessionmanager.h"
#include "taskengine.h"

AgentLoop* AgentLoop::s_instance = nullptr;

AgentLoop* AgentLoop::instance() {
    if (!s_instance) s_instance = new AgentLoop();
    return s_instance;
}

AgentLoop::AgentLoop(QObject* parent) : QObject(parent) {
}

AgentLoop::State AgentLoop::state() const {
    return m_state;
}

int AgentLoop::iterationCount() const {
    return m_iterationCount;
}

void AgentLoop::setMaxIterations(int max) {
    m_maxIterations = max;
}

void AgentLoop::start(const QString& aiResponse, const QString& sessionId) {
    m_state = Running;
    m_iterationCount = 0;
    m_sessionId = sessionId;
    emit stateChanged(m_state);
    processNextIteration(aiResponse);
}

void AgentLoop::continueWithResponse(const QString& response) {
    if (m_state != Running) return;
    processNextIteration(response);
}

void AgentLoop::processNextIteration(const QString& response) {
    // Check for task completion first
    if (isTaskComplete(response)) {
        SessionManager::instance()->addMessageToSession(m_sessionId, "assistant", response);
        m_state = Completed;
        emit stateChanged(m_state);
        emit loopFinished(tr("Task completed"), m_sessionId);
        return;
    }

    TaskEngine* engine = TaskEngine::instance();
    OperationPlan plan = engine->parsePlanFromAIResponse(response);

    if (plan.isEmpty()) {
        // No TASK_PLAN found — treat as normal chat message and end loop
        SessionManager::instance()->addMessageToSession(m_sessionId, "assistant", response);
        m_state = Completed;
        emit stateChanged(m_state);
        emit loopFinished(tr("Task finished"), m_sessionId);
        return;
    }

    SafetyChecker::Result safetyResult = engine->validatePlan(plan);

    if (safetyResult == SafetyChecker::Blocked) {
        QString errMsg = tr("Operation blocked: %1")
                                 .arg(engine->safetyChecker().lastBlockReason());
        SessionManager::instance()->addMessageToSession(m_sessionId, "assistant", errMsg);
        m_state = Stopped;
        emit stateChanged(m_state);
        emit loopFinished(errMsg, m_sessionId);
        return;
    }

    if (safetyResult == SafetyChecker::NeedsConfirmation) {
        m_pendingPlan = plan;
        m_state = AwaitingUserConfirm;
        emit stateChanged(m_state);
        QVector<PathViolation> violations = engine->safetyChecker().lastPathViolations();
        emit planRequiresConfirmation(plan, violations);
        return;
    }

    // Approved — execute and continue
    executeAndContinue(plan);
}

void AgentLoop::confirmPlan() {
    if (m_state != AwaitingUserConfirm) return;
    m_state = Running;
    m_iterationCount++;
    emit stateChanged(m_state);
    executeAndContinue(m_pendingPlan);
}

void AgentLoop::cancelPlan() {
    if (m_state != AwaitingUserConfirm) return;
    m_pendingPlan = OperationPlan();
    m_state = Stopped;
    emit stateChanged(m_state);
    emit loopFinished(tr("Cancelled"), m_sessionId);
}

void AgentLoop::stop() {
    TaskEngine::instance()->executor()->cancel();
    m_state = Stopped;
    emit stateChanged(m_state);
    emit loopFinished(tr("Stopped"), m_sessionId);
}

void AgentLoop::executeAndContinue(const OperationPlan& plan) {
    // Check iteration limit
    if (m_iterationCount >= m_maxIterations) {
        m_state = MaxIterations;
        emit stateChanged(m_state);
        emit loopFinished(tr("Maximum iterations reached (%1)").arg(m_maxIterations), m_sessionId);
        return;
    }

    TaskEngine* engine = TaskEngine::instance();
    QVector<CommandResult> results = engine->executePlan(plan);

    // Build structured feedback message
    QString feedback = buildResultFeedback(results);

    // Save the original AI response (with TASK_PLAN) as assistant message
    SessionManager::instance()->addMessageToSession(m_sessionId, "assistant",
                                                     plan.generateSummary());

    // Add feedback as a user message to continue the conversation
    SessionManager::instance()->addMessageToSession(m_sessionId, "user", feedback);

    emit executionResultReady(feedback, m_sessionId);
}

QString AgentLoop::buildResultFeedback(const QVector<CommandResult>& results) const {
    QString feedback;
    feedback += QStringLiteral("[ITERATION_FEEDBACK]\n");
    feedback += tr("Previous iteration results:\n");

    int successCount = 0;
    int failCount = 0;
    for (int i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        if (r.success) {
            successCount++;
            feedback += tr("  Operation %1: SUCCESS").arg(i + 1);
            if (!r.stdoutOutput.trimmed().isEmpty()) {
                QString output = r.stdoutOutput.trimmed();
                if (output.length() > 80) output = output.left(77) + QStringLiteral("...");
                feedback += QStringLiteral(" — ") + output;
            }
        } else {
            failCount++;
            feedback += tr("  Operation %1: FAILED").arg(i + 1);
            if (!r.errorMessage.isEmpty())
                feedback += QStringLiteral(" — ") + r.errorMessage;
        }
        feedback += QLatin1Char('\n');
    }

    feedback += tr("  Total: %1 succeeded, %2 failed\n").arg(successCount).arg(failCount);
    feedback += QStringLiteral("\n");
    feedback += tr("You MUST respond now. If all operations succeeded, output [TASK_COMPLETE] with "
                   "a user-facing summary of what was accomplished. If more work is needed, output "
                   "a new [TASK_PLAN]. Never remain silent — the conversation will stall.");

    return feedback;
}

bool AgentLoop::isTaskComplete(const QString& response) const {
    static QRegularExpression re(QStringLiteral("\\[TASK_COMPLETE\\]|\\[TASK_FINISHED\\]"),
                                 QRegularExpression::CaseInsensitiveOption);
    return re.match(response).hasMatch();
}
