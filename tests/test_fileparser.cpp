#include <QtTest>
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QTemporaryFile>
#include "filemanager.h"

class TestFileParser : public QObject
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

    void testIsTextFile_KnownExtensions()
    {
        QVERIFY(FileManager::isTextFile("/path/to/file.txt"));
        QVERIFY(FileManager::isTextFile("/path/to/file.md"));
        QVERIFY(FileManager::isTextFile("/path/to/file.cpp"));
        QVERIFY(FileManager::isTextFile("/path/to/file.py"));
        QVERIFY(FileManager::isTextFile("/path/to/file.json"));
        QVERIFY(FileManager::isTextFile("/path/to/file.xml"));
        QVERIFY(FileManager::isTextFile("/path/to/file.yaml"));
        QVERIFY(FileManager::isTextFile("/path/to/file.html"));
        QVERIFY(FileManager::isTextFile("/path/to/file.css"));
        QVERIFY(FileManager::isTextFile("/path/to/file.js"));
    }

    void testIsTextFile_CaseInsensitive()
    {
        QVERIFY(FileManager::isTextFile("/path/to/file.TXT"));
        QVERIFY(FileManager::isTextFile("/path/to/file.MD"));
        QVERIFY(FileManager::isTextFile("/path/to/file.Py"));
    }

    void testIsTextFile_SpecialNames()
    {
        QVERIFY(FileManager::isTextFile("/path/to/Dockerfile"));
        QVERIFY(FileManager::isTextFile("/path/to/Makefile"));
    }

    void testIsTextFile_NotText()
    {
        QVERIFY(!FileManager::isTextFile("/path/to/file.png"));
        QVERIFY(!FileManager::isTextFile("/path/to/file.jpg"));
        QVERIFY(!FileManager::isTextFile("/path/to/file.exe"));
        QVERIFY(!FileManager::isTextFile("/path/to/file.zip"));
    }

    void testIsImageFile()
    {
        QVERIFY(FileManager::isImageFile("/path/to/file.png"));
        QVERIFY(FileManager::isImageFile("/path/to/file.jpg"));
        QVERIFY(FileManager::isImageFile("/path/to/file.jpeg"));
        QVERIFY(FileManager::isImageFile("/path/to/file.gif"));
        QVERIFY(FileManager::isImageFile("/path/to/file.svg"));
    }

    void testIsImageFile_NotImage()
    {
        QVERIFY(!FileManager::isImageFile("/path/to/file.txt"));
        QVERIFY(!FileManager::isImageFile("/path/to/file.mp4"));
    }

    void testAddTextFile()
    {
        QTemporaryFile tmpFile(QDir::tempPath() + "/test_locai_XXXXXX.txt");
        QVERIFY(tmpFile.open());
        tmpFile.write("Hello, this is test content.\nSecond line.");
        tmpFile.flush();
        QString path = tmpFile.fileName();

        FileManager fm;
        bool added = fm.addFile(path);
        QVERIFY(added);
        QCOMPARE(fm.pendingFileCount(), 1);

        FileAttachment att = fm.pendingFiles().first();
        QCOMPARE(att.type, QString("text"));
        QVERIFY(att.content.contains("Hello, this is test content"));
        QVERIFY(att.content.contains("Second line"));
    }

    void testAddFile_Nonexistent()
    {
        FileManager fm;
        bool added = fm.addFile("/tmp/nonexistent_file_xyz_123.txt");
        QVERIFY(!added);
        QCOMPARE(fm.pendingFileCount(), 0);
    }

    void testClearPendingFiles()
    {
        QTemporaryFile tmpFile(QDir::tempPath() + "/test_locai_XXXXXX.txt");
        QVERIFY(tmpFile.open());
        tmpFile.write("test");
        tmpFile.flush();

        FileManager fm;
        fm.addFile(tmpFile.fileName());
        QVERIFY(fm.pendingFileCount() > 0);
        fm.clearPendingFiles();
        QCOMPARE(fm.pendingFileCount(), 0);
    }

    void testFileListSummary()
    {
        QTemporaryFile tmpFile(QDir::tempPath() + "/test_locai_XXXXXX.txt");
        QVERIFY(tmpFile.open());
        tmpFile.write("test content");
        tmpFile.flush();

        FileManager fm;
        QCOMPARE(fm.fileListSummary(), QString("待发送文件列表为空"));
        fm.addFile(tmpFile.fileName());
        QString summary = fm.fileListSummary();
        QVERIFY(summary.contains("test_locai"));
    }

    void testPendingFiles_ReturnsCopy()
    {
        FileManager fm;
        QVector<FileAttachment> files1 = fm.pendingFiles();
        QCOMPARE(files1.size(), 0);

        QTemporaryFile tmpFile(QDir::tempPath() + "/test_locai_XXXXXX.txt");
        QVERIFY(tmpFile.open());
        tmpFile.write("test");
        tmpFile.flush();
        fm.addFile(tmpFile.fileName());

        QVector<FileAttachment> files2 = fm.pendingFiles();
        QCOMPARE(files2.size(), 1);
        // Original empty list should be unchanged (value semantics)
        QCOMPARE(files1.size(), 0);
    }
};

QTEST_GUILESS_MAIN(TestFileParser)
#include "test_fileparser.moc"
