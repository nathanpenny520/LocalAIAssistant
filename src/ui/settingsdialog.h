#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include "stylesheetmanager.h"

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

private:
    QLineEdit *m_apiUrlLine;
    QLineEdit *m_apiKeyLine;
    QLineEdit *m_modelNameLine;
    QCheckBox *m_localModeCheckBox;
    QComboBox *m_themeComboBox;
    QComboBox *m_languageComboBox;
    QCheckBox *m_streamingCheckBox;
};

#endif
