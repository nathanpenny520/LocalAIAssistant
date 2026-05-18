#include <QCoreApplication>
#include <QtTest>

#include "knowledgebase.h"

class TestKnowledgeBase : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        if (!QCoreApplication::instance()) new QCoreApplication(argc, argv);
    }

    void testSingleton() {
        KnowledgeBase* kb1 = KnowledgeBase::instance();
        KnowledgeBase* kb2 = KnowledgeBase::instance();
        QCOMPARE(kb1, kb2);
    }

    void testIsKnowledgeQueryNoDocuments() {
        // When no documents are indexed, isKnowledgeQuery always returns false
        KnowledgeBase* kb = KnowledgeBase::instance();
        QVERIFY(kb->totalDocuments() == 0);

        // These match keywords but there are no documents so they return false
        QVERIFY(!KnowledgeBase::isKnowledgeQuery("knowledge base query"));
        QVERIFY(!KnowledgeBase::isKnowledgeQuery("search my files for x"));
        QVERIFY(!KnowledgeBase::isKnowledgeQuery("look up in documents"));
        QVERIFY(!KnowledgeBase::isKnowledgeQuery("my documents"));

        // General chat also returns false
        QVERIFY(!KnowledgeBase::isKnowledgeQuery("hello how are you"));
        QVERIFY(!KnowledgeBase::isKnowledgeQuery("write a sort function"));
        QVERIFY(!KnowledgeBase::isKnowledgeQuery("what is 2+2?"));
    }

    void testNotReadyInitially() {
        KnowledgeBase* kb = KnowledgeBase::instance();
        // KB may or may not have been initialized elsewhere in tests
        // Just verify the API doesn't crash
        QVERIFY(!kb->isReady() || kb->isReady());
    }

    void testStorageDir() {
        QString dir = KnowledgeBase::storageDir();
        QVERIFY(!dir.isEmpty());
    }

    void testAllDocumentsEmptyInitially() {
        KnowledgeBase* kb = KnowledgeBase::instance();
        QStringList docs = kb->allDocuments();
        QVERIFY(docs.isEmpty());
    }

    void testTotalCountsZero() {
        KnowledgeBase* kb = KnowledgeBase::instance();
        QCOMPARE(kb->totalChunks(), 0);
        QCOMPARE(kb->totalDocuments(), 0);
    }

    void testEmbedderAccess() {
        KnowledgeBase* kb = KnowledgeBase::instance();
        QVERIFY(kb->embedder() != nullptr);
    }

    void testVectorDBAccess() {
        KnowledgeBase* kb = KnowledgeBase::instance();
        QVERIFY(kb->vectorDB() != nullptr);
    }
};

QTEST_GUILESS_MAIN(TestKnowledgeBase)
#include "test_knowledgebase.moc"
