#include "agentloop.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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
    if (m_state != Running) return;
    processNextIteration(response);
}

void AgentLoop::processNextIteration(const QString& response) {
    // Check for TASK_PLAN first — the AI may mix TASK_COMPLETE (closing prior
    // context) and TASK_PLAN (new work) in a single response. Processing the plan
    // takes precedence; TASK_COMPLETE alone ends the loop.
    TaskEngine* engine = TaskEngine::instance();
    OperationPlan plan = engine->parsePlanFromAIResponse(response);

    if (!plan.isEmpty()) {
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
        return;
    }

    // No TASK_PLAN — check if this is a task completion
    if (isTaskComplete(response)) {
        SessionManager::instance()->addMessageToSession(m_sessionId, "assistant", response);
        m_state = Completed;
        emit stateChanged(m_state);
        emit loopFinished(tr("Task completed"), m_sessionId);
        return;
    }

    // No TASK_PLAN and no completion tag — treat as normal chat, end loop
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

    // Check iteration limit
    if (m_iterationCount > m_maxIterations) {
        m_state = MaxIterations;
        emit stateChanged(m_state);
        emit loopFinished(tr("Maximum iterations reached (%1)").arg(m_maxIterations), m_sessionId);
        return;
    }

    TaskEngine* engine = TaskEngine::instance();
    QVector<CommandResult> results = engine->executePlan(plan);

    // Build structured feedback message
    QString feedback = buildResultFeedback(plan, results);
    m_lastFeedback = feedback;

    // Add feedback as a user message to continue the conversation
    ChatMessage feedbackMsg("user", feedback);
    feedbackMsg.isAgentLoopInjected = true;
    SessionManager::instance()->addMessageToSession(m_sessionId, feedbackMsg);

    emit executionResultReady(feedback, m_sessionId);
}

QString AgentLoop::buildResultFeedback(const OperationPlan& plan,
                                       const QVector<CommandResult>& results) const {
    static const int kMaxFieldLength = 4000;

    int successCount = 0;
    int failCount = 0;
    QJsonArray opsArray;

    for (int i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        if (r.success) successCount++;
        else failCount++;

        QJsonObject opObj;
        opObj[QStringLiteral("index")] = i + 1;
        opObj[QStringLiteral("success")] = r.success;
        opObj[QStringLiteral("exit_code")] = r.exitCode;

        if (i < plan.operations.size()) {
            opObj[QStringLiteral("type")] = plan.operations[i].typeName();
            if (!plan.operations[i].description.isEmpty())
                opObj[QStringLiteral("description")] = plan.operations[i].description;
        }

        auto truncated = [](const QString& s, int maxLen) -> QString {
            if (s.length() <= maxLen) return s;
            return s.left(maxLen - 3) + QStringLiteral("...");
        };

        opObj[QStringLiteral("stdout")] = truncated(r.stdoutOutput.trimmed(), kMaxFieldLength);
        opObj[QStringLiteral("stderr")] = truncated(r.stderrOutput.trimmed(), kMaxFieldLength);
        opObj[QStringLiteral("error")] = r.errorMessage;
        opObj[QStringLiteral("elapsed_ms")] = static_cast<double>(r.elapsedMs);

        opsArray.append(opObj);
    }

    QJsonObject root;
    root[QStringLiteral("iteration")] = m_iterationCount;
    root[QStringLiteral("summary")] = QJsonObject{
        {QStringLiteral("total"), results.size()},
        {QStringLiteral("succeeded"), successCount},
        {QStringLiteral("failed"), failCount}
    };
    root[QStringLiteral("operations")] = opsArray;

    QString feedback;
    feedback += QStringLiteral("[ITERATION_FEEDBACK]\n");
    feedback += QString::fromUtf8(
        QJsonDocument(root).toJson(QJsonDocument::Indented));

    feedback += QStringLiteral("\n\n");
    feedback += tr("You MUST respond now. If all operations succeeded, output %1 or %2 with "
                   "a user-facing summary of what was accomplished. If more work is needed, output "
                   "a new %3. Never remain silent — the conversation will stall.")
                        .arg(TaskEngine::kTagTaskComplete,
                             TaskEngine::kTagTaskFinished,
                             TaskEngine::kTagTaskPlan);
    feedback += QLatin1Char('\n');

    return feedback;
}

bool AgentLoop::isTaskComplete(const QString& response) const {
    QString pattern = QRegularExpression::escape(TaskEngine::kTagTaskComplete) +
                      QStringLiteral("|") +
                      QRegularExpression::escape(TaskEngine::kTagTaskFinished);
    static QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
    return re.match(response).hasMatch();
}
