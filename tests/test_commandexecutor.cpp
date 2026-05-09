#include <QCoreApplication>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest>

#include "commandexecutor.h"
#include "operationplan.h"

class TestCommandExecutor : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tempDir;

private slots:
    void initTestCase() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        if (!QCoreApplication::instance()) new QCoreApplication(argc, argv);
        QVERIFY(m_tempDir.isValid());
    }

    // ── expandPath ──

    void testExpandPath_Empty() {
        QCOMPARE(CommandExecutor::expandPath(""), QString(""));
    }

    void testExpandPath_Tilde() {
        QString result = CommandExecutor::expandPath("~");
        QCOMPARE(result, QDir::homePath());
    }

    void testExpandPath_TildeSlash() {
        QString result = CommandExecutor::expandPath("~/Documents");
        QCOMPARE(result, QDir::homePath() + "/Documents");
    }

    void testExpandPath_Absolute() {
        QString result = CommandExecutor::expandPath("/usr/local/bin");
        QCOMPARE(result, QString("/usr/local/bin"));
    }

    // ── Shell commands (existing) ──

    void testExecute_EmptyCommand() {
        CommandExecutor executor;
        ShellOperation op;
        op.command = "";
        op.workingDir = QDir::homePath();
        op.timeoutSecs = 5;

        CommandResult result = executor.execute(op);
        QVERIFY(!result.success);
    }

    void testExecute_SimpleEcho() {
        CommandExecutor executor;
        ShellOperation op;
        op.command = "echo hello_world_test";
        op.workingDir = QDir::homePath();
        op.timeoutSecs = 5;

        CommandResult result = executor.execute(op);
        QVERIFY(result.success);
        QCOMPARE(result.exitCode, 0);
    }

    void testExecute_FailingCommand() {
        CommandExecutor executor;
        ShellOperation op;
        op.command = "exit 42";
        op.workingDir = QDir::homePath();
        op.timeoutSecs = 5;

        CommandResult result = executor.execute(op);
        QVERIFY(!result.success);
        QVERIFY(result.exitCode != 0);
    }

    void testExecutePlan_AllSuccess() {
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

    void testExecutePlan_StopOnFailure() {
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
        QCOMPARE(results.size(), 1);
        QVERIFY(!results[0].success);
    }

    // ── Shell detection ──

    void testShellName_NotEmpty() {
        CommandExecutor executor;
        QVERIFY(!executor.shellName().isEmpty());
    }

    // ── Native: CreateDir ──

    void testCreateDir_Success() {
        CommandExecutor executor;
        QString dirPath = m_tempDir.path() + "/new_directory";
        ShellOperation op;
        op.type = ShellOperation::CreateDir;
        op.target = dirPath;

        CommandResult result = executor.execute(op);
        QVERIFY2(result.success, qPrintable(result.errorMessage));
        QVERIFY(QDir(dirPath).exists());
    }

    void testCreateDir_Nested() {
        CommandExecutor executor;
        QString dirPath = m_tempDir.path() + "/a/b/c";
        ShellOperation op;
        op.type = ShellOperation::CreateDir;
        op.target = dirPath;

        CommandResult result = executor.execute(op);
        QVERIFY2(result.success, qPrintable(result.errorMessage));
        QVERIFY(QDir(dirPath).exists());
    }

    // ── Native: WriteFile ──

    void testWriteFile_Success() {
        CommandExecutor executor;
        QString filePath = m_tempDir.path() + "/test_write.txt";
        ShellOperation op;
        op.type = ShellOperation::WriteFile;
        op.target = filePath;
        op.command = "Hello, World!";

        CommandResult result = executor.execute(op);
        QVERIFY2(result.success, qPrintable(result.errorMessage));

        QFile file(filePath);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll().trimmed(), QByteArray("Hello, World!"));
    }

    void testWriteFile_CreatesParentDir() {
        CommandExecutor executor;
        QString filePath = m_tempDir.path() + "/subdir/test_write.txt";
        ShellOperation op;
        op.type = ShellOperation::WriteFile;
        op.target = filePath;
        op.command = "content";

        CommandResult result = executor.execute(op);
        QVERIFY2(result.success, qPrintable(result.errorMessage));
        QVERIFY(QFile::exists(filePath));
    }

    // ── Native: DeleteFile ──

    void testDeleteFile_File() {
        CommandExecutor executor;
        QString filePath = m_tempDir.path() + "/to_delete.txt";
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("delete me");
        file.close();

        ShellOperation op;
        op.type = ShellOperation::DeleteFile;
        op.source = filePath;

        CommandResult result = executor.execute(op);
        QVERIFY2(result.success, qPrintable(result.errorMessage));
        QVERIFY(!QFile::exists(filePath));
    }

    void testDeleteFile_Directory() {
        CommandExecutor executor;
        QString dirPath = m_tempDir.path() + "/dir_to_delete";
        QDir().mkpath(dirPath);
        QFile file(dirPath + "/file.txt");
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("content");
        file.close();

        ShellOperation op;
        op.type = ShellOperation::DeleteFile;
        op.source = dirPath;

        CommandResult result = executor.execute(op);
        QVERIFY2(result.success, qPrintable(result.errorMessage));
        QVERIFY(!QDir(dirPath).exists());
    }

    void testDeleteFile_NonExistent() {
        CommandExecutor executor;
        ShellOperation op;
        op.type = ShellOperation::DeleteFile;
        op.source = m_tempDir.path() + "/does_not_exist";

        CommandResult result = executor.execute(op);
        QVERIFY(!result.success);
    }

    // ── Native: CopyFile ──

    void testCopyFile_File() {
        CommandExecutor executor;
        QString srcPath = m_tempDir.path() + "/copy_src.txt";
        QString dstPath = m_tempDir.path() + "/copy_dst.txt";
        QFile file(srcPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("copy me");
        file.close();

        ShellOperation op;
        op.type = ShellOperation::CopyFile;
        op.source = srcPath;
        op.target = dstPath;

        CommandResult result = executor.execute(op);
        QVERIFY2(result.success, qPrintable(result.errorMessage));
        QVERIFY(QFile::exists(dstPath));

        QFile dstFile(dstPath);
        QVERIFY(dstFile.open(QIODevice::ReadOnly));
        QCOMPARE(dstFile.readAll().trimmed(), QByteArray("copy me"));
    }

    void testCopyFile_Directory() {
        CommandExecutor executor;
        QString srcDir = m_tempDir.path() + "/copy_src_dir";
        QString dstDir = m_tempDir.path() + "/copy_dst_dir";
        QDir().mkpath(srcDir);
        QFile f1(srcDir + "/a.txt");
        QVERIFY(f1.open(QIODevice::WriteOnly));
        f1.write("a");
        f1.close();
        QDir().mkpath(srcDir + "/sub");
        QFile f2(srcDir + "/sub/b.txt");
        QVERIFY(f2.open(QIODevice::WriteOnly));
        f2.write("b");
        f2.close();

        ShellOperation op;
        op.type = ShellOperation::CopyFile;
        op.source = srcDir;
        op.target = dstDir;

        CommandResult result = executor.execute(op);
        QVERIFY2(result.success, qPrintable(result.errorMessage));
        QVERIFY(QFile::exists(dstDir + "/a.txt"));
        QVERIFY(QFile::exists(dstDir + "/sub/b.txt"));
    }

    // ── Native: MoveFile ──

    void testMoveFile_SameDevice() {
        CommandExecutor executor;
        QString srcPath = m_tempDir.path() + "/move_src.txt";
        QString dstPath = m_tempDir.path() + "/move_dst.txt";
        QFile file(srcPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("move me");
        file.close();

        ShellOperation op;
        op.type = ShellOperation::MoveFile;
        op.source = srcPath;
        op.target = dstPath;

        CommandResult result = executor.execute(op);
        QVERIFY2(result.success, qPrintable(result.errorMessage));
        QVERIFY(!QFile::exists(srcPath));
        QVERIFY(QFile::exists(dstPath));
    }

    void testMoveFile_SourceDoesNotExist() {
        CommandExecutor executor;
        ShellOperation op;
        op.type = ShellOperation::MoveFile;
        op.source = m_tempDir.path() + "/no_such_src";
        op.target = m_tempDir.path() + "/no_such_dst";

        CommandResult result = executor.execute(op);
        QVERIFY(!result.success);
    }

    // ── Native: SearchFiles ──

    void testSearchFiles() {
        CommandExecutor executor;
        QString dir = m_tempDir.path();
        QFile f1(dir + "/alpha.txt");
        QVERIFY(f1.open(QIODevice::WriteOnly));
        f1.close();
        QFile f2(dir + "/beta.txt");
        QVERIFY(f2.open(QIODevice::WriteOnly));
        f2.close();
        QFile f3(dir + "/alpha.log");
        QVERIFY(f3.open(QIODevice::WriteOnly));
        f3.close();

        ShellOperation op;
        op.type = ShellOperation::SearchFiles;
        op.source = dir;
        op.command = "*.txt";

        CommandResult result = executor.execute(op);
        QVERIFY(result.success);
        QVERIFY(result.stdoutOutput.contains("alpha.txt"));
        QVERIFY(result.stdoutOutput.contains("beta.txt"));
        QVERIFY(!result.stdoutOutput.contains("alpha.log"));
    }
};

QTEST_GUILESS_MAIN(TestCommandExecutor)
#include "test_commandexecutor.moc"
