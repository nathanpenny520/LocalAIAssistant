#include "settingsdialog.h"
#include "translationmanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_apiUrlLine(new QLineEdit(this))
    , m_apiKeyLine(new QLineEdit(this))
    , m_modelNameLine(new QLineEdit(this))
    , m_localModeCheckBox(new QCheckBox(tr("使用本地模式"), this))
    , m_themeComboBox(new QComboBox(this))
    , m_languageComboBox(new QComboBox(this))
    , m_streamingCheckBox(new QCheckBox(tr("启用流式输出"), this))
{
    setWindowTitle(tr("设置"));
    setMinimumWidth(420);

    QSettings settings("LocalAIAssistant", "Settings");

    m_apiUrlLine->setText(settings.value("apiBaseUrl", "http://127.0.0.1:8080").toString());
    m_apiKeyLine->setEchoMode(QLineEdit::Password);
    m_apiKeyLine->setText(settings.value("apiKey", "").toString());
    m_modelNameLine->setText(settings.value("modelName", "local-model").toString());
    m_localModeCheckBox->setChecked(settings.value("localMode", true).toBool());
    m_streamingCheckBox->setChecked(settings.value("streamingEnabled", true).toBool());

    m_themeComboBox->addItem(tr("跟随系统"), static_cast<int>(StyleSheetManager::SystemTheme));
    m_themeComboBox->addItem(tr("亮色"), static_cast<int>(StyleSheetManager::LightTheme));
    m_themeComboBox->addItem(tr("暗色"), static_cast<int>(StyleSheetManager::DarkTheme));
    int themeValue = settings.value("theme", static_cast<int>(StyleSheetManager::SystemTheme)).toInt();
    for (int i = 0; i < m_themeComboBox->count(); ++i) {
        if (m_themeComboBox->itemData(i).toInt() == themeValue) {
            m_themeComboBox->setCurrentIndex(i);
            break;
        }
    }

    m_languageComboBox->addItem(tr("简体中文"), "zh_CN");
    m_languageComboBox->addItem(tr("English"), "en");
    QString savedLang = settings.value("language", "system").toString();
    if (savedLang == "system") {
        QLocale locale = QLocale::system();
        if (locale.language() == QLocale::Chinese) {
            m_languageComboBox->setCurrentIndex(0);
        } else {
            m_languageComboBox->setCurrentIndex(1);
        }
    } else {
        for (int i = 0; i < m_languageComboBox->count(); ++i) {
            if (m_languageComboBox->itemData(i).toString() == savedLang) {
                m_languageComboBox->setCurrentIndex(i);
                break;
            }
        }
    }

    QGroupBox *apiGroup = new QGroupBox(tr("API 设置"), this);
    QFormLayout *apiLayout = new QFormLayout(apiGroup);
    apiLayout->addRow(tr("API 基础 URL:"), m_apiUrlLine);
    apiLayout->addRow(tr("API 密钥:"), m_apiKeyLine);
    apiLayout->addRow(tr("模型名称:"), m_modelNameLine);
    apiLayout->addRow(m_localModeCheckBox);
    apiLayout->addRow(m_streamingCheckBox);

    QGroupBox *appearanceGroup = new QGroupBox(tr("外观"), this);
    QFormLayout *appearanceLayout = new QFormLayout(appearanceGroup);
    appearanceLayout->addRow(tr("主题:"), m_themeComboBox);
    appearanceLayout->addRow(tr("语言:"), m_languageComboBox);

    QPushButton *okButton = new QPushButton(tr("确定"), this);
    QPushButton *cancelButton = new QPushButton(tr("取消"), this);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(apiGroup);
    mainLayout->addWidget(appearanceGroup);
    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, [this]() {
        QSettings settings("LocalAIAssistant", "Settings");
        QString oldLanguage = settings.value("language", "system").toString();
        QString newLanguage = m_languageComboBox->currentData().toString();

        settings.setValue("apiBaseUrl", m_apiUrlLine->text().trimmed());
        settings.setValue("apiKey", m_apiKeyLine->text());
        settings.setValue("modelName", m_modelNameLine->text());
        settings.setValue("localMode", m_localModeCheckBox->isChecked());
        settings.setValue("streamingEnabled", m_streamingCheckBox->isChecked());
        settings.setValue("theme", m_themeComboBox->currentData().toInt());
        settings.setValue("language", newLanguage);

        StyleSheetManager::Theme theme = static_cast<StyleSheetManager::Theme>(m_themeComboBox->currentData().toInt());
        StyleSheetManager::instance()->setTheme(theme);

        if (oldLanguage != newLanguage) {
            QString locale;
            if (newLanguage == "system") {
                QLocale sysLocale = QLocale::system();
                locale = sysLocale.language() == QLocale::Chinese ? "zh_CN" : "en";
            } else {
                locale = newLanguage;
            }
            TranslationManager::instance()->loadTranslation(locale);
        }

        accept();
    });

    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QString SettingsDialog::getApiBaseUrl() const
{
    return m_apiUrlLine->text();
}

QString SettingsDialog::getApiKey() const
{
    return m_apiKeyLine->text();
}

QString SettingsDialog::getModelName() const
{
    return m_modelNameLine->text();
}

bool SettingsDialog::isLocalMode() const
{
    return m_localModeCheckBox->isChecked();
}

StyleSheetManager::Theme SettingsDialog::getTheme() const
{
    return static_cast<StyleSheetManager::Theme>(m_themeComboBox->currentData().toInt());
}

QString SettingsDialog::getLanguage() const
{
    return m_languageComboBox->currentData().toString();
}

bool SettingsDialog::isStreamingEnabled() const
{
    return m_streamingCheckBox->isChecked();
}
