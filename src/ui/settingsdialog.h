#pragma once

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QPlainTextEdit>
#include "stylesheetmanager.h"
#include "networkmanager.h"

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    QString getApiBaseUrl() const;
    QString getApiKey() const;
    QString getModelName() const;
    bool isLocalMode() const;
    StyleSheetManager::Theme getTheme() const;
    QString getLanguage() const;
    bool isStreamingEnabled() const;
    ApiType getApiType() const;

    // Security settings
    QStringList pathWhitelist() const;
    bool operationConfirmationEnabled() const;
    void setOperationConfirmationEnabled(bool enabled);

private slots:
    void showUsageHelp();

private:
    QLineEdit *m_apiUrlLine;
    QLineEdit *m_apiKeyLine;
    QLineEdit *m_modelNameLine;
    QCheckBox *m_localModeCheckBox;
    QComboBox *m_apiTypeComboBox;
    QComboBox *m_themeComboBox;
    QComboBox *m_languageComboBox;
    QCheckBox *m_streamingCheckBox;
    QListWidget *m_kbDocList;

    // Security tab
    QPlainTextEdit *m_pathWhitelistEdit;
    QCheckBox *m_confirmOpsCheckBox;
};

#endif
