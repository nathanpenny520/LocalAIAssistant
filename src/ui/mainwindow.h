#pragma once

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTextBrowser>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QAction>
#include <QCloseEvent>
#include <QMenu>
#include <QTextDocument>
#include <QMap>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include "networkmanager.h"
#include "settingsdialog.h"
#include "sessionmanager.h"
#include "filemanager.h"  // 新增
#include "girlfriendwindow.h"
#include "../tasks/taskengine.h"
#include "../knowledge/knowledgebase.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void showEvent(QShowEvent *event) override;

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
    void onRenameSession();
    void onTogglePinSession();
    void onCustomContextMenuRequested(const QPoint &pos);
    void onThemeChanged(int theme);
    void onLanguageChanged();
    void onToggleHistoryPanel();  // 显示/隐藏历史面板
    void onGirlfriendClicked();  // AI女友入口
    void handleTaskResponse(const QString &response);  // 处理AI返回的任务计划

    // 文件相关
    void onFileButtonClicked();           // 新增
    void onRemoveFileClicked();           // 新增

    // 搜索相关
    void onSearchTriggered();             // 显示搜索栏
    void onSearchTextChanged();           // 搜索文本变化
    void onSearchNext();                  // 查找下一个
    void onSearchPrevious();              // 查找上一个
    void onSearchClose();                 // 关闭搜索栏

    // 命令执行实时输出
    void appendCommandOutput(const QString &line);

private:
    void setupUI();
    void setupMenuBar();
    void retranslateUi();
    void appendUserMessageToDisplay(const QString &text, const QVector<FileAttachment> &attachments);
    void renderCurrentSession();
    void updateSessionList();
    void setInputEnabled(bool enabled);
    QMap<QString, QString> parseThinkingContent(const QString &content);
    QString formatMessageWithThinking(const QString &role, const QString &content);
    void adjustInputHeight();             // 动态调整输入框高度
    void updateFileListDisplay();         // 新增
    void clearFileListDisplay();          // 新增
    void setupSearchBar();               // 新增：设置搜索栏
    void updateSearchBarStyle();          // 新增：更新搜索栏样式
    void highlightAllMatches();          // 新增：高亮所有匹配
    void clearHighlights();              // 新增：清除高亮
    void updateCurrentMatchIndex();      // 新增：更新当前匹配索引
    void updateSearchResultLabel();      // 新增：更新搜索结果标签

    QListWidget *m_historyList;
    QTextBrowser *m_chatDisplay;
    QPlainTextEdit *m_inputLine;
    int m_maxInputHeight = 120;          // 输入框最大高度
    QString m_inputPlaceholder;          // 保存占位符文本用于恢复
    QPushButton *m_sendButton;
    QPushButton *m_newChatButton;
    QAction *m_settingsAction;
    QAction *m_toggleHistoryAction;  // 显示/隐藏历史面板
    QMenu *m_contextMenu;
    QAction *m_deleteAction;
    QAction *m_renameAction;
    QAction *m_pinAction;
    NetworkManager *m_networkManager;
    QMap<QString, QListWidgetItem*> m_sessionItemMap;
    QTextDocument *m_markdownDoc;
    QSplitter *m_splitter;           // 主分割器
    QWidget *m_leftPanel;            // 左侧面板（历史列表）

    bool m_isStreaming;
    bool m_firstShow = true;
    bool m_suppressRender = false;   // 仅阻止 onSendClicked 期间的 renderCurrentSession
    QString m_streamingContent;
    bool m_streamEndedWithNewline = false;
    bool m_isRendering = false;
    QString m_requestSessionId;  // 记录发起请求时的会话ID
    QString m_contextMenuSessionId;  // 右键菜单/⋯按钮对应的会话ID

    // 文件相关成员
    FileManager *m_fileManager;           // 新增
    QPushButton *m_fileButton;            // 新增
    QWidget *m_fileListArea;              // 新增
    QHBoxLayout *m_fileListLayout;        // 新增

    // 搜索相关成员
    QFrame *m_searchBar;                  // 新增：搜索栏容器
    QLineEdit *m_searchInput;             // 新增：搜索输入框
    QPushButton *m_searchPrevBtn;         // 新增：上一个按钮
    QPushButton *m_searchNextBtn;         // 新增：下一个按钮
    QPushButton *m_searchCloseBtn;        // 新增：关闭按钮
    QLabel *m_searchResultLabel;          // 新增：搜索结果计数
    QAction *m_searchAction;              // 新增：搜索快捷键动作
    // AI女友入口
    QAction *m_girlfriendAction;
    int m_currentMatchIndex;              // 新增：当前匹配索引
    int m_totalMatches;                   // 新增：总匹配数
};

#endif
