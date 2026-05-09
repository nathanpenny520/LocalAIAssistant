#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QtTest>

#include "datamodels.h"
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
        QCOMPARE(sm->allSessions().size(), countBefore - 1);
        QVERIFY(!sm->allSessions().contains(id));
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
};

QTEST_GUILESS_MAIN(TestSessionManager)
#include "test_sessionmanager.moc"
