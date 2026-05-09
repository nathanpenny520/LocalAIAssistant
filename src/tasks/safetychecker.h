#pragma once

#ifndef SAFETYCHECKER_H
#define SAFETYCHECKER_H

#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QVector>

#include "operationplan.h"

struct PathViolation {
    QString path;
    enum Type { OutsideWhitelist, SystemPath };
    Type violationType;
    bool isWriteOp;
};

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

    QVector<PathViolation> lastPathViolations() const;
    void temporarilyAllowPath(const QString& path);
    void persistentlyAllowPath(const QString& path);
    QStringList persistentlyAllowedPaths() const;
    static bool isReadOnlyCommand(const QString& command);

private:
    bool isPathSafe(const QString& path) const;
    bool isSystemPath(const QString& path) const;
    bool hasCommandInjection(const QString& command);
    bool isDangerousCommand(const QString& command);
    QStringList extractPathsFromCommand(const QString& command) const;
    void loadPersistentlyAllowedPaths();

    QStringList m_allowedPaths;
    QString m_lastBlockReason;
    QVector<PathViolation> m_lastPathViolations;
    QStringList m_temporaryAllowedPaths;
};

#endif  // SAFETYCHECKER_H
