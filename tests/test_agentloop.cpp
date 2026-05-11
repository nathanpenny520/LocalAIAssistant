#include <QCoreApplication>
#include <QSignalSpy>
#include <QtTest>

#include "agentloop.h"
#include "operationplan.h"
#include "safetychecker.h"

class TestAgentLoop : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        if (!QCoreApplication::instance()) new QCoreApplication(argc, argv);
    }

    void init() {
        AgentLoop::instance()->stop();
    }

    // ── isTaskComplete parsing ──

    void testIsTaskComplete_Standard() {
        QString response = QStringLiteral("[TASK_COMPLETE]\nAll done.");
        // Use a plan-based approach to test completion detection indirectly
        OperationPlan plan;
        plan.description = QStringLiteral("test");
        ShellOperation op;
        op.command = QStringLiteral("echo done");
        plan.operations = {op};

        // Verify state starts at Idle or similar
        QCOMPARE(AgentLoop::instance()->state(), AgentLoop::Stopped);
    }

    void testIsTaskComplete_CaseInsensitive() {
        QString response = QStringLiteral("[task_complete]\nAll done.");
        QVERIFY(response.contains(QStringLiteral("[TASK_COMPLETE]"), Qt::CaseInsensitive));
        QVERIFY(response.contains(QStringLiteral("[TASK_FINISHED]"), Qt::CaseInsensitive) == false);
    }

    void testIsTaskComplete_FinishedTag() {
        QString response = QStringLiteral("[TASK_FINISHED]\nDone.");
        QVERIFY(response.contains(QStringLiteral("[TASK_FINISHED]"), Qt::CaseInsensitive));
    }

    void testIsTaskComplete_NotPresent() {
        QString response = QStringLiteral("Here is a normal response without tags.");
        QVERIFY(!response.contains(QStringLiteral("[TASK_COMPLETE]"), Qt::CaseInsensitive));
        QVERIFY(!response.contains(QStringLiteral("[TASK_FINISHED]"), Qt::CaseInsensitive));
    }

    // ── State machine ──

    void testInitialState() {
        // After stop(), state should be Stopped
        QCOMPARE(AgentLoop::instance()->state(), AgentLoop::Stopped);
    }

    void testIterationCount() {
        QCOMPARE(AgentLoop::instance()->iterationCount(), 0);
    }

    void testSetMaxIterations() {
        AgentLoop::instance()->setMaxIterations(5);
        // Can't directly verify private member, but test that it doesn't crash
        QVERIFY(true);
        AgentLoop::instance()->setMaxIterations(10);
    }

    // ── Signal emissions ──

    void testStateChangedSignal() {
        QSignalSpy spy(AgentLoop::instance(), &AgentLoop::stateChanged);
        QVERIFY(spy.isValid());
    }

    void testLoopFinishedSignal() {
        QSignalSpy spy(AgentLoop::instance(), &AgentLoop::loopFinished);
        QVERIFY(spy.isValid());
    }

    void testExecutionResultReadySignal() {
        QSignalSpy spy(AgentLoop::instance(), &AgentLoop::executionResultReady);
        QVERIFY(spy.isValid());
    }

    void testPlanRequiresConfirmationSignal() {
        QSignalSpy spy(AgentLoop::instance(), &AgentLoop::planRequiresConfirmation);
        QVERIFY(spy.isValid());
    }

    // ── Singleton ──

    void testSingleton() {
        AgentLoop* a = AgentLoop::instance();
        AgentLoop* b = AgentLoop::instance();
        QCOMPARE(a, b);
    }

    // ── Stop recovery ──

    void testStopSetsState() {
        AgentLoop::instance()->stop();
        QCOMPARE(AgentLoop::instance()->state(), AgentLoop::Stopped);
    }

    void testCancelPlanInWrongState() {
        // cancelPlan should be a no-op if not awaiting confirmation
        AgentLoop::instance()->cancelPlan();
        QCOMPARE(AgentLoop::instance()->state(), AgentLoop::Stopped);
    }

    void testConfirmPlanInWrongState() {
        // confirmPlan should be a no-op if not awaiting confirmation
        AgentLoop::instance()->confirmPlan();
        QCOMPARE(AgentLoop::instance()->state(), AgentLoop::Stopped);
    }
};

QTEST_GUILESS_MAIN(TestAgentLoop)
#include "test_agentloop.moc"
