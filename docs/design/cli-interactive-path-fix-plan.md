# Fix Plan: CLI Interactive Mode Per-Violation Path Toggle

> Status: **DRAFT — awaiting review**
> Date: 2026-05-09
> Related: [cli-test-plan.md](cli-test-plan.md) — Known Limitations

## Problem

In CLI interactive mode (`chat`), when `AgentLoop::planRequiresConfirmation` fires, the
`onAgentLoopPlanConfirm` method in `cli_application.cpp` prints:

```
a=allow all once, p=permanently allow all, d=deny all, or enter number to toggle
```

But then immediately prints:

```
Type /confirm to execute, /cancel to abort, or handle path violations first.
```

And sets `m_pendingPlan = plan; m_hasPendingPlan = true;` before returning to the `readInput` loop.

The `readInput` loop only routes to `handleCommand()` for `/`-prefixed commands or sends the input
as a chat message. The single-key inputs (`a`, `p`, `d`, numbers) are **never parsed**.

**Current behavior** (line 422-435 of cli_application.cpp):
- `/confirm` → auto-allows ALL violations temporarily, calls `confirmPlan()`
- `/cancel` → calls `cancelPlan()`
- Any other input → treated as a new chat message, stops AgentLoop

**Missing behavior**:
- `a` → allow all once (temporarily) — this is what `/confirm` already does
- `p` → permanently allow all (write to QSettings)
- `d` → deny all (same as `/cancel`)
- Number toggle → per-violation: toggle individual violation response (0=Deny, 1=Allow Once, 2=Always Allow)

## Fix Approach: Per-Violation State Machine in Interactive Mode

### Option A: Single-key intercept in `readInput` (Recommended)

When `m_hasPendingPlan == true` AND `m_pendingViolations` is non-empty, intercept raw
single-character inputs in the `readInput` loop before routing to `handleCommand()`.

**Changes to `cli_application.h`:**

```cpp
// New member to store pending violations for interactive toggling
QVector<PathViolation> m_pendingViolations;
QVector<int> m_pendingViolationResponses; // 0=Deny, 1=Allow Once, 2=Always Allow

// Helper to render current violation toggle state
void renderPathViolationToggles() const;
```

**Changes to `cli_application.cpp`:**

In `onAgentLoopPlanConfirm()`, when in interactive mode and violations are non-empty:

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

In `readInput()` (before `handleCommand()` call):

```cpp
// Intercept single-key path violation toggles
if (m_hasPendingPlan && !m_pendingViolations.isEmpty()) {
    QString raw = QString::fromLocal8Bit(line);
    if (raw == QStringLiteral("a")) {
        m_pendingViolationResponses.fill(1); // All Allow Once
        renderPathViolationToggles();
        return;
    } else if (raw == QStringLiteral("p")) {
        m_pendingViolationResponses.fill(2); // All Always Allow
        renderPathViolationToggles();
        return;
    } else if (raw == QStringLiteral("d")) {
        AgentLoop::instance()->cancelPlan();
        return;
    } else {
        bool ok;
        int idx = raw.toInt(&ok);
        if (ok && idx >= 1 && idx <= m_pendingViolations.size()) {
            // Cycle: 0→1→2→0
            int& resp = m_pendingViolationResponses[idx - 1];
            resp = (resp + 1) % 3;
            renderPathViolationToggles();
            return;
        }
    }
}
```

In `handleCommand()` for `/confirm`:

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
            // resp == 0: deny — but AgentLoop confirmation still proceeds
            // (individual path denial is handled differently)
        }
        AgentLoop::instance()->confirmPlan();
    }
    // ... existing m_hasPendingPlan handling
}
```

**New method `renderPathViolationToggles()`:**

```cpp
void CLIApplication::renderPathViolationToggles() const {
    static const char* labels[] = {"DENY", "ALLOW ONCE", "ALWAYS ALLOW"};
    std::cout << "\n*** Path access toggles ***" << std::endl;
    for (int i = 0; i < m_pendingViolations.size(); ++i) {
        const auto& v = m_pendingViolations[i];
        int resp = m_pendingViolationResponses[i];
        QString icon = v.isWriteOp ? "[WRITE]" : "[READ]";
        QString sysTag = v.violationType == PathViolation::SystemPath
            ? "[SYSTEM PATH]" : "[OUTSIDE WHITELIST]";
        std::cout << "  " << (i + 1) << ". " << icon.toStdString()
                  << " " << sysTag.toStdString()
                  << " " << v.path.toStdString()
                  << " → " << labels[resp] << std::endl;
    }
    std::cout << "a=allow all once  p=always allow all  d=deny all  "
              << "number=toggle single" << std::endl;
}
```

### Option B: `/allow` `/deny` `/always` subcommands (Alternative)

Instead of single-key intercepts, add slash commands with optional path index:

```
> /allow 1          # Allow path #1 once (this session)
> /always 2         # Permanently allow path #2
> /deny 3           # Deny path #3
> /allow all        # Allow all once
> /deny all         # Deny all
```

More verbose but consistent with the existing `/command` paradigm. No state machine in `readInput`.

**Tradeoff**: Option B is easier to implement and test but less fluid. Option A is more user-friendly
(one keystroke toggles) but requires a more invasive change to `readInput`.

### Recommendation: Option A

The prompt already tells users to type `a`/`p`/`d`/`number` — we should make it work as documented.
Option A is the natural completion of the already-printed UI.

## Files Changed

| Action | File | Lines |
|--------|------|-------|
| EDIT | `src/cli/cli_application.h` | +3 members |
| EDIT | `src/cli/cli_application.cpp` | +60 (readInput intercept, renderPathViolationToggles, updated /confirm handler) |

## Risk Assessment

- **Risk**: Low. Only affects interactive mode when `m_hasPendingPlan && !m_pendingViolations.isEmpty()`.
- **Regression surface**: Regular chat, ask mode, and non-violation plan confirmation are unchanged.
- **Testing**: Covered by Phase 5 and Phase 8 of `cli-test-plan.md`.

## Acceptance Criteria

1. In CLI interactive mode, after a plan with path violations is shown:
   - Typing `a` toggles all violations to "Allow Once" and re-renders the list
   - Typing `p` toggles all to "Always Allow" and re-renders
   - Typing `d` cancels the plan immediately
   - Typing a number (e.g., `1`) cycles that violation through Deny→Allow Once→Always Allow
2. `/confirm` applies the per-violation choices: response=2 calls `persistentlyAllowPath()`,
   response=1 calls `temporarilyAllowPath()`, response=0 does nothing
3. `/cancel` still works and clears the pending state
4. Typing a normal chat message still stops AgentLoop and starts a new conversation
