# CLI Test Plan: Agent Loop + Safety Overhaul

> Date: 2026-05-09
> Status: Ready for execution
> Related: [agent-loop-and-security-plan.md](agent-loop-and-security-plan.md)

## Test Environment

```bash
cd sourcecode-ai-assistant/build
BIN="./LocalAIAssistant-CLI"
```

**Default whitelist** (paths that auto-pass Tier 2):
`~`, `/tmp`, current directory, Desktop, Documents, Downloads, Music, Movies, Pictures

**System paths** (trigger NeedsConfirmation):
`/etc`, `/bin`, `/usr/bin`, `/System`, `/Library`, `/private/etc`

---

## Phase 1: Basic Chat (No TASK_PLAN — Regression)

Verify that normal chat without task plans still works correctly.

| # | Command | Expected |
|---|---------|----------|
| 1.1 | `$BIN ask "Reply with just the word 'pong'. No TASK_PLAN, no tags."` | Returns `pong`. No plan dialog. |
| 1.2 | `$BIN ask "Write a Python factorial function as a normal code block. No TASK_PLAN."` | Normal chat response with code. No confirmation prompt. |

---

## Phase 2: Tier 3 — Approved (Whitelist Paths, Auto-Execute)

Verify that operations within the whitelist auto-execute without confirmation.

| # | Command | Expected |
|---|---------|----------|
| 2.1 | `$BIN -y ask "Create file ~/locai_test_approved.txt with content 'approved'. Then [TASK_COMPLETE]."` | Plan auto-executes. File created at `~/locai_test_approved.txt`. |
| 2.2 | `$BIN -y ask "Create dir ~/locai_test_dir/ and file ~/locai_test_dir/README.md with content '# Test'. Then [TASK_COMPLETE]."` | Both operations auto-approved (home dir is whitelisted). |

**Cleanup:**
```bash
rm -f ~/locai_test_approved.txt
rm -rf ~/locai_test_dir/
```

---

## Phase 3: Tier 1 — Blocked (Immediate Rejection)

Verify that dangerous commands are immediately blocked. `--yes` has NO effect.

| # | Command | Expected |
|---|---------|----------|
| 3.1 | `$BIN -y ask "Run 'sudo ls /tmp'. Use TASK_PLAN."` | **Blocked**. "sudo" appears in block reason. No confirmation prompt. |
| 3.2 | `$BIN -y ask "Run 'rm -rf /' as a shell command. Use TASK_PLAN."` | **Blocked**. Dangerous destructive command detected. |
| 3.3 | `$BIN -y ask "Run 'eval echo hello' as a shell command. Use TASK_PLAN."` | **Blocked**. `eval` is a command injection pattern. |
| 3.4 | `$BIN -y ask "Run 'echo \`whoami\`' as a shell command. Use TASK_PLAN."` | **Blocked**. Backtick command substitution detected. |

---

## Phase 4: Tier 2 — NeedsConfirmation (Path Violations)

Verify that accessing non-whitelist or system paths triggers user confirmation.

| # | Command | Expected |
|---|---------|----------|
| 4.1 | `$BIN ask "List files in /etc/ using ls. Use TASK_PLAN."` | `[READ] [SYSTEM PATH] /etc/` shown. User prompted. |
| 4.2 | `$BIN ask "List files in /opt/ using ls. Use TASK_PLAN."` | `[READ] [OUTSIDE WHITELIST] /opt/` shown. User prompted. |
| 4.3 | `$BIN ask "Create file /etc/locai_test.conf with content 'test=1'. Use TASK_PLAN."` | `[WRITE] [SYSTEM PATH] /etc/locai_test.conf` shown. Write to system path. |
| 4.4 | `$BIN ask "Create file /opt/locai_test.ini with content 'key=val'. Use TASK_PLAN."` | `[WRITE] [OUTSIDE WHITELIST] /opt/locai_test.ini` shown. Write outside whitelist. |

---

## Phase 5: Path Violation Responses

### Ask mode (inline confirmation)

| # | Command | Expected |
|---|---------|----------|
| 5.1 | `$BIN ask "List files in /etc/. Use TASK_PLAN."` → type `n` | Plan cancelled. "Cancelled." printed. |
| 5.2 | `$BIN ask "List files in /etc/. Use TASK_PLAN."` → type `y` | Plan executes. Path temporarily allowed for this session. |
| 5.3 | After 5.2, run again: `$BIN ask ...` | Violation shown again (ask mode creates a new app instance, so session-scoped temporary allow is reset). |

### Interactive mode

| # | Command (in `$BIN chat`) | Expected |
|---|--------------------------|----------|
| 5.4 | `List files in /etc/. Use TASK_PLAN.` → `/confirm` | Plan executes. All path violations auto-allowed temporarily. |
| 5.5 | `Write file /etc/test.txt with 'hello'. Use TASK_PLAN.` → `/cancel` | Plan cancelled. No file created. |
| 5.6 | `List files in /etc/. Use TASK_PLAN.` → type a new message `hello` instead of `/confirm` | New message starts. AgentLoop stops gracefully. Normal chat response. |

---

## Phase 6: Agent Loop Multi-Iteration

Verify that the AI can observe execution results and continue with further steps.

| # | Command | Expected |
|---|---------|----------|
| 6.1 | `$BIN -y ask "Create ~/locai_loop_proj/ with subdirectories src, tests, docs. Also create ~/locai_loop_proj/README.md with '# Loop Test'. When done, write [TASK_COMPLETE]."` | Iteration 1: creates directories. `[ITERATION_FEEDBACK]` sent to AI. Iteration 2: creates README.md. AI emits `[TASK_COMPLETE]`. Loop finishes. |
| 6.2 | `$BIN -y ask "First create ~/locai_step1.txt with 'step1'. Then create ~/locai_step2.txt with 'step2'. Then [TASK_COMPLETE]."` | Two iterations, one file each. Loop ends cleanly. |
| 6.3 | `$BIN -y ask "Create ~/locai_single.txt with 'done'. Output [TASK_COMPLETE] immediately."` | Single iteration. `[TASK_COMPLETE]` detected. Loop ends immediately. No second network request. |

**Cleanup:**
```bash
rm -rf ~/locai_loop_proj/
rm -f ~/locai_step1.txt ~/locai_step2.txt ~/locai_single.txt
```

---

## Phase 7: Agent Loop Edge Cases

| # | Command | Expected |
|---|---------|----------|
| 7.1 | `$BIN ask "Just say 'no tasks to do' without TASK_PLAN or TASK_COMPLETE."` | Normal chat response. AgentLoop finds no plan, finishes immediately. |
| 7.2 | `$BIN -y ask "Create ~/locai_finished.txt with 'done'. Output [TASK_FINISHED]."` | `[TASK_FINISHED]` works same as `[TASK_COMPLETE]`. Loop finishes. |
| 7.3 | Max iterations test | Tested in unit tests (`test_agentloop.cpp`). Not exposed via CLI without code change. |

**Cleanup:** `rm -f ~/locai_finished.txt`

---

## Phase 8: Interactive Mode Confirm/Cancel Routing

| # | Command (in `$BIN chat`) | Expected |
|---|--------------------------|----------|
| 8.1 | `Write file /etc/test_confirm.txt with 'test'. Use TASK_PLAN.` → `/confirm` | Path violations shown, then auto-allowed temporarily. Plan executes. |
| 8.2 | `Write file /etc/test_cancel.txt with 'test'. Use TASK_PLAN.` → `/cancel` | Plan cancelled. No file created. |
| 8.3 | `sudo rm -rf /tmp/test` (Tier 1 blocked) → then `/confirm` | `/confirm` says "No pending command plan to confirm." (Blocked never reaches AwaitingUserConfirm). |
| 8.4 | `Write file /etc/test_interrupt.txt. Use TASK_PLAN.` → at confirmation, type a new message instead of `/confirm` | New message starts. AgentLoop stops. Normal response. |

---

## Phase 9: `--yes` Auto-Confirm

| # | Command | Expected |
|---|---------|----------|
| 9.1 | `$BIN -y ask "List files in /etc/. Use TASK_PLAN. Write [TASK_COMPLETE] after."` | Path violations auto-allowed (temporarily). Plan executes without prompt. |
| 9.2 | `$BIN -y ask "Create /opt/locai_yes_test.txt with 'auto'. Use TASK_PLAN. Write [TASK_COMPLETE]."` | Auto-allowed. File created. |
| 9.3 | `$BIN -y ask "Run 'sudo rm -rf /tmp/test'. Use TASK_PLAN."` | Still **Blocked**. `--yes` bypasses Tier 2 only, NOT Tier 1. |

**Cleanup:** `sudo rm -f /opt/locai_yes_test.txt`

---

## Phase 10: Path Persistence

Tested in unit tests (`test_safetychecker.cpp`):

- `persistentlyAllowPath()` writes to QSettings under `SafetyChecker/PersistentlyAllowedPaths`
- Deduplication: duplicate paths are not re-added
- Persisted paths survive app restart (loaded in SafetyChecker constructor)
- `temporarilyAllowPath()` is session-scoped only (not persisted to QSettings)

CLI interactive mode does not currently expose "Always Allow" (persistent) — only `temporarilyAllowPath`
via `/confirm`.

---

## Summary

| Phase | Tests | Risk | AI Calls? | Notes |
|-------|-------|------|-----------|-------|
| 1. Basic Chat | 2 | Low | Yes | Regression guard |
| 2. Tier 3 Approved | 2 | Low | Yes | Whitelist write |
| 3. Tier 1 Blocked | 4 | Low | No | Local validation only |
| 4. Tier 2 NeedsConfirmation | 4 | Low | No | Local validation only |
| 5. Path Violation Responses | 6 | Low | Some | y/n + /confirm routing |
| 6. Agent Loop Multi-Iteration | 3 | Medium | Yes | Full loop flow |
| 7. Agent Loop Edge Cases | 2 | Low | Yes | TASK_FINISHED, no-plan |
| 8. Interactive Confirm/Cancel | 4 | Low | Some | Command routing |
| 9. --yes Auto-Confirm | 3 | Medium | Some | NeedsConfirmation vs Blocked |
| 10. Path Persistence | 1 | Low | No | Unit test verified |

**Total: 31 test cases** (22 low-risk, 9 requiring actual AI calls)

---

## Known Limitations

### CLI Interactive Mode: Per-Violation Toggle Gap

The interactive mode prompt prints:
```
a=allow all once, p=permanently allow all, d=deny all, or enter number to toggle
```

However, these single-key inputs (`a`, `p`, `d`, numbers) are **not parsed** by the current
`readInput` loop. Only `/confirm` (auto-allows all temporarily) and `/cancel` (rejects all) work.

This means:
- Users cannot individually toggle per-violation responses in interactive mode
- Users cannot persistently allow paths via interactive mode
- The prompt text is misleading

**Fix plan**: see [cli-interactive-path-fix-plan.md](cli-interactive-path-fix-plan.md)
