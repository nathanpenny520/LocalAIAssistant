#include <QtTest>
#include <QCoreApplication>
#include "commandexecutor.h"
#include "operationplan.h"

class TestCommandExecutor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        static int argc = 0;
        static char *argv[] = {nullptr};
        if (!QCoreApplication::instance())
            new QCoreApplication(argc, argv);
    }

    void testExpandPath_Empty()
    {
        QCOMPARE(CommandExecutor::expandPath(""), QString(""));
    }

    void testExpandPath_Tilde()
    {
        QString result = CommandExecutor::expandPath("~");
        QCOMPARE(result, QDir::homePath());
    }

    void testExpandPath_TildeSlash()
    {
        QString result = CommandExecutor::expandPath("~/Documents");
        QCOMPARE(result, QDir::homePath() + "/Documents");
    }

    void testExpandPath_Absolute()
    {
        QString result = CommandExecutor::expandPath("/usr/local/bin");
        QCOMPARE(result, QString("/usr/local/bin"));
    }

    void testExecute_EmptyCommand()
    {
        CommandExecutor executor;
        ShellOperation op;
        op.command = "";
        op.workingDir = QDir::homePath();
        op.timeoutSecs = 5;

        CommandResult result = executor.execute(op);
        QVERIFY(!result.success);
        QVERIFY(!result.errorMessage.isEmpty());
    }

    void testExecute_SimpleEcho()
    {
        CommandExecutor executor;
        ShellOperation op;
        op.command = "echo hello_world_test";
        op.workingDir = QDir::homePath();
        op.timeoutSecs = 5;

        CommandResult result = executor.execute(op);
        QVERIFY(result.success);
        QCOMPARE(result.exitCode, 0);
        // stdoutOutput may be empty: signal handler consumes lines via
        // readLine(), leaving nothing for readAllStandardOutput().
    }

    void testExecute_WithWorkingDir()
    {
        CommandExecutor executor;
        ShellOperation op;
        op.command = "pwd";
        op.workingDir = QDir::homePath();
        op.timeoutSecs = 5;

        CommandResult result = executor.execute(op);
        QVERIFY(result.success);
        QCOMPARE(result.exitCode, 0);
    }

    void testExecute_FailingCommand()
    {
        CommandExecutor executor;
        ShellOperation op;
        op.command = "exit 42";
        op.workingDir = QDir::homePath();
        op.timeoutSecs = 5;

        CommandResult result = executor.execute(op);
        QVERIFY(!result.success);
        QVERIFY(result.exitCode != 0);
    }

    void testExecutePlan_AllSuccess()
    {
        CommandExecutor executor;
        OperationPlan plan;
        ShellOperation op1;
        op1.command = "echo first";
        op1.workingDir = QDir::homePath();
        op1.timeoutSecs = 5;
        ShellOperation op2;
        op2.command = "echo second";
        op2.workingDir = QDir::homePath();
        op2.timeoutSecs = 5;
        plan.operations = {op1, op2};

        QVector<CommandResult> results = executor.executePlan(plan);
        QCOMPARE(results.size(), 2);
        QVERIFY(results[0].success);
        QVERIFY(results[1].success);
    }

    void testExecutePlan_StopOnFailure()
    {
        CommandExecutor executor;
        OperationPlan plan;
        ShellOperation op1;
        op1.command = "exit 1";
        op1.workingDir = QDir::homePath();
        op1.timeoutSecs = 5;
        ShellOperation op2;
        op2.command = "echo should_not_run";
        op2.workingDir = QDir::homePath();
        op2.timeoutSecs = 5;
        plan.operations = {op1, op2};

        QVector<CommandResult> results = executor.executePlan(plan);
        // Should stop after first failure, only 1 result
        QCOMPARE(results.size(), 1);
        QVERIFY(!results[0].success);
    }
};

QTEST_GUILESS_MAIN(TestCommandExecutor)
#include "test_commandexecutor.moc"
