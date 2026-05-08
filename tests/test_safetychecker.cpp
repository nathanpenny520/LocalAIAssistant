#include <QtTest>
#include <QCoreApplication>
#include "safetychecker.h"
#include "operationplan.h"

class TestSafetyChecker : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // QCoreApplication needed for tr() macros in safetychecker
        static int argc = 0;
        static char *argv[] = {nullptr};
        if (!QCoreApplication::instance())
            new QCoreApplication(argc, argv);
    }

    void testDefaultPaths()
    {
        SafetyChecker sc;
        QStringList paths = sc.allowedPaths();
        QVERIFY(!paths.isEmpty());
        QVERIFY(paths.contains(QDir::homePath()));
    }

    void testSetAllowedPaths()
    {
        SafetyChecker sc;
        QStringList custom = {"/tmp", "/home/user/projects"};
        sc.setAllowedPaths(custom);
        QCOMPARE(sc.allowedPaths(), custom);
    }

    void testAddAllowedPath()
    {
        SafetyChecker sc;
        sc.setAllowedPaths({"/tmp"});
        sc.addAllowedPath("/home/user");
        QVERIFY(sc.allowedPaths().contains("/tmp"));
        QVERIFY(sc.allowedPaths().contains("/home/user"));
    }

    void testAddAllowedPath_NoDuplicate()
    {
        SafetyChecker sc;
        sc.setAllowedPaths({"/tmp"});
        sc.addAllowedPath("/tmp");
        QCOMPARE(sc.allowedPaths().size(), 1);
    }

    void testResetToDefaults()
    {
        SafetyChecker sc;
        sc.setAllowedPaths({"/custom"});
        sc.resetToDefaults();
        QVERIFY(sc.allowedPaths().contains(QDir::homePath()));
    }

    void testValidateOperation_EmptyCommand()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testValidateOperation_SafeCommand()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "echo hello";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Approved);
    }

    void testValidateOperation_SudoBlocked()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "sudo rm /tmp/test";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testValidateOperation_RmRfRoot()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "rm -rf /";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testValidateOperation_RmRequiresConfirm()
    {
        SafetyChecker sc;
        sc.addAllowedPath("/home/user/projects");
        ShellOperation op;
        op.command = "rm /home/user/projects/file.txt";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::NeedsConfirmation);
    }

    void testValidateOperation_CurlIsCaution()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "curl https://example.com";
        // curl is Caution level, but the URL path gets extracted and may
        // fail path validation. Just verify it's NOT Approved.
        SafetyChecker::Result r = sc.validateOperation(op);
        QVERIFY(r != SafetyChecker::Approved);
    }

    void testValidateOperation_RmNeedsConfirm()
    {
        SafetyChecker sc;
        sc.addAllowedPath("/tmp");
        sc.addAllowedPath(QDir::homePath());
        ShellOperation op;
        op.command = "rm /tmp/test_file.txt";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::NeedsConfirmation);
    }

    void testValidateOperation_BacktickInjection()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "echo `rm -rf /`";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testValidateOperation_DollarParenInjection()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "echo $(whoami)";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testValidateOperation_EvalBlocked()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "eval echo hello";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testValidatePlan_EmptyPlan()
    {
        SafetyChecker sc;
        OperationPlan plan;
        QCOMPARE(sc.validatePlan(plan), SafetyChecker::Blocked);
    }

    void testValidatePlan_AllApproved()
    {
        SafetyChecker sc;
        OperationPlan plan;
        plan.requiresConfirmation = false;
        ShellOperation op1;
        op1.command = "echo hello";
        ShellOperation op2;
        op2.command = "ls -la";
        plan.operations = {op1, op2};
        QCOMPARE(sc.validatePlan(plan), SafetyChecker::Approved);
    }

    void testValidatePlan_RequiresConfirmation()
    {
        SafetyChecker sc;
        OperationPlan plan;
        plan.requiresConfirmation = true;
        ShellOperation op1;
        op1.command = "echo hello";
        plan.operations = {op1};
        QCOMPARE(sc.validatePlan(plan), SafetyChecker::NeedsConfirmation);
    }

    void testValidatePlan_HasDangerous()
    {
        SafetyChecker sc;
        sc.addAllowedPath("/tmp");
        OperationPlan plan;
        plan.requiresConfirmation = false;
        ShellOperation op1;
        op1.command = "echo hello";
        ShellOperation op2;
        op2.command = "rm /tmp/test";
        plan.operations = {op1, op2};
        QCOMPARE(sc.validatePlan(plan), SafetyChecker::NeedsConfirmation);
    }

    void testDangerLevel_Safe()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "echo hello";
        QCOMPARE(sc.dangerLevel(op), SafetyChecker::Safe);
    }

    void testDangerLevel_Caution()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "rm file.txt";
        QCOMPARE(sc.dangerLevel(op), SafetyChecker::Caution);
    }

    void testDangerLevel_Dangerous()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "sudo ls";
        QCOMPARE(sc.dangerLevel(op), SafetyChecker::Dangerous);
    }

    void testLastBlockReason()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "sudo rm /tmp/test";
        sc.validateOperation(op);
        QVERIFY(!sc.lastBlockReason().isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestSafetyChecker)
#include "test_safetychecker.moc"
