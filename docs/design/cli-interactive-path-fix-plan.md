# Fix Plan: CLI Interactive Mode Per-Violation Path Toggle

> Status: **IMPLEMENTED** (commit `0e1a1b3`)  
> Date: 2026-05-09  
> Related: [cli-test-plan.md](cli-test-plan.md) — Known Limitations

## Problem

In CLI interactive mode (`chat`), when `AgentLoop::planRequiresConfirmation` fires, the
`onAgentLoopPlanConfirm` method prints:

```
a=allow all once, p=permanently allow all, d=deny all, or enter number to toggle
```

Then immediately prints:

```
Type /confirm to execute, /cancel to abort, or handle path violations first.
```

The single-key inputs (`a`, `p`, `d`, numbers) are **never parsed** — `readInput` only routes to
`handleCommand()` for `/`-prefixed commands or sends the input as a chat message.

## Fix: Single-Key Intercept in `readInput` (Option A)

When `m_hasPendingPlan == true` AND `m_pendingViolations` is non-empty, intercept raw
single-character inputs before routing to `handleCommand()`.

---

## Changes to `cli_application.h`

```cpp
// Pending path violation toggle state (interactive mode only)
QVector<SafetyChecker::PathViolation> m_pendingViolations;
QVector<int> m_pendingViolationResponses; // 0=Deny, 1=Allow Once, 2=Always Allow

void renderPathViolationToggles() const;
void clearPendingPlan();
```

---

## Changes to `cli_application.cpp`

### 1. `onAgentLoopPlanConfirm()` — store violations and render toggles

Replace the current interactive-mode block (lines 1130–1134):

```cpp
if (m_interactiveMode) {
    m_pendingPlan = plan;
    m_hasPendingPlan = true;
    m_pendingViolations = violations;
    m_pendingViolationResponses.resize(violations.size());
    m_pendingViolationResponses.fill(1); // Default: Allow Once

    renderPathViolationToggles();
    std::cout << "\nType /confirm to execute, /cancel to abort, "
              << "or toggle paths above." << std::endl;
}
```

### 2. `readInput()` — intercept single-key toggles before `handleCommand()`

Insert between reading the line and calling `handleCommand()`:

```cpp
// Intercept single-key path violation toggles (interactive mode only)
if (m_hasPendingPlan && !m_pendingViolations.isEmpty()) {
    QString raw = QString::fromLocal8Bit(line).trimmed();

    // Only intercept single-character inputs — longer input is a chat message
    if (raw.length() == 1) {
        QChar c = raw[0];

        if (c == QLatin1Char('a')) {
            m_pendingViolationResponses.fill(1);
            renderPathViolationToggles();
            return;
        }
        if (c == QLatin1Char('p')) {
            m_pendingViolationResponses.fill(2);
            renderPathViolationToggles();
            return;
        }
        if (c == QLatin1Char('d')) {
            m_pendingViolationResponses.fill(0);
            renderPathViolationToggles();
            return;
        }
        bool ok;
        int idx = raw.toInt(&ok);
        if (ok && idx >= 1 && idx <= m_pendingViolations.size()) {
            // Cycle: Deny(0) → Allow Once(1) → Always Allow(2) → Deny(0)
            int& resp = m_pendingViolationResponses[idx - 1];
            resp = (resp + 1) % 3;
            renderPathViolationToggles();
            return;
        }
        // Single char but not a valid toggle key — give a hint
        std::cout << "Invalid input. Use a/p/d/number, /confirm, or /cancel."
                  << std::endl;
        return;
    }
    // Longer input → treated as a normal chat message
    // Falls through to handleCommand() which will stop the pending plan
}
```

### 3. `handleCommand()` — `/confirm` applies per-violation choices

Replace the AgentLoop `/confirm` block (lines 422–430):

```cpp
} else if (cmd == "/confirm") {
    if (AgentLoop::instance()->state() == AgentLoop::AwaitingUserConfirm) {
        SafetyChecker& sc = m_taskEngine->safetyChecker();
        for (int i = 0; i < m_pendingViolations.size(); ++i) {
            const auto& v = m_pendingViolations[i];
            int resp = m_pendingViolationResponses.value(i, 1);
            if (resp == 2) {
                sc.persistentlyAllowPath(v.path);
            } else if (resp == 1) {
                sc.temporarilyAllowPath(v.path);
            }
            // resp == 0: Deny — path not allowed, plan still executes for other paths
        }
        clearPendingPlan();
        AgentLoop::instance()->confirmPlan();
    } else if (m_hasPendingPlan) {
        executeConfirmedPlan();
    } else {
        std::cout << "No pending command plan to confirm." << std::endl;
    }
```

### 4. `handleCommand()` — `/cancel` clears state

Replace the AgentLoop `/cancel` block (lines 436–445):

```cpp
} else if (cmd == "/cancel") {
    if (AgentLoop::instance()->state() == AgentLoop::AwaitingUserConfirm) {
        clearPendingPlan();
        AgentLoop::instance()->cancelPlan();
    } else if (m_hasPendingPlan) {
        clearPendingPlan();
        std::cout << "Pending command plan cancelled." << std::endl;
    } else {
        std::cout << "No pending command plan to cancel." << std::endl;
    }
```

### 5. New method `renderPathViolationToggles()`

```cpp
void CLIApplication::renderPathViolationToggles() const {
    static const char* labels[] = {"DENY", "ALLOW ONCE", "ALWAYS ALLOW"};
    std::cout << "\n*** Path access toggles ***" << std::endl;
    for (int i = 0; i < m_pendingViolations.size(); ++i) {
        const auto& v = m_pendingViolations[i];
        int resp = m_pendingViolationResponses[i];
        QString icon = v.isWriteOp ? QStringLiteral("[WRITE]") : QStringLiteral("[READ]");
        QString sysTag = v.violationType == SafetyChecker::PathViolation::SystemPath
            ? QStringLiteral(" [SYSTEM PATH]")
            : QStringLiteral(" [OUTSIDE WHITELIST]");
        std::cout << "  " << (i + 1) << ". " << icon.toStdString()
                  << sysTag.toStdString()
                  << " " << v.path.toStdString()
                  << " → " << labels[resp] << std::endl;
    }
    std::cout << "a=allow all once  p=always allow all  d=deny all  "
              << "number=toggle single" << std::endl;
}
```

### 6. New method `clearPendingPlan()`

```cpp
void CLIApplication::clearPendingPlan() {
    m_hasPendingPlan = false;
    m_pendingPlan = OperationPlan();
    m_pendingViolations.clear();
    m_pendingViolationResponses.clear();
}
```

---

## Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| `d` = all Deny, NOT cancel | `d` toggles the violation response to "Deny" — the plan is still pending. `/cancel` is the only way to abort the plan. Consistent with `a`/`p` which also only toggle state. |
| `.trimmed()` on raw input | CLI input always includes a trailing newline. `trimmed()` normalizes before comparison. |
| Single-char guard (`raw.length() == 1`) | Prevents multi-character input from being treated as a toggle. Anything longer falls through to normal chat routing (which stops the pending plan). |
| Invalid single-char hint | If the user types `x` or another single char, print a quick hint instead of silently doing nothing. |
| `clearPendingPlan()` helper | Deduplicates cleanup logic across `/confirm`, `/cancel`, and normal chat interrupt paths. |
| Default `fill(1)` = Allow Once | Safest default: path is allowed for this session but not persisted. User must explicitly choose `p` for permanent. |

---

## Files Changed

| Action | File | Lines |
|--------|------|-------|
| EDIT | `src/cli/cli_application.h` | +4 members |
| EDIT | `src/cli/cli_application.cpp` | ~70 (readInput intercept, renderPathViolationToggles, clearPendingPlan, updated /confirm and /cancel handlers) |

---

## Acceptance Criteria

1. Typing `a` → all violations set to **Allow Once**, list re-renders
2. Typing `p` → all violations set to **Always Allow**, list re-renders
3. Typing `d` → all violations set to **Deny**, list re-renders (plan NOT cancelled)
4. Typing a number (e.g., `1`) → that violation cycles Deny→Allow Once→Always Allow, list re-renders
5. Typing a non-toggle single char (e.g., `x`) → hint printed, plan still pending
6. `/confirm` → applies per-violation choices (`persistentlyAllowPath` for 2, `temporarilyAllowPath` for 1, nothing for 0), clears pending state, executes plan
7. `/cancel` → clears pending state, cancels plan
8. Typing a normal chat message → falls through, stops AgentLoop, starts new conversation
9. All pending state (`m_hasPendingPlan`, `m_pendingPlan`, `m_pendingViolations`, `m_pendingViolationResponses`) is cleared after `/confirm`, `/cancel`, or chat interrupt
