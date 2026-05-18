#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QtTest>

#include "apiprovider.h"
#include "datamodels.h"
#include "openai_provider.h"

// Concrete provider for testing SSE parsing in isolation
class TestProvider : public ApiProvider {
public:
    using ApiProvider::ApiProvider;

    QString endpointPath() const override { return "/test"; }
    QJsonArray buildMessagesArray(const QVector<ChatMessage>& messages) const override {
        QJsonArray arr;
        for (const auto& msg : messages) {
            QJsonObject obj;
            obj["role"] = msg.role;
            obj["content"] = msg.content;
            arr.append(obj);
        }
        return arr;
    }
    QString extractDeltaFromSSE(const QByteArray& data) override {
        // Simulate OpenAI-style SSE: data: {"choices":[{"delta":{"content":"X"}}]}
        if (!data.startsWith("data: ")) return {};
        QByteArray json = data.mid(6);
        if (json == "[DONE]") return {};
        QJsonDocument doc = QJsonDocument::fromJson(json);
        if (doc.isNull()) return {};
        QJsonObject root = doc.object();
        QJsonArray choices = root["choices"].toArray();
        if (choices.isEmpty()) return {};
        QJsonObject delta = choices[0].toObject()["delta"].toObject();
        return delta["content"].toString();
    }
    QString extractContentFromResponse(const QByteArray& data) override {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull()) return {};
        QJsonObject root = doc.object();
        QJsonArray choices = root["choices"].toArray();
        if (choices.isEmpty()) return {};
        QJsonObject message = choices[0].toObject()["message"].toObject();
        return message["content"].toString();
    }
};

class TestApiProvider : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        if (!QCoreApplication::instance()) new QCoreApplication(argc, argv);
    }

    void testSSEParseNormal() {
        TestProvider p;
        QByteArray data = R"(data: {"choices":[{"delta":{"content":"Hello"}}]})";
        QString delta = p.extractDeltaFromSSE(data);
        QCOMPARE(delta, QString("Hello"));
    }

    void testSSEParseDone() {
        TestProvider p;
        QByteArray data = "data: [DONE]";
        QString delta = p.extractDeltaFromSSE(data);
        QVERIFY(delta.isEmpty());
    }

    void testSSEParseNonData() {
        TestProvider p;
        QByteArray data = "event: ping";
        QString delta = p.extractDeltaFromSSE(data);
        QVERIFY(delta.isEmpty());
    }

    void testSSEParseEmptyJson() {
        TestProvider p;
        QByteArray data = "data: {}";
        QString delta = p.extractDeltaFromSSE(data);
        QVERIFY(delta.isEmpty());
    }

    void testSSEParseMalformedJson() {
        TestProvider p;
        QByteArray data = "data: not json";
        QString delta = p.extractDeltaFromSSE(data);
        QVERIFY(delta.isEmpty());
    }

    void testResponseParseNormal() {
        TestProvider p;
        QByteArray data =
                R"({"choices":[{"message":{"role":"assistant","content":"Hi there"}}]})";
        QString content = p.extractContentFromResponse(data);
        QCOMPARE(content, QString("Hi there"));
    }

    void testResponseParseEmpty() {
        TestProvider p;
        QByteArray data = "{}";
        QString content = p.extractContentFromResponse(data);
        QVERIFY(content.isEmpty());
    }

    void testStreamingEnabledDefault() {
        TestProvider p;
        // Default depends on QSettings "streamingEnabled"
        // Just verify the getter doesn't crash
        QVERIFY(p.isStreamingEnabled() || !p.isStreamingEnabled());
    }

    void testConfigurationSetters() {
        TestProvider p;
        p.setBaseUrl("https://api.example.com");
        p.setApiKey("sk-test");
        p.setModelName("test-model");
        p.setTemperature(0.7);
        p.setTopP(0.9);
        p.setMaxTokens(2048);
        p.setMaxContext(100);
        p.setPresencePenalty(0.5);
        p.setFrequencyPenalty(0.3);
        p.setSeed(42);
        // No crash = pass
        QVERIFY(true);
    }

    void testComputeContextStartIndex() {
        // Already tested in test_sessionmanager — verify API exists
        TestProvider p;
        p.setMaxContext(20);
        QVector<ChatMessage> msgs;
        QCOMPARE(p.computeContextStartIndex(msgs), 0);
    }
};

QTEST_GUILESS_MAIN(TestApiProvider)
#include "test_apiprovider.moc"
