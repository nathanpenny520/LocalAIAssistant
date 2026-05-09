#pragma once

#ifndef SAFETYCHECKER_H
#define SAFETYCHECKER_H

#include <QCoreApplication>
#include <QString>
#include <QStringList>

#include "operationplan.h"

class SafetyChecker {
    Q_DECLARE_TR_FUNCTIONS(SafetyChecker)

public:
    enum Result { Approved, NeedsConfirmation, Blocked };
    enum DangerLevel { Safe, Caution, Dangerous };

    SafetyChecker();

    void setAllowedPaths(const QStringList& paths);
    void addAllowedPath(const QString& path);
    void resetToDefaults();
    QStringList allowedPaths() const;

    Result validatePlan(const OperationPlan& plan);
    Result validateOperation(const ShellOperation& op);

    QString lastBlockReason() const;
    DangerLevel dangerLevel(const ShellOperation& op) const;

private:
    bool isPathSafe(const QString& path) const;
    bool isSystemPath(const QString& path) const;
    bool hasCommandInjection(const QString& command);
    bool isDangerousCommand(const QString& command);
    QStringList extractPathsFromCommand(const QString& command) const;

    QStringList m_allowedPaths;
    QString m_lastBlockReason;
};

#endif  // SAFETYCHECKER_H
