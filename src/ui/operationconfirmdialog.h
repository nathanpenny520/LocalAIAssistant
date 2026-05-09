#pragma once

#ifndef OPERATIONCONFIRMDIALOG_H
#define OPERATIONCONFIRMDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>

#include "../tasks/operationplan.h"
#include "../tasks/safetychecker.h"

class OperationConfirmDialog : public QDialog {
    Q_OBJECT

public:
    explicit OperationConfirmDialog(const OperationPlan& plan, QWidget* parent = nullptr);

    bool isConfirmed() const {
        return m_confirmed;
    }
    bool isModifyRequested() const {
        return m_modifyRequested;
    }

    void setPathViolations(const QVector<PathViolation>& violations);
    QVector<int> pathViolationResponses() const;  // 0=Deny, 1=Allow Once, 2=Always Allow

private slots:
    void onConfirm();
    void onCancel();
    void onModify();

private:
    void setupUI(const OperationPlan& plan);

    OperationPlan m_plan;
    bool m_confirmed = false;
    bool m_modifyRequested = false;

    QVector<PathViolation> m_pathViolations;
    QVector<int> m_pathViolationResponses;

    QLabel* m_titleLabel;
    QTextBrowser* m_planPreview;
    QTextBrowser* m_shellPreview;
    QPushButton* m_confirmBtn;
    QPushButton* m_cancelBtn;
    QPushButton* m_modifyBtn;
    QWidget* m_pathWarningArea = nullptr;
};

#endif  // OPERATIONCONFIRMDIALOG_H
