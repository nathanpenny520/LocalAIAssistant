#pragma once

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QCloseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTextBrowser>
#include <QTextDocument>

#include "../knowledge/knowledgebase.h"
#include "../tasks/agentloop.h"
#include "../tasks/safetychecker.h"
#include "../tasks/taskengine.h"
#include "filemanager.h"
#include "girlfriendwindow.h"
#include "networkmanager.h"
#include "sessionmanager.h"
#include "settingsdialog.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onSendClicked();
    void onNetworkFinished(const QString& response);
    void onNetworkError(const QString& error);
    void onStreamChunkReceived(const QString& chunk);
    void onStreamFinished(const QString& fullContent);
    void onSettingsClicked();
    void onNewChatClicked();
    void onSessionItemClicked(QListWidgetItem* item);
    void onDeleteSession();
    void onRenameSession();
    void onTogglePinSession();
    void onCustomContextMenuRequested(const QPoint& pos);
    void onThemeChanged(int theme);
    void onLanguageChanged();
    void onToggleHistoryPanel();
    void onGirlfriendClicked();                        // AI girlfriend entry point
    void handleTaskResponse(const QString& response);  // Handle task plan in AI response

    // AgentLoop slots
    void onAgentLoopResultReady(const QString& feedbackMessage, const QString& sessionId);
    void onAgentLoopPlanConfirm(const OperationPlan& plan,
                                const QVector<PathViolation>& violations);
    void onAgentLoopFinished(const QString& summary, const QString& sessionId);

    // File operations
    void onFileButtonClicked();
    void onRemoveFileClicked();

    // Search operations
    void onSearchTriggered();
    void onSearchTextChanged();
    void onSearchNext();
    void onSearchPrevious();
    void onSearchClose();

    // Command execution live output
    void appendCommandOutput(const QString& line);

private:
    void setupUI();
    void setupMenuBar();
    void retranslateUi();
    void appendUserMessageToDisplay(const QString& text,
                                    const QVector<FileAttachment>& attachments);
    void renderCurrentSession();
    void updateSessionList();
    void setInputEnabled(bool enabled);
    QMap<QString, QString> parseThinkingContent(const QString& content);
    QString formatMessageWithThinking(const QString& role, const QString& content);
    void adjustInputHeight();
    void resizeEvent(QResizeEvent* event) override;
    void stopCurrentStreamingSession();  // abort in-flight request, save partial content
    void updateFileListDisplay();
    void clearFileListDisplay();
    void setupSearchBar();
    void updateSearchBarStyle();
    void highlightAllMatches();
    void clearHighlights();
    void updateCurrentMatchIndex();
    void updateSearchResultLabel();

    QListWidget* m_historyList;
    QTextBrowser* m_chatDisplay;
    QPlainTextEdit* m_inputLine;
    int m_maxInputHeight = 300;
    QString m_inputPlaceholder;  // Saved placeholder text for restoration after IME input
    QPushButton* m_sendButton;
    QPushButton* m_newChatButton;
    QAction* m_settingsAction;
    QAction* m_toggleHistoryAction;
    QMenu* m_contextMenu;
    QAction* m_deleteAction;
    QAction* m_renameAction;
    QAction* m_pinAction;
    NetworkManager* m_networkManager;
    QMap<QString, QListWidgetItem*> m_sessionItemMap;
    QTextDocument* m_markdownDoc;
    QSplitter* m_splitter;
    QWidget* m_leftPanel;

    bool m_isStreaming;
    bool m_firstShow = true;
    bool m_suppressRender = false;  // Suppress renderCurrentSession only during onSendClicked
    QString m_streamingContent;
    bool m_streamEndedWithNewline = false;
    bool m_isRendering = false;
    QString m_requestSessionId;
    QString m_contextMenuSessionId;  // Session ID for right-click context menu

    // File-related members
    FileManager* m_fileManager;
    QPushButton* m_fileButton;
    QWidget* m_fileListArea;
    QHBoxLayout* m_fileListLayout;

    // Search-related members
    QFrame* m_searchBar;
    QLineEdit* m_searchInput;
    QPushButton* m_searchPrevBtn;
    QPushButton* m_searchNextBtn;
    QPushButton* m_searchCloseBtn;
    QLabel* m_searchResultLabel;
    QAction* m_searchAction;
    // AI girlfriend entry point
    QAction* m_girlfriendAction;
    int m_currentMatchIndex;
    int m_totalMatches;
};

#endif
