#include "agentloop.h"

#include <QRegularExpression>
#include <QSettings>

#include "../core/sessionmanager.h"
#include "taskengine.h"

AgentLoop* AgentLoop::s_instance = nullptr;

AgentLoop* AgentLoop::instance() {
    if (!s_instance) s_instance = new AgentLoop();
    return s_instance;
}

AgentLoop::AgentLoop(QObject* parent) : QObject(parent) {
    QSettings settings("LocalAIAssistant", "Settings");
    m_preserveLoopMessages = settings.value("preserveAgentLoopMessages", true).toBool();
}

AgentLoop::State AgentLoop::state() const {
    return m_state;
}

int AgentLoop::iterationCount() const {
    return m_iterationCount;
}

QString AgentLoop::lastFeedback() const {
    return m_lastFeedback;
}

void AgentLoop::setMaxIterations(int max) {
    m_maxIterations = max;
}

void AgentLoop::setPreserveLoopMessages(bool preserve) {
    m_preserveLoopMessages = preserve;
    QSettings settings("LocalAIAssistant", "Settings");
    settings.setValue("preserveAgentLoopMessages", preserve);
}

bool AgentLoop::preserveLoopMessages() const {
    return m_preserveLoopMessages;
}

void AgentLoop::start(const QString& aiResponse, const QString& sessionId) {
    m_state = Running;
    m_iterationCount = 0;
    m_sessionId = sessionId;
    emit stateChanged(m_state);
    processNextIteration(aiResponse);
}

void AgentLoop::continueWithResponse(const QString& response) {
    qDebug() << "[DEBUG] AgentLoop::continueWithResponse called, state:" << m_state;
    if (m_state != Running) {
        qDebug() << "[DEBUG] AgentLoop::continueWithResponse: state is not Running, returning without processing";
        return;
    }
    processNextIteration(response);
}

void AgentLoop::processNextIteration(const QString& response) {
    qDebug() << "[DEBUG] AgentLoop::processNextIteration, response length:" << response.size()
             << "preview:" << response.left(200);
    // Check for TASK_PLAN first — the AI may mix TASK_COMPLETE (closing prior
    // context) and TASK_PLAN (new work) in a single response. Processing the plan
    // takes precedence; TASK_COMPLETE alone ends the loop.
    TaskEngine* engine = TaskEngine::instance();
    OperationPlan plan = engine->parsePlanFromAIResponse(response);

    if (!plan.isEmpty()) {
        qDebug() << "[DEBUG] AgentLoop::processNextIteration: found TASK_PLAN";
        SafetyChecker::Result safetyResult = engine->validatePlan(plan);
        qDebug() << "[DEBUG] AgentLoop: safety result:" << safetyResult;

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
        return;
    }

    // No TASK_PLAN — check if this is a task completion
    if (isTaskComplete(response)) {
        qDebug() << "[DEBUG] AgentLoop::processNextIteration: detected TASK_COMPLETE/FINISHED";
        SessionManager::instance()->addMessageToSession(m_sessionId, "assistant", response);
        m_state = Completed;
        emit stateChanged(m_state);
        emit loopFinished(tr("Task completed"), m_sessionId);
        return;
    }

    // No TASK_PLAN and no completion tag — treat as normal chat, end loop
    qDebug() << "[DEBUG] AgentLoop::processNextIteration: no plan or completion tag, treating as chat";
    SessionManager::instance()->addMessageToSession(m_sessionId, "assistant", response);
    m_state = Completed;
    emit stateChanged(m_state);
    emit loopFinished(tr("Task finished"), m_sessionId);
}

void AgentLoop::confirmPlan() {
    if (m_state != AwaitingUserConfirm) return;
    m_state = Running;
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
    m_iterationCount++;

    qDebug() << "[DEBUG] AgentLoop::executeAndContinue, iteration:" << m_iterationCount
             << "maxIterations:" << m_maxIterations;

    // Check iteration limit
    if (m_iterationCount > m_maxIterations) {
        qDebug() << "[DEBUG] AgentLoop: max iterations reached, stopping";
        m_state = MaxIterations;
        emit stateChanged(m_state);
        emit loopFinished(tr("Maximum iterations reached (%1)").arg(m_maxIterations), m_sessionId);
        return;
    }

    TaskEngine* engine = TaskEngine::instance();
    QVector<CommandResult> results = engine->executePlan(plan);
    qDebug() << "[DEBUG] AgentLoop: executePlan returned" << results.size() << "results";

    // Build structured feedback message
    QString feedback = buildResultFeedback(results);
    m_lastFeedback = feedback;

    qDebug() << "[DEBUG] AgentLoop: feedback built, length:" << feedback.size();

    // Add feedback as a user message to continue the conversation
    ChatMessage feedbackMsg("user", feedback);
    feedbackMsg.isAgentLoopInjected = true;
    SessionManager::instance()->addMessageToSession(m_sessionId, feedbackMsg);

    qDebug() << "[DEBUG] AgentLoop: emitting executionResultReady, sessionId:" << m_sessionId;
    emit executionResultReady(feedback, m_sessionId);
    qDebug() << "[DEBUG] AgentLoop: executionResultReady emitted";
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
    feedback += tr("You MUST respond now. If all operations succeeded, output %1 or %2 with "
                   "a user-facing summary of what was accomplished. If more work is needed, output "
                   "a new %3. Never remain silent — the conversation will stall.")
                        .arg(TaskEngine::kTagTaskComplete,
                             TaskEngine::kTagTaskFinished,
                             TaskEngine::kTagTaskPlan);

    return feedback;
}

bool AgentLoop::isTaskComplete(const QString& response) const {
    QString pattern = QRegularExpression::escape(TaskEngine::kTagTaskComplete) +
                      QStringLiteral("|") +
                      QRegularExpression::escape(TaskEngine::kTagTaskFinished);
    static QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
    return re.match(response).hasMatch();
}
