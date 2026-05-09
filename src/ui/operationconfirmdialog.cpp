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

    // Path access warning area (hidden by default, populated by setPathViolations)
    m_pathWarningArea = new QWidget(this);
    m_pathWarningArea->setVisible(false);
    m_pathWarningArea->setObjectName(QStringLiteral("pathWarningArea"));
    mainLayout->addWidget(m_pathWarningArea);

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
    // If there are path violations, check none are denied
    if (!m_pathViolations.isEmpty()) {
        for (int i = 0; i < m_pathViolationResponses.size(); ++i) {
            if (m_pathViolationResponses[i] == 0) {
                // At least one path is still denied — reject confirmation
                m_confirmed = false;
                reject();
                return;
            }
        }
    }
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

void OperationConfirmDialog::setPathViolations(const QVector<PathViolation>& violations) {
    m_pathViolations = violations;
    m_pathViolationResponses.resize(violations.size());
    m_pathViolationResponses.fill(0);  // default: Deny

    if (violations.isEmpty() || !m_pathWarningArea) return;

    // Clear existing content
    QLayout* existingLayout = m_pathWarningArea->layout();
    if (existingLayout) {
        QLayoutItem* item;
        while ((item = existingLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete existingLayout;
    }

    auto* areaLayout = new QVBoxLayout(m_pathWarningArea);
    areaLayout->setContentsMargins(0, 8, 0, 4);

    auto* headerLabel = new QLabel(tr("Path Access Warning"), m_pathWarningArea);
    QFont headerFont;
    headerFont.setPointSize(11);
    headerFont.setBold(true);
    headerLabel->setFont(headerFont);
    headerLabel->setObjectName(QStringLiteral("warningLabel"));
    areaLayout->addWidget(headerLabel);

    for (int i = 0; i < violations.size(); ++i) {
        const auto& v = violations[i];

        auto* row = new QWidget(m_pathWarningArea);
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(4, 4, 4, 4);

        // Icon and path label
        QString icon = v.isWriteOp ? QStringLiteral("🔴") : QStringLiteral("🟡");
        QString labelText = v.isWriteOp && v.violationType == PathViolation::SystemPath
                                    ? tr("%1 %2 — This is a protected system directory")
                                              .arg(icon, v.path)
                                    : QStringLiteral("%1 %2").arg(icon, v.path);
        auto* label = new QLabel(labelText, row);
        label->setWordWrap(true);
        rowLayout->addWidget(label, 1);

        // Allow Once button
        auto* allowOnceBtn = new QPushButton(tr("Allow Once"), row);
        allowOnceBtn->setToolTip(tr("Allow this session only"));
        int idx = i;
        connect(allowOnceBtn, &QPushButton::clicked, this, [this, idx]() {
            m_pathViolationResponses[idx] = 1;
            // Update button states for this row
        });
        rowLayout->addWidget(allowOnceBtn);

        // Always Allow button
        auto* alwaysAllowBtn = new QPushButton(tr("Always Allow"), row);
        alwaysAllowBtn->setToolTip(tr("Permanently add to allowed paths"));
        connect(alwaysAllowBtn, &QPushButton::clicked, this, [this, idx]() {
            m_pathViolationResponses[idx] = 2;
        });
        rowLayout->addWidget(alwaysAllowBtn);

        // Deny button
        auto* denyBtn = new QPushButton(tr("Deny"), row);
        denyBtn->setDefault(true);
        connect(denyBtn, &QPushButton::clicked, this, [this, idx]() {
            m_pathViolationResponses[idx] = 0;
        });
        rowLayout->addWidget(denyBtn);

        areaLayout->addWidget(row);
    }

    m_pathWarningArea->setVisible(true);
    // Resize dialog to accommodate the new content
    resize(width(), height() + violations.size() * 50);
}

QVector<int> OperationConfirmDialog::pathViolationResponses() const {
    return m_pathViolationResponses;
}
