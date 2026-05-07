#pragma once

#ifndef OPERATIONCONFIRMDIALOG_H
#define OPERATIONCONFIRMDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QTextBrowser>
#include <QPushButton>
#include "../tasks/operationplan.h"

class OperationConfirmDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OperationConfirmDialog(const OperationPlan &plan, QWidget *parent = nullptr);

    bool isConfirmed() const { return m_confirmed; }
    bool isModifyRequested() const { return m_modifyRequested; }

private slots:
    void onConfirm();
    void onCancel();
    void onModify();

private:
    void setupUI(const OperationPlan &plan);

    OperationPlan m_plan;
    bool m_confirmed = false;
    bool m_modifyRequested = false;

    QLabel *m_titleLabel;
    QTextBrowser *m_planPreview;
    QTextBrowser *m_shellPreview;
    QPushButton *m_confirmBtn;
    QPushButton *m_cancelBtn;
    QPushButton *m_modifyBtn;
};

#endif // OPERATIONCONFIRMDIALOG_H
