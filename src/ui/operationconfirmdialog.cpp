#include "operationconfirmdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QFrame>
#include <QLabel>

OperationConfirmDialog::OperationConfirmDialog(const OperationPlan &plan, QWidget *parent)
    : QDialog(parent), m_plan(plan)
{
    setupUI(plan);
}

void OperationConfirmDialog::setupUI(const OperationPlan &plan)
{
    setWindowTitle(QStringLiteral("确认命令计划"));
    setMinimumSize(560, 440);
    resize(600, 520);
    setModal(true);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // 标题
    m_titleLabel = new QLabel(this);
    m_titleLabel->setText(QStringLiteral("📋 命令计划（共 %1 条命令）").arg(plan.totalOperations()));
    QFont titleFont;
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    mainLayout->addWidget(m_titleLabel);

    // 分隔线
    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(separator);

    // 操作摘要
    auto *summaryLabel = new QLabel(tr("操作摘要："), this);
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

    // Shell 命令预览
    auto *shellLabel = new QLabel(tr("将执行的命令："), this);
    shellLabel->setFont(sectionFont);
    mainLayout->addWidget(shellLabel);

    m_shellPreview = new QTextBrowser(this);
    m_shellPreview->setPlainText(plan.generateShellPreview());
    m_shellPreview->setReadOnly(true);
    QFont monoFont(QStringLiteral("Menlo"), 11);
    monoFont.setStyleHint(QFont::Monospace);
    m_shellPreview->setFont(monoFont);
    mainLayout->addWidget(m_shellPreview);

    // 警告
    auto *warningLabel = new QLabel(
        QStringLiteral("⚠️ 命令将在真实终端中执行"), this);
    QFont warnFont;
    warnFont.setPointSize(11);
    warningLabel->setFont(warnFont);
    warningLabel->setStyleSheet(QStringLiteral("color: #ff9500;"));
    mainLayout->addWidget(warningLabel);

    // 按钮行
    auto *btnLayout = new QHBoxLayout();

    m_modifyBtn = new QPushButton(QStringLiteral("修改计划"), this);
    m_modifyBtn->setToolTip(QStringLiteral("返回对话，补充说明"));

    m_cancelBtn = new QPushButton(QStringLiteral("取消"), this);

    m_confirmBtn = new QPushButton(QStringLiteral("确认执行"), this);
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

void OperationConfirmDialog::onConfirm()
{
    m_confirmed = true;
    accept();
}

void OperationConfirmDialog::onCancel()
{
    m_confirmed = false;
    reject();
}

void OperationConfirmDialog::onModify()
{
    m_modifyRequested = true;
    reject();
}
