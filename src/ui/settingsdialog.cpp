#include "settingsdialog.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>

#include "../knowledge/embedder.h"
#include "knowledgebase.h"
#include "stylesheetmanager.h"
#include "translationmanager.h"

SettingsDialog::SettingsDialog(QWidget* parent)
        : QDialog(parent)
        , m_apiUrlLine(new QLineEdit(this))
        , m_apiKeyLine(new QLineEdit(this))
        , m_modelNameLine(new QLineEdit(this))
        , m_apiTypeComboBox(new QComboBox(this))
        , m_themeComboBox(new QComboBox(this))
        , m_languageComboBox(new QComboBox(this))
        , m_streamingCheckBox(new QCheckBox(tr("启用流式输出"), this)) {
    setWindowTitle(tr("设置"));
    setMinimumWidth(460);
    setStyleSheet(StyleSheetManager::instance()->currentStyleSheet());

    QSettings settings("LocalAIAssistant", "Settings");

    // ── Common settings loading ─────────────────────────────────
    m_apiUrlLine->setText(settings.value("apiBaseUrl", "http://127.0.0.1:8080").toString());
    m_apiKeyLine->setEchoMode(QLineEdit::Password);
    m_apiKeyLine->setText(settings.value("apiKey", "").toString());
    m_modelNameLine->setText(settings.value("modelName", "local-model").toString());
    m_streamingCheckBox->setChecked(settings.value("streamingEnabled", true).toBool());

    m_themeComboBox->addItem(tr("跟随系统"), static_cast<int>(StyleSheetManager::SystemTheme));
    m_themeComboBox->addItem(tr("亮色"), static_cast<int>(StyleSheetManager::LightTheme));
    m_themeComboBox->addItem(tr("暗色"), static_cast<int>(StyleSheetManager::DarkTheme));
    int themeValue =
            settings.value("theme", static_cast<int>(StyleSheetManager::SystemTheme)).toInt();
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
        if (locale.language() == QLocale::Chinese)
            m_languageComboBox->setCurrentIndex(0);
        else
            m_languageComboBox->setCurrentIndex(1);
    } else {
        for (int i = 0; i < m_languageComboBox->count(); ++i) {
            if (m_languageComboBox->itemData(i).toString() == savedLang) {
                m_languageComboBox->setCurrentIndex(i);
                break;
            }
        }
    }

    // API type combo box
    m_apiTypeComboBox->addItem(tr("OpenAI 兼容 (llama.cpp, vLLM 等)"),
                               static_cast<int>(ApiType::OpenAI));
    m_apiTypeComboBox->addItem(QStringLiteral("Ollama"), static_cast<int>(ApiType::Ollama));
    m_apiTypeComboBox->addItem(tr("llama.cpp (本地 OpenAI 兼容)"),
                               static_cast<int>(ApiType::LlamaCpp));
    m_apiTypeComboBox->addItem(tr("Anthropic 兼容"), static_cast<int>(ApiType::Anthropic));
    QString savedApiType = settings.value("apiType", "openai").toString().toLower();
    if (savedApiType == "ollama")
        m_apiTypeComboBox->setCurrentIndex(1);
    else if (savedApiType == "llamacpp")
        m_apiTypeComboBox->setCurrentIndex(2);
    else if (savedApiType == "anthropic")
        m_apiTypeComboBox->setCurrentIndex(3);
    else
        m_apiTypeComboBox->setCurrentIndex(0);

    // ── Tab 1: General (API + Appearance) ───────────────────────
    QGroupBox* apiGroup = new QGroupBox(tr("API 设置"), this);
    QFormLayout* apiLayout = new QFormLayout(apiGroup);
    apiLayout->addRow(tr("API 基础 URL:"), m_apiUrlLine);
    apiLayout->addRow(tr("API 密钥:"), m_apiKeyLine);
    apiLayout->addRow(tr("模型名称:"), m_modelNameLine);
    apiLayout->addRow(tr("API 类型:"), m_apiTypeComboBox);
    apiLayout->addRow(m_streamingCheckBox);

    QGroupBox* appearanceGroup = new QGroupBox(tr("外观"), this);
    QFormLayout* appearanceLayout = new QFormLayout(appearanceGroup);
    appearanceLayout->addRow(tr("主题:"), m_themeComboBox);
    appearanceLayout->addRow(tr("语言:"), m_languageComboBox);

    QWidget* generalTab = new QWidget(this);
    QVBoxLayout* generalLayout = new QVBoxLayout(generalTab);
    generalLayout->addWidget(apiGroup);
    generalLayout->addWidget(appearanceGroup);
    generalLayout->addStretch();

    // ── Tab 2: Knowledge Base ───────────────────────────────────
    KnowledgeBase* kb = KnowledgeBase::instance();
    QLabel* kbStatusLabel = new QLabel(this);
    m_kbDocList = new QListWidget(this);
    m_kbDocList->setMaximumHeight(160);
    m_kbDocList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    auto updateKbStatus = [this, kb, kbStatusLabel]() {
        int docs = kb->totalDocuments();
        int chunks = kb->totalChunks();

        // 检测嵌入模型状态
        Embedder* emb = kb->embedder();
        bool hasModel = emb && emb->isLoaded() && !Embedder::findModelPath().isEmpty();

        if (docs == 0) {
            kbStatusLabel->setText(SettingsDialog::tr("状态：未导入文档"));
        } else {
            kbStatusLabel->setText(
                    SettingsDialog::tr("状态：%1 个文档，%2 个片段").arg(docs).arg(chunks));
        }

        if (!hasModel) {
            kbStatusLabel->setText(kbStatusLabel->text() + QStringLiteral("\n") +
                                   SettingsDialog::tr("（警告：嵌入模型未找到，搜索准确性较低）"));
            kbStatusLabel->setObjectName(QStringLiteral("warningLabel"));
        } else {
            kbStatusLabel->setObjectName(QString());
        }

        m_kbDocList->clear();
        for (const QString& doc : kb->allDocuments()) m_kbDocList->addItem(doc);
    };
    updateKbStatus();

    QPushButton* importBtn = new QPushButton(tr("导入文档到知识库"), this);
    QPushButton* deleteBtn = new QPushButton(tr("删除所选文档"), this);
    deleteBtn->setEnabled(false);

    QHBoxLayout* kbBtnLayout = new QHBoxLayout();
    kbBtnLayout->addWidget(importBtn);
    kbBtnLayout->addWidget(deleteBtn);

    connect(m_kbDocList, &QListWidget::itemSelectionChanged, this, [deleteBtn, this]() {
        deleteBtn->setEnabled(!m_kbDocList->selectedItems().isEmpty());
    });

    connect(importBtn, &QPushButton::clicked, this, [this, kb, updateKbStatus, importBtn]() {
        QStringList files = QFileDialog::getOpenFileNames(this, tr("选择要导入的文档"), QString(),
                                                          tr("文档文件 (*.txt *.md *.pdf "
                                                             "*.docx);;所有文件 (*)"));
        if (files.isEmpty()) return;

        importBtn->setEnabled(false);
        importBtn->setText(tr("导入中..."));

        // 同步导入 — 避免跨线程 SQLite 访问问题
        int imported = 0;
        int failed = 0;
        for (const QString& path : files) {
            if (kb->importDocument(path))
                imported++;
            else
                failed++;
        }

        importBtn->setEnabled(true);
        updateKbStatus();

        QString msg;
        if (imported > 0 && failed == 0)
            msg = tr("成功导入 %1 个文档").arg(imported);
        else if (imported > 0)
            msg = tr("导入 %1 个成功，%2 个失败").arg(imported).arg(failed);
        else
            msg = tr("导入失败，请检查文件格式");

        QString origText = tr("导入文档到知识库");
        importBtn->setText(msg);
        QTimer::singleShot(3000, importBtn,
                           [importBtn, origText]() { importBtn->setText(origText); });
    });

    connect(deleteBtn, &QPushButton::clicked, this, [this, kb, updateKbStatus]() {
        QList<QListWidgetItem*> selected = m_kbDocList->selectedItems();
        if (selected.isEmpty()) return;

        QStringList paths;
        for (QListWidgetItem* item : selected) paths.append(item->text());

        QMessageBox confirmBox(this);
        confirmBox.setIcon(QMessageBox::Warning);
        confirmBox.setWindowTitle(tr("确认删除"));
        if (paths.size() == 1)
            confirmBox.setText(tr("确定要删除 \"%1\" 吗？").arg(paths.first()));
        else
            confirmBox.setText(tr("确定要删除 %1 个文档吗？").arg(paths.size()));
        confirmBox.setInformativeText(tr("此操作无法撤销。"));
        confirmBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
        confirmBox.button(QMessageBox::Ok)->setText(tr("删除"));
        confirmBox.setDefaultButton(QMessageBox::Cancel);
        if (confirmBox.exec() != QMessageBox::Ok) return;

        int removed = 0;
        for (const QString& path : paths) {
            if (kb->removeDocument(path)) removed++;
        }

        if (removed == 0) {
            QMessageBox::warning(this, tr("删除失败"), tr("无法删除所选文档，请重试。"));
        } else if (removed < paths.size()) {
            QMessageBox::information(this, tr("部分删除"),
                                     tr("成功删除 %1 个文档，%2 个文档删除失败。")
                                             .arg(removed)
                                             .arg(paths.size() - removed));
        }

        updateKbStatus();
    });

    QWidget* kbTab = new QWidget(this);
    QVBoxLayout* kbTabLayout = new QVBoxLayout(kbTab);
    kbTabLayout->addWidget(kbStatusLabel);
    kbTabLayout->addWidget(m_kbDocList);
    kbTabLayout->addLayout(kbBtnLayout);
    kbTabLayout->addStretch();

    // ── Tab 3: Security ─────────────────────────────────────────
    QGroupBox* pathWhitelistGroup = new QGroupBox(tr("路径白名单"), this);
    QVBoxLayout* whitelistLayout = new QVBoxLayout(pathWhitelistGroup);
    QLabel* whitelistDesc =
            new QLabel(tr("允许 AI 操作的文件路径（每行一个，留空则限制在用户主目录内）："), this);
    whitelistDesc->setWordWrap(true);
    m_pathWhitelistEdit = new QPlainTextEdit(this);
    m_pathWhitelistEdit->setMaximumHeight(120);
    m_pathWhitelistEdit->setPlaceholderText(
            tr("例如：\n/Users/username/Projects\n/Users/username/Documents"));
    QStringList savedWhitelist = settings.value("pathWhitelist").toStringList();
    m_pathWhitelistEdit->setPlainText(savedWhitelist.join(QChar::LineFeed));
    whitelistLayout->addWidget(whitelistDesc);
    whitelistLayout->addWidget(m_pathWhitelistEdit);

    QGroupBox* confirmGroup = new QGroupBox(tr("操作确认"), this);
    QVBoxLayout* confirmLayout = new QVBoxLayout(confirmGroup);
    m_confirmOpsCheckBox = new QCheckBox(tr("执行操作前显示确认对话框"), this);
    m_confirmOpsCheckBox->setChecked(settings.value("operationConfirmation", true).toBool());
    QLabel* confirmDesc =
            new QLabel(tr("关闭后，AI 将直接执行文件操作而不弹出确认对话框。建议保持开启。"), this);
    confirmDesc->setWordWrap(true);
    confirmLayout->addWidget(m_confirmOpsCheckBox);
    confirmLayout->addWidget(confirmDesc);

    QWidget* securityTab = new QWidget(this);
    QVBoxLayout* securityLayout = new QVBoxLayout(securityTab);
    securityLayout->addWidget(pathWhitelistGroup);
    securityLayout->addWidget(confirmGroup);
    securityLayout->addStretch();

    // ── Tab widget ──────────────────────────────────────────────
    QTabWidget* tabWidget = new QTabWidget(this);
    tabWidget->addTab(generalTab, tr("通用"));
    tabWidget->addTab(kbTab, tr("知识库"));
    tabWidget->addTab(securityTab, tr("安全"));

    // ── Buttons ─────────────────────────────────────────────────
    QPushButton* helpButton = new QPushButton(tr("使用帮助"), this);
    QPushButton* okButton = new QPushButton(tr("确定"), this);
    QPushButton* cancelButton = new QPushButton(tr("取消"), this);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(helpButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    connect(helpButton, &QPushButton::clicked, this, &SettingsDialog::showUsageHelp);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, [this]() {
        QSettings settings("LocalAIAssistant", "Settings");
        QString oldLanguage = settings.value("language", "system").toString();
        QString newLanguage = m_languageComboBox->currentData().toString();

        settings.setValue("apiBaseUrl", m_apiUrlLine->text().trimmed());
        settings.setValue("apiKey", m_apiKeyLine->text());
        settings.setValue("modelName", m_modelNameLine->text());
        settings.setValue(
                "apiType",
                m_apiTypeComboBox->currentData().toInt() == static_cast<int>(ApiType::Ollama) ? "ol"
                                                                                                "la"
                                                                                                "ma"
                : m_apiTypeComboBox->currentData().toInt() == static_cast<int>(ApiType::LlamaCpp)
                        ? "llamacpp"
                : m_apiTypeComboBox->currentData().toInt() == static_cast<int>(ApiType::Anthropic)
                        ? "anthropic"
                        : "openai");
        settings.setValue("streamingEnabled", m_streamingCheckBox->isChecked());
        settings.setValue("theme", m_themeComboBox->currentData().toInt());
        settings.setValue("language", newLanguage);

        // Security settings
        QStringList whitelist =
                m_pathWhitelistEdit->toPlainText().split(QChar::LineFeed, Qt::SkipEmptyParts);
        settings.setValue("pathWhitelist", whitelist);
        settings.setValue("operationConfirmation", m_confirmOpsCheckBox->isChecked());

        StyleSheetManager::Theme theme =
                static_cast<StyleSheetManager::Theme>(m_themeComboBox->currentData().toInt());
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

void SettingsDialog::showUsageHelp() {
    // Resolve doc path based on language
    QString locale;
    QSettings appSettings("LocalAIAssistant", "Settings");
    QString language = appSettings.value("language", "system").toString();
    if (language == "system") {
        QLocale sysLocale = QLocale::system();
        locale = sysLocale.language() == QLocale::Chinese ? "zh_CN" : "en";
    } else {
        locale = language;
    }

    QString docName = (locale == "zh_CN") ? "USAGE_zh_CN.md" : "USAGE.md";
    QString appDir = QApplication::applicationDirPath();

#ifdef Q_OS_MACOS
    QString docPath = QDir::cleanPath(appDir + "/../Resources/docs/" + docName);
#else
    QString docPath = QDir::cleanPath(appDir + "/docs/" + docName);
#endif
    if (!QFile::exists(docPath)) {
        docPath.clear();
    }

    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("使用帮助"));
    dialog->resize(680, 560);

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(0, 0, 0, 0);

    QTextBrowser* browser = new QTextBrowser(dialog);
    browser->setOpenExternalLinks(true);
    browser->setStyleSheet("QTextBrowser { border: none; padding: 16px 20px; font-size: 13px; }");

    if (!docPath.isEmpty()) {
        QFile file(docPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString markdown = in.readAll();
            file.close();
            browser->setMarkdown(markdown);
        } else {
            browser->setPlainText(tr("无法读取帮助文档。"));
        }
    } else {
        browser->setPlainText(tr("帮助文档未找到。请确认 docs/USAGE.md 存在。"));
    }

    layout->addWidget(browser);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(12, 8, 12, 12);
    btnLayout->addStretch();
    QPushButton* closeBtn = new QPushButton(tr("Close"), dialog);
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    dialog->exec();
    dialog->deleteLater();
}

QString SettingsDialog::getApiBaseUrl() const {
    return m_apiUrlLine->text();
}

QString SettingsDialog::getApiKey() const {
    return m_apiKeyLine->text();
}

QString SettingsDialog::getModelName() const {
    return m_modelNameLine->text();
}

StyleSheetManager::Theme SettingsDialog::getTheme() const {
    return static_cast<StyleSheetManager::Theme>(m_themeComboBox->currentData().toInt());
}

QString SettingsDialog::getLanguage() const {
    return m_languageComboBox->currentData().toString();
}

bool SettingsDialog::isStreamingEnabled() const {
    return m_streamingCheckBox->isChecked();
}

ApiType SettingsDialog::getApiType() const {
    return static_cast<ApiType>(m_apiTypeComboBox->currentData().toInt());
}

QStringList SettingsDialog::pathWhitelist() const {
    return m_pathWhitelistEdit->toPlainText().split(QChar::LineFeed, Qt::SkipEmptyParts);
}

bool SettingsDialog::operationConfirmationEnabled() const {
    return m_confirmOpsCheckBox->isChecked();
}

void SettingsDialog::setOperationConfirmationEnabled(bool enabled) {
    m_confirmOpsCheckBox->setChecked(enabled);
}
