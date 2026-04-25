#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTextBrowser>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QAction>
#include <QCloseEvent>
#include <QMenu>
#include <QTextDocument>
#include <QMap>
#include "networkmanager.h"
#include "settingsdialog.h"
#include "sessionmanager.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onSendClicked();
    void onNetworkFinished(const QString &response);
    void onNetworkError(const QString &error);
    void onStreamChunkReceived(const QString &chunk);
    void onStreamFinished(const QString &fullContent);
    void onSettingsClicked();
    void onNewChatClicked();
    void onSessionItemClicked(QListWidgetItem *item);
    void onDeleteSession();
    void onCustomContextMenuRequested(const QPoint &pos);
    void onThemeChanged(int theme);
    void onLanguageChanged();

private:
    void setupUI();
    void setupMenuBar();
    void retranslateUi();
    void appendChatMessage(const QString &sender, const QString &message);
    void renderCurrentSession();
    void updateSessionList();
    void setInputEnabled(bool enabled);
    QMap<QString, QString> parseThinkingContent(const QString &content);
    QString formatMessageWithThinking(const QString &role, const QString &content);

    QListWidget *m_historyList;
    QTextBrowser *m_chatDisplay;
    QLineEdit *m_inputLine;
    QPushButton *m_sendButton;
    QPushButton *m_newChatButton;
    QAction *m_settingsAction;
    QMenu *m_contextMenu;
    QAction *m_deleteAction;
    NetworkManager *m_networkManager;
    QMap<QString, QListWidgetItem*> m_sessionItemMap;
    QTextDocument *m_markdownDoc;

    bool m_isStreaming;
    QString m_streamingContent;
    bool m_isRendering = false;
};

#endif
