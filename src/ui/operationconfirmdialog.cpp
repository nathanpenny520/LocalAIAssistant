#include "operationconfirmdialog.h"

#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "stylesheetmanager.h"

OperationConfirmDialog::OperationConfirmDialog(const OperationPlan& plan, QWidget* parent)
        : QDialog(parent), m_plan(plan) {
    setupUI(plan);
}

void OperationConfirmDialog::setupUI(const OperationPlan& plan) {
    setWindowTitle(tr("Confirm Command Plan"));
    setMinimumSize(560, 440);
    resize(600, 520);
    setModal(true);
    setStyleSheet(StyleSheetManager::instance()->currentStyleSheet());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // Title
    m_titleLabel = new QLabel(this);
    m_titleLabel->setText(tr("Command Plan (%1 command(s))").arg(plan.totalOperations()));
    QFont titleFont;
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    mainLayout->addWidget(m_titleLabel);

    // Separator
    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(separator);

    // Operation summary
    auto* summaryLabel = new QLabel(tr("Operation Summary:"), this);
    QFont sectionFont;
    sectionFont.setPointSize(11);
    sectionFont.setBold(true);
    summaryLabel->setFont(sectionFont);
    mainLayout->addWidget(summaryLabel);

    m_planPreview = new QTextBrowser(this);
    m_planPreview->setPlainText(plan.generateSummary());
    m_planPreview->setReadOnly(true);
    m_planPreview->setMaximumHeight(150);
    mainLayout->addWidget(m_planPreview);

    // Shell command preview
    auto* shellLabel = new QLabel(tr("Commands to execute:"), this);
    shellLabel->setFont(sectionFont);
    mainLayout->addWidget(shellLabel);

    m_shellPreview = new QTextBrowser(this);
    m_shellPreview->setPlainText(plan.generateShellPreview());
    m_shellPreview->setReadOnly(true);
    QFont monoFont(QStringLiteral("Menlo"), 11);
    monoFont.setStyleHint(QFont::Monospace);
    m_shellPreview->setFont(monoFont);
    mainLayout->addWidget(m_shellPreview);

    // Warning
    auto* warningLabel = new QLabel(tr("Commands will execute in a real terminal"), this);
    QFont warnFont;
    warnFont.setPointSize(11);
    warningLabel->setFont(warnFont);
    warningLabel->setObjectName(QStringLiteral("warningLabel"));
    mainLayout->addWidget(warningLabel);

    // Button row
    auto* btnLayout = new QHBoxLayout();

    m_modifyBtn = new QPushButton(tr("Modify Plan"), this);
    m_modifyBtn->setToolTip(tr("Return to conversation to add details"));

    m_cancelBtn = new QPushButton(tr("Cancel"), this);

    m_confirmBtn = new QPushButton(tr("Confirm Execute"), this);
    m_confirmBtn->setDefault(true);
    QFont btnFont = m_confirmBtn->font();
    btnFont.setBold(true);
    m_confirmBtn->setFont(btnFont);

    btnLayout->addWidget(m_modifyBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_confirmBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_confirmBtn, &QPushButton::clicked, this, &OperationConfirmDialog::onConfirm);
    connect(m_cancelBtn, &QPushButton::clicked, this, &OperationConfirmDialog::onCancel);
    connect(m_modifyBtn, &QPushButton::clicked, this, &OperationConfirmDialog::onModify);
}

void OperationConfirmDialog::onConfirm() {
    m_confirmed = true;
    accept();
}

void OperationConfirmDialog::onCancel() {
    m_confirmed = false;
    reject();
}

void OperationConfirmDialog::onModify() {
    m_modifyRequested = true;
    reject();
}
