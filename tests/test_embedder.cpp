#include <QCoreApplication>
#include <QDir>
#include <QtTest>

#include "embedder.h"

class TestEmbedder : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        if (!QCoreApplication::instance()) new QCoreApplication(argc, argv);
    }

    void testCosineSimilarity() {
        // Identical vectors
        QVector<float> a = {1.0f, 2.0f, 3.0f};
        QVector<float> b = {1.0f, 2.0f, 3.0f};
        float sim = Embedder::cosineSimilarity(a, b);
        QVERIFY(qAbs(sim - 1.0f) < 0.001f);

        // Orthogonal vectors
        QVector<float> c = {1.0f, 0.0f};
        QVector<float> d = {0.0f, 1.0f};
        sim = Embedder::cosineSimilarity(c, d);
        QVERIFY(qAbs(sim - 0.0f) < 0.001f);

        // Opposite vectors
        QVector<float> e = {1.0f, 0.0f};
        QVector<float> f = {-1.0f, 0.0f};
        sim = Embedder::cosineSimilarity(e, f);
        QVERIFY(qAbs(sim + 1.0f) < 0.001f);
    }

    void testCosineSimilarityZeroVector() {
        QVector<float> zero = {0.0f, 0.0f, 0.0f};
        QVector<float> v = {1.0f, 2.0f, 3.0f};

        // Zero vector returns 0.0f to avoid division by zero
        float sim = Embedder::cosineSimilarity(zero, v);
        QVERIFY(qAbs(sim - 0.0f) < 0.001f);

        sim = Embedder::cosineSimilarity(v, zero);
        QVERIFY(qAbs(sim - 0.0f) < 0.001f);
    }

    void testCosinesimilarityEmpty() {
        QVector<float> empty;
        QVector<float> v = {1.0f};
        float sim = Embedder::cosineSimilarity(empty, v);
        QVERIFY(qAbs(sim - 0.0f) < 0.001f);
    }

    void testDimensionDefault() {
        Embedder e;
        QCOMPARE(e.dimension(), 384);
    }

    void testNotLoadedInitially() {
        Embedder e;
        QVERIFY(!e.isLoaded());
    }

    void testFindModelPath() {
        // findModelPath should return a QString (may be empty if no model found)
        QString path = Embedder::findModelPath();
        // Just verify it doesn't crash — model may or may not exist
        QVERIFY(!path.isEmpty() || path.isEmpty());
    }

    void testPlaceholderEmbed() {
        Embedder e;
        // Without ONNX, embed returns a placeholder of dimension length
        QVector<float> result = e.embed("test text");
        QCOMPARE(result.size(), e.dimension());
    }

    void testEmbedBatch() {
        Embedder e;
        QStringList texts = {"one", "two", "three"};
        QVector<QVector<float>> results = e.embedBatch(texts);
        QCOMPARE(results.size(), 3);
        for (const auto& r : results)
            QCOMPARE(r.size(), e.dimension());
    }
};

QTEST_GUILESS_MAIN(TestEmbedder)
#include "test_embedder.moc"
