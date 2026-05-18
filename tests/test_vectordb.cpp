#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "textchunker.h"
#include "vectordb.h"

class TestVectorDB : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        if (!QCoreApplication::instance()) new QCoreApplication(argc, argv);
    }

    void testInit() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        VectorDB db;
        QVERIFY(db.init(128, dir.path()));
        QCOMPARE(db.totalChunks(), 0);
        QCOMPARE(db.totalDocuments(), 0);
    }

    void testInitInvalidDir() {
        // Create a temporary file, then try to use a path inside it as a storage dir.
        // mkpath fails because a file (not a directory) exists at that path component.
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString filePath = dir.path() + "/not_a_dir";
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();

        VectorDB db;
        QVERIFY(!db.init(128, filePath + "/chunks"));
    }

    void testAddAndSearch() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        VectorDB db;
        QVERIFY(db.init(4, dir.path()));

        // Build test chunks
        TextChunk c1, c2, c3;
        c1.content = "hello world";
        c1.documentPath = "/tmp/doc1.txt";
        c2.content = "foo bar baz";
        c2.documentPath = "/tmp/doc1.txt";
        c3.content = "goodbye world";
        c3.documentPath = "/tmp/doc2.txt";

        QVector<QVector<float>> vectors = {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {-1.0f, 0.0f, 0.0f, 0.0f},
        };

        db.addVectors(vectors, {c1, c2, c3});
        QCOMPARE(db.totalChunks(), 3);
        QCOMPARE(db.totalDocuments(), 2);

        // Search for something similar to c1
        QVector<float> query = {0.9f, 0.0f, 0.0f, 0.0f};
        QVector<SearchResult> results = db.search(query, 2);
        QVERIFY(!results.isEmpty());
        QVERIFY(results.size() <= 2);
    }

    void testSearchEmpty() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        VectorDB db;
        QVERIFY(db.init(4, dir.path()));

        QVector<float> query = {1.0f, 0.0f, 0.0f, 0.0f};
        QVector<SearchResult> results = db.search(query, 5);
        QVERIFY(results.isEmpty());
    }

    void testRemoveDocument() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        VectorDB db;
        QVERIFY(db.init(4, dir.path()));

        TextChunk c1;
        c1.content = "doc a text";
        c1.documentPath = "/tmp/doc_a.txt";
        TextChunk c2;
        c2.content = "doc b text";
        c2.documentPath = "/tmp/doc_b.txt";

        db.addVectors({{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}}, {c1, c2});
        QCOMPARE(db.totalDocuments(), 2);

        int removed = db.removeDocument("/tmp/doc_a.txt");
        QVERIFY(removed >= 0);
        QCOMPARE(db.totalDocuments(), 1);
        QVERIFY(!db.allDocuments().contains("/tmp/doc_a.txt"));
    }

    void testAllDocuments() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        VectorDB db;
        QVERIFY(db.init(4, dir.path()));

        TextChunk c;
        c.content = "sample";
        c.documentPath = "/tmp/uniq.txt";

        db.addVectors({{1.0f, 0.0f, 0.0f, 0.0f}}, {c});
        QStringList docs = db.allDocuments();
        QVERIFY(docs.contains("/tmp/uniq.txt"));
    }

    void testSaveLoad() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        // First instance: add data and save
        {
            VectorDB db;
            QVERIFY(db.init(4, dir.path()));

            TextChunk c;
            c.content = "persist me";
            c.documentPath = "/tmp/persist.txt";

            db.addVectors({{0.5f, 0.5f, 0.0f, 0.0f}}, {c});
            QVERIFY(db.save());
        }

        // Second instance: load and verify
        {
            VectorDB db2;
            QVERIFY(db2.init(4, dir.path()));
            QVERIFY(db2.load());
            QCOMPARE(db2.totalChunks(), 1);
            QCOMPARE(db2.totalDocuments(), 1);
        }
    }
};

QTEST_GUILESS_MAIN(TestVectorDB)
#include "test_vectordb.moc"
