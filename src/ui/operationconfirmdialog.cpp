#include "operationconfirmdialog.h"

#include <QButtonGroup>
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
    m_shellPreview->setMaximumHeight(120);
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

    // Error label (hidden, shown when confirming with denied paths)
    m_errorLabel = new QLabel(this);
    m_errorLabel->setVisible(false);
    m_errorLabel->setObjectName(QStringLiteral("errorLabel"));
    m_errorLabel->setStyleSheet(QStringLiteral(
            "QLabel#errorLabel { color: #e53935; font-weight: bold; padding: 8px; "
            "background: #ffebee; border: 1px solid #e53935; border-radius: 4px; }"));
    mainLayout->addWidget(m_errorLabel);

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
    // If there are path violations, check none are still Deny (0)
    if (!m_pathViolations.isEmpty()) {
        QStringList denied;
        for (int i = 0; i < m_pathViolationResponses.size(); ++i) {
            if (m_pathViolationResponses[i] == 0) {
                denied << m_pathViolations[i].path;
            }
        }
        if (!denied.isEmpty()) {
            m_errorLabel->setText(
                    tr("⚠ The following paths are still denied. "
                       "Click \"Allow Once\" or \"Always Allow\" for each, then confirm:\n%1")
                            .arg(denied.join(QStringLiteral(", "))));
            m_errorLabel->setVisible(true);
            return;
        }
    }
    m_errorLabel->setVisible(false);
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

    // Stylesheet for checked state on per-path buttons
    static const char* kRowBtnStyle =
            "QPushButton:checked {"
            "  background-color: #1976D2;"
            "  color: white;"
            "  font-weight: bold;"
            "  border: 2px solid #0D47A1;"
            "}";

    for (int i = 0; i < violations.size(); ++i) {
        const auto& v = violations[i];

        auto* row = new QWidget(m_pathWarningArea);
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(4, 4, 4, 4);

        // Color-coded label: red for write ops, orange for read ops
        QString color = v.isWriteOp ? QStringLiteral("red") : QStringLiteral("orange");
        QString opType = v.isWriteOp ? tr("Write") : tr("Read");
        QString labelText = v.isWriteOp && v.violationType == PathViolation::SystemPath
                                    ? tr("<font color='%1'>[%2]</font> %3 — This is a protected "
                                         "system directory")
                                              .arg(color, opType, v.path)
                                    : tr("<font color='%1'>[%2]</font> %3")
                                              .arg(color, opType, v.path);
        auto* label = new QLabel(labelText, row);
        label->setWordWrap(true);
        rowLayout->addWidget(label, 1);

        // Per-row button group for mutual exclusion
        auto* btnGroup = new QButtonGroup(row);
        btnGroup->setExclusive(true);
        int idx = i;

        auto makeBtn = [&](const QString& text, const QString& tooltip, int responseValue,
                           bool isDefault) -> QPushButton* {
            auto* btn = new QPushButton(text, row);
            btn->setToolTip(tooltip);
            btn->setCheckable(true);
            btn->setStyleSheet(QString::fromLatin1(kRowBtnStyle));
            if (isDefault) btn->setChecked(true);
            return btn;
        };

        auto* allowOnceBtn = makeBtn(tr("Allow Once"), tr("Allow this session only"), 1, false);
        auto* alwaysAllowBtn =
                makeBtn(tr("Always Allow"), tr("Permanently add to allowed paths"), 2, false);
        auto* denyBtn = makeBtn(tr("Deny"), tr("Block this path"), 0, true);

        btnGroup->addButton(allowOnceBtn, 1);
        btnGroup->addButton(alwaysAllowBtn, 2);
        btnGroup->addButton(denyBtn, 0);

        connect(btnGroup, &QButtonGroup::idClicked, this, [this, idx](int id) {
            m_pathViolationResponses[idx] = id;
            // Auto-hide error when all paths have been decided
            if (m_errorLabel && m_errorLabel->isVisible()) {
                bool allDecided = true;
                for (int resp : m_pathViolationResponses) {
                    if (resp == 0) {
                        allDecided = false;
                        break;
                    }
                }
                if (allDecided) m_errorLabel->setVisible(false);
            }
        });

        rowLayout->addWidget(allowOnceBtn);
        rowLayout->addWidget(alwaysAllowBtn);
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
