#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QtTest>

#include "apiprovider.h"
#include "datamodels.h"
#include "openai_provider.h"
#include "sessionmanager.h"

class TestSessionManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        if (!QCoreApplication::instance()) new QCoreApplication(argc, argv);
    }

    void testSingleton() {
        SessionManager* sm1 = SessionManager::instance();
        SessionManager* sm2 = SessionManager::instance();
        QCOMPARE(sm1, sm2);
    }

    void testInitialSession() {
        SessionManager* sm = SessionManager::instance();
        QVERIFY(!sm->currentSessionId().isEmpty());
        QVERIFY(!sm->allSessions().isEmpty());
    }

    void testCreateNewSession() {
        SessionManager* sm = SessionManager::instance();
        int countBefore = sm->allSessions().size();
        sm->createNewSession("Test Session");
        QCOMPARE(sm->allSessions().size(), countBefore + 1);
        QVERIFY(sm->currentSession().title == "Test Session");
    }

    void testSwitchSession() {
        SessionManager* sm = SessionManager::instance();
        sm->createNewSession("Session A");
        QString idA = sm->currentSessionId();
        sm->createNewSession("Session B");
        QString idB = sm->currentSessionId();
        QVERIFY(idA != idB);

        sm->switchToSession(idA);
        QCOMPARE(sm->currentSessionId(), idA);
        QCOMPARE(sm->currentSession().title, QString("Session A"));
    }

    void testSwitchToInvalidSession() {
        SessionManager* sm = SessionManager::instance();
        QString currentBefore = sm->currentSessionId();
        sm->switchToSession("nonexistent-id");
        // Should not change
        QCOMPARE(sm->currentSessionId(), currentBefore);
    }

    void testAddMessage() {
        SessionManager* sm = SessionManager::instance();
        sm->createNewSession("Message Test");
        int msgCountBefore = sm->currentSession().messages.size();
        sm->addMessageToCurrentSession("user", "Hello, world!");
        QCOMPARE(sm->currentSession().messages.size(), msgCountBefore + 1);
        QCOMPARE(sm->currentSession().messages.last().role, QString("user"));
        QCOMPARE(sm->currentSession().messages.last().content, QString("Hello, world!"));
    }

    void testAddMessageWithAttachments() {
        SessionManager* sm = SessionManager::instance();
        sm->createNewSession("Attachment Test");

        FileAttachment att;
        att.path = "/tmp/test.txt";
        att.type = "text";
        att.mimeType = "text/plain";
        att.content = "test content";
        att.size = 100;

        sm->addMessageToCurrentSession("user", "File attached", QVector<FileAttachment> {att});
        QVERIFY(!sm->currentSession().messages.isEmpty());
        QVERIFY(!sm->currentSession().messages.last().attachments.isEmpty());
        QCOMPARE(sm->currentSession().messages.last().attachments[0].path, QString("/tmp/"
                                                                                   "test.txt"));
    }

    void testUpdateSessionTitle() {
        SessionManager* sm = SessionManager::instance();
        sm->createNewSession("Original Title");
        QString id = sm->currentSessionId();
        sm->updateSessionTitle(id, "Updated Title");
        QCOMPARE(sm->allSessions()[id].title, QString("Updated Title"));
    }

    void testSetSessionPinned() {
        SessionManager* sm = SessionManager::instance();
        sm->createNewSession("Pin Test");
        QString id = sm->currentSessionId();
        QVERIFY(!sm->allSessions()[id].pinned);
        sm->setSessionPinned(id, true);
        QVERIFY(sm->allSessions()[id].pinned);
    }

    void testRemoveSession() {
        SessionManager* sm = SessionManager::instance();
        sm->createNewSession("To Remove");
        QString id = sm->currentSessionId();
        int countBefore = sm->allSessions().size();
        sm->removeSession(id);
        // Removing the current session creates a new one (count stays the same)
        QCOMPARE(sm->allSessions().size(), countBefore);
        QVERIFY(!sm->allSessions().contains(id));
        // Verify currentSessionId now points to a valid, different session
        QVERIFY(!sm->currentSessionId().isEmpty());
        QVERIFY(sm->currentSessionId() != id);
        QVERIFY(sm->allSessions().contains(sm->currentSessionId()));
    }

    void testJsonRoundTrip() {
        SessionManager* sm = SessionManager::instance();
        sm->createNewSession("JSON Test");
        sm->addMessageToCurrentSession("user", "Message 1");
        sm->addMessageToCurrentSession("assistant", "Response 1");

        QString id = sm->currentSessionId();
        QString title = sm->currentSession().title;
        int msgCount = sm->currentSession().messages.size();

        // Save to file
        sm->saveSessionsToFile();

        // Load from file
        sm->loadSessionsFromFile();

        // Verify session was restored
        QVERIFY(sm->allSessions().contains(id));
        QCOMPARE(sm->allSessions()[id].title, title);
        QCOMPARE(sm->allSessions()[id].messages.size(), msgCount);
        QCOMPARE(sm->allSessions()[id].messages[0].content, QString("Message 1"));
    }

    void testJsonRoundTrip_EmptySessions() {
        SessionManager* sm = SessionManager::instance();
        // Clear all sessions, create one to ensure file is written
        // saveSessionsToFile writes all current sessions
        sm->saveSessionsToFile();
        sm->loadSessionsFromFile();
        // After reload, we should have the same sessions that were saved
        QVERIFY(!sm->allSessions().isEmpty());
    }

    // ── computeContextStartIndex boundary tests ────────────────

    void testContextStartIndex_EmptyMessages() {
        OpenAIProvider provider;
        provider.setMaxContext(20);
        QVector<ChatMessage> messages;
        QCOMPARE(provider.computeContextStartIndex(messages), 0);
    }

    void testContextStartIndex_AllRealExactlyAtLimit() {
        OpenAIProvider provider;
        provider.setMaxContext(20);
        QVector<ChatMessage> messages;
        for (int i = 0; i < 20; ++i)
            messages.append(ChatMessage("user", QString("msg %1").arg(i)));
        // 20 real messages at limit → all sent, startIndex 0
        QCOMPARE(provider.computeContextStartIndex(messages), 0);
    }

    void testContextStartIndex_AllRealOverLimit() {
        OpenAIProvider provider;
        provider.setMaxContext(20);
        QVector<ChatMessage> messages;
        for (int i = 0; i < 21; ++i)
            messages.append(ChatMessage("user", QString("msg %1").arg(i)));
        // 21 real messages, max 20 → oldest dropped, startIndex 1
        QCOMPARE(provider.computeContextStartIndex(messages), 1);
    }

    void testContextStartIndex_AllInjected() {
        OpenAIProvider provider;
        provider.setMaxContext(20);
        QVector<ChatMessage> messages;
        for (int i = 0; i < 10; ++i) {
            ChatMessage msg("user", QString("injected %1").arg(i));
            msg.isAgentLoopInjected = true;
            messages.append(msg);
        }
        // All injected, no real messages → all sent, startIndex 0
        QCOMPARE(provider.computeContextStartIndex(messages), 0);
    }

    void testContextStartIndex_MixedRealAndInjected() {
        OpenAIProvider provider;
        provider.setMaxContext(20);
        QVector<ChatMessage> messages;
        // 18 real + 5 injected interleaved
        for (int i = 0; i < 3; ++i) {
            messages.append(ChatMessage("user", QString("real %1").arg(i)));
        }
        for (int i = 0; i < 5; ++i) {
            ChatMessage msg("user", QString("injected %1").arg(i));
            msg.isAgentLoopInjected = true;
            messages.append(msg);
        }
        for (int i = 3; i < 18; ++i) {
            messages.append(ChatMessage("user", QString("real %1").arg(i)));
        }
        // 18 real (< 20 limit) → all sent, injected included naturally
        QCOMPARE(provider.computeContextStartIndex(messages), 0);
    }

    void testContextStartIndex_OldInjectedExcluded() {
        OpenAIProvider provider;
        provider.setMaxContext(20);
        QVector<ChatMessage> messages;
        // 3 injected at positions 0-2
        for (int i = 0; i < 3; ++i) {
            ChatMessage msg("user", QString("old injected %1").arg(i));
            msg.isAgentLoopInjected = true;
            messages.append(msg);
        }
        // 25 real messages at positions 3-27
        for (int i = 0; i < 25; ++i)
            messages.append(ChatMessage("user", QString("real %1").arg(i)));
        // 25 real, limit 20 → scanning backward hits 20 real at pos 8
        // Injected at pos 0-2 are outside the window and excluded
        QCOMPARE(provider.computeContextStartIndex(messages), 8);
    }

    void testContextStartIndex_PreserveFalseExcludesInjected() {
        OpenAIProvider provider;
        provider.setMaxContext(20);
        // Disable preserving loop messages
        QSettings settings("LocalAIAssistant", "Settings");
        settings.setValue("preserveAgentLoopMessages", false);

        QVector<ChatMessage> messages;
        // 10 injected + 25 real = 35 total, limit 20
        for (int i = 0; i < 5; ++i) {
            ChatMessage msg("user", QString("injected %1").arg(i));
            msg.isAgentLoopInjected = true;
            messages.append(msg);
        }
        for (int i = 0; i < 25; ++i)
            messages.append(ChatMessage("user", QString("real %1").arg(i)));
        for (int i = 5; i < 10; ++i) {
            ChatMessage msg("user", QString("injected %1").arg(i));
            msg.isAgentLoopInjected = true;
            messages.append(msg);
        }
        // With preserve=false, injected are skipped entirely in backward scan
        // 25 real messages, limit 20 → startIndex should be 5 real messages in
        // But injected are skipped, so we count only real
        // Expected: startIndex at the 5th real message from the front
        // Messages: 5 injected + 20 real (pos 5-24) + 5 real (pos 25-29) + 5 injected (pos 30-34)
        // Scanning backward from 34: skip 5 injected, count 5 real (29-25), skip 5 injected...
        // Actually: need 20 real. From back: skip 5 injected, count real at 29,28,... → need 20 real
        // Real messages are at pos 5-29 (25 real). Need 20 → start at pos 5+5=10
        QCOMPARE(provider.computeContextStartIndex(messages), 10);

        // Restore default
        settings.setValue("preserveAgentLoopMessages", true);
    }
};

QTEST_GUILESS_MAIN(TestSessionManager)
#include "test_sessionmanager.moc"
