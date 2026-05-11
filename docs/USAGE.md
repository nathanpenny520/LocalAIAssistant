# Local AI Assistant — Usage Guide

## Overview

Local AI Assistant is a cross-platform desktop AI assistant powered by Qt 6. It provides AI chat
through both GUI and CLI modes, plus an AI Girlfriend module with voice interaction.

### Key Features

- **Dual-mode**: GUI desktop app + CLI command-line interface
- **File uploads**: text, images, and PDF attachments
- **Streaming output**: real-time AI response display
- **Session management**: multiple conversations with persistent history
- **Multi-language**: Simplified Chinese / English
- **Theme switching**: Light / Dark / System
- **Knowledge base**: AI-powered document search
- **AI Girlfriend**: voice interaction with emotion system
- **Cross-platform**: macOS / Windows / Linux

---

## Getting Started

### Starting the App

**macOS:**

```bash
open build/LocalAIAssistant.app
```

**Windows:**

```bash
build\LocalAIAssistant.exe
# Debug mode (console log window):
build\LocalAIAssistant.exe --debug
```

**Linux:**

```bash
./build/LocalAIAssistant
```

> **macOS first launch**: Right-click the app → "Open" to bypass Gatekeeper (app is unsigned free
> software). **Windows SmartScreen**: Click "More info" → "Run anyway".

---

## Main Window (GUI)

### Chatting with AI

1. Type your message in the input box at the bottom
2. Press **Enter** or click **Send** to submit
3. The AI response will stream in real-time
4. You can type while the AI is responding — your message queues up

### File Attachments

Click the **📎** button to attach files before sending:

| Format                  | Notes                              |
| ----------------------- | ---------------------------------- |
| `.txt`, `.md`           | Text files                         |
| `.png`, `.jpg`, `.jpeg` | Images (model must support vision) |
| `.pdf`                  | PDF documents                      |

### Sessions

The left panel shows your conversation sessions:

- Click **+ New Session** to start a new chat
- Click any session to switch to it
- Right-click a session to rename, pin, or delete it
- Conversations auto-save as you chat

### Keyboard Shortcuts

| Shortcut       | Action                          |
| -------------- | ------------------------------- |
| `Ctrl/Cmd + N` | New session                     |
| `Ctrl/Cmd + G` | Open/close AI Girlfriend window |
| `Enter`        | Send message                    |

---

## Settings

Open settings from the menu bar or toolbar gear icon.

### General Tab

| Setting          | Description                                                                        |
| ---------------- | ---------------------------------------------------------------------------------- |
| **API Base URL** | Your AI service endpoint (e.g., `https://api.openai.com`)                          |
| **API Key**      | Your authentication key                                                            |
| **Model Name**   | AI model to use (e.g., `gpt-4o`, `claude-sonnet-4-6`)                              |
| **API Type**     | Backend: OpenAI-compatible, Ollama, llama.cpp (local OpenAI), Anthropic-compatible |
| **Streaming**    | Enable real-time token-by-token response                                           |
| **Theme**        | Light / Dark / Follow System                                                       |
| **Language**     | Interface language (Chinese / English)                                             |

### Knowledge Base Tab

See the [Knowledge Base](#knowledge-base) section below.

### Security Tab

See the [Security](#security) section below.

---

## AI Service Setup

The app requires an AI backend service. Two options:

### Option A: Cloud API (Recommended)

| Provider                | API URL                       | Notes             |
| ----------------------- | ----------------------------- | ----------------- |
| OpenAI                  | `https://api.openai.com`      | Requires API key  |
| Paratera                | `https://llmapi.paratera.com` | Chinese API proxy |
| Other OpenAI-compatible | Per-provider docs             | —                 |

Enter the **API URL**, **API Key**, and **Model Name** in Settings → General.

### Option B: Local Deployment

#### Ollama

1. Install [Ollama](https://ollama.com/download)
2. Download a model: `ollama pull llama3`
3. In Settings, set:
    - API URL: `http://127.0.0.1:11434`
    - API Type: `Ollama`
    - Model: `llama3`

#### llama.cpp

1. Download & build [llama.cpp](https://github.com/ggerganov/llama.cpp)
2. Download a GGUF model file
3. Start the server: `llama-server -m model.gguf --port 8080`
4. In Settings, set:
    - API URL: `http://127.0.0.1:8080`
    - API Type: `llama.cpp (本地 OpenAI 兼容)`
    - Model: `local-model`

> llama.cpp server uses the OpenAI-compatible protocol (`/v1/chat/completions`), default port 8080.
> The `llama.cpp` API type automatically sets local mode (HTTP, no auth).

---

## Knowledge Base

The knowledge base lets you search documents using AI-powered semantic retrieval.

### Importing Documents

1. Open Settings → **Knowledge Base** tab
2. Click **Import Documents**
3. Select `.txt`, `.md`, `.pdf`, or `.docx` files

Documents are automatically split into chunks, vectorized, and indexed.

### Searching

In the main chat, the AI automatically searches the knowledge base for relevant context when
answering questions.

### Requirements

- **Without ONNX Runtime**: Uses placeholder vectors (search still works, reduced accuracy)
- **With ONNX Runtime**: Real embedding inference for accurate semantic search

---

## AI Girlfriend Module

Open the AI Girlfriend window via **Ctrl/Cmd + G** or from the menu.

### Avatar Levels

| Level            | Type       | Description           |
| ---------------- | ---------- | --------------------- |
| Level 1 (Belle)  | Static PNG | Classic style         |
| Level 2 (Hot)    | Static PNG | Hotter style          |
| Level 3 (Hotter) | MP4 Video  | Animated, comes alive |

> **Level 3 video mode**: Chat input is intentionally hidden — video mode is for enjoying the visual
> experience. The settings button, emotion label, and mood bar remain visible on top of the video.
> This is by design, not a bug.

### Emotion System

The avatar expresses **14 different emotions** in real-time based on conversation: Happy, Shy, Love,
Angry, Sad, Crying, Afraid, Worried, Dislike, Expecting, Speaking, Thinking, Default, Travelling.

Emotions are selected based on keyword detection in your messages. When no specific keywords match,
the avatar picks a **random emotion** from a mood-appropriate pool — so the same conversation won't
always look the same.

**Idle cycling**: When you haven't sent a message for 30 seconds, the avatar gently cycles through
Default, Thinking, and Expecting emotions (every 5 seconds). Sending a new message immediately stops
the idle cycling.

### Mood System

- **Mood bar**: displayed at the top-left with a percentage. Color transitions from dim gray (low
  mood) → neutral gray → warm pink → bright pink (high mood) — the happier, the brighter.
- **Mood influence**: configurable as Low / Medium / High — controls how much emotion detection
  affects mood
- Mood evolves naturally through conversation

### Settings Menu (⚙️)

Open from the top-right of the girlfriend window:

| Menu Item              | Description                            |
| ---------------------- | -------------------------------------- |
| **Voice Output**       | Toggle text-to-speech on/off           |
| **Configure Voice...** | Set Xunfei (iFlytek) voice credentials |
| **Clear History**      | Delete current session messages        |

### Voice Interaction

The AI Girlfriend supports voice input (speech-to-text) and voice output (text-to-speech) via Xunfei
(iFlytek) WebSocket API.

#### Setting Up Voice

1. Register at [Xunfei Open Platform](https://www.xfyun.cn)
2. Create an app and enable services:
    - **Speech Recognition** (WebSocket streaming version)
    - **Speech Synthesis** (Ultra-realistic version)
3. Get your credentials: **APP ID**, **API Key**, **API Secret**
4. In the girlfriend window, click ⚙️ → **Configure Voice...** and fill in the credentials
5. APP ID, API Key, and API Secret are required; URL and Voice Type are optional (defaults provided)

#### Voice Input

Click the **microphone button** in the girlfriend window to start speaking. Your voice is
transcribed and sent as a text message.

#### Voice Output

When enabled, the AI's text replies are automatically synthesized into speech and played through
your speakers.

> **Platform support**:
>
> - **macOS**: Voice input + output ✅
> - **Windows**: Voice output only (TTS); voice input not yet supported ⚠️
> - **Linux**: Voice output only (TTS); voice input untested

### Customizing Personality

Edit `personality.md` to customize the AI girlfriend's personality, speaking style, and behavior.
The file supports template variables (auto-replaced by the app):

| Variable            | Description                                   |
| ------------------- | --------------------------------------------- |
| `{{user_nickname}}` | User's nickname, defaults to "你"             |
| `{{mood_hint}}`     | Auto-filled mood hint based on mood value     |
| `{{time_context}}`  | Auto-filled time-of-day context               |
| `{{user_memories}}` | Injected user memory archive from `memory.md` |

The `<!-- CONFIG_START -->` block at the bottom of the file allows customizing mood and time hint
text in `key=value` format:

| Config Key       | Trigger    | Example Default           |
| ---------------- | ---------- | ------------------------- |
| `mood_low`       | mood < 0.3 | 心情很差，说话带着哭腔... |
| `mood_mid`       | mood < 0.5 | 有点不开心，说话简短...   |
| `mood_high`      | mood > 0.8 | 开开心心，语气特别甜...   |
| `time_morning`   | 6-10 AM    | 早上%1点，用户刚起床...   |
| `time_noon`      | 10 AM-2 PM | 中午%1点，该吃午饭了      |
| `time_evening`   | 6-10 PM    | 晚上%1点，用户可能在休息  |
| `time_night`     | 10 PM-6 AM | 深夜%1点，用户该睡觉了... |
| `time_afternoon` | 2-6 PM     | 下午%1点                  |

Edit the text after `=` to customize hints. `%1` is replaced with the current hour. If the entire
CONFIG block is removed, built-in defaults are used.

### Memory System

The AI girlfriend remembers information about you through a persistent memory system. Memories are
stored in `memory.md` and referenced across sessions.

---

## CLI Mode

Run the command-line interface:

```bash
# macOS / Linux
./build/LocalAIAssistant-CLI

# Windows (Git Bash)
./build/LocalAIAssistant-CLI.exe
```

### CLI Commands

| Command        | Function                  |
| -------------- | ------------------------- |
| `/help`        | Show help                 |
| `/new`         | New session               |
| `/list`        | List all sessions         |
| `/switch <id>` | Switch session            |
| `/delete <id>` | Delete session            |
| `/config`      | Show current config       |
| `/file <path>` | Attach a file             |
| `/listfiles`   | List attached files       |
| `/clearfiles`  | Clear file list           |
| `/confirm`     | Confirm pending task plan |
| `/cancel`      | Cancel pending task plan  |
| `/undo`        | Undo last operation       |
| `/exit`        | Exit program              |

### One-shot Queries

```bash
# Single question
./build/LocalAIAssistant-CLI ask "What is artificial intelligence?"

# Session management
./build/LocalAIAssistant-CLI sessions -l    # list
./build/LocalAIAssistant-CLI sessions -n    # new

# Config
./build/LocalAIAssistant-CLI config --show-config
./build/LocalAIAssistant-CLI config --api-url "http://127.0.0.1:11434"
```

### Task Execution & Agent Iteration Loop

The AI can autonomously execute multi-step tasks using a feedback loop:

```
User asks → AI generates [TASK_PLAN] → plan executed → [ITERATION_FEEDBACK] sent back
    → AI inspects results → if more work needed: new [TASK_PLAN]
                        → if done: [TASK_COMPLETE]
```

This repeats until the AI declares `[TASK_COMPLETE]` or the iteration limit is reached (default: 10).

**Tags the AI uses:**

| Tag | Purpose |
|-----|---------|
| `[TASK_PLAN]...[/TASK_PLAN]` | Issue a command plan in JSON format |
| `[TASK_COMPLETE]` | Signal that the task is fully done |
| `[TASK_FINISHED]` | Alias for `[TASK_COMPLETE]` |
| `[ITERATION_FEEDBACK]` | Injected by the app — shows previous execution results |

#### TASK_PLAN JSON Format

The AI generates task plans as JSON blocks. The app supports both text-based and JSON-based formats,
automatically detecting and parsing either one. Newer models produce JSON by default:

```json
{
    "description": "Brief description of what you're doing",
    "requiresConfirmation": false,
    "operations": [
        {
            "type": "create_dir",
            "target": "~/myproject/src",
            "description": "Create source directory"
        },
        {
            "type": "write_file",
            "target": "~/myproject/README.md",
            "command": "# My Project\n\nProject description here",
            "description": "Write README with project description"
        }
    ]
}
```

**Available operation types:**

| Type | Purpose | Key Fields |
|------|---------|------------|
| `create_dir` | Create directory (cross-platform) | `target` |
| `write_file` | Create or overwrite a file | `target`, `command` (content) |
| `move_file` | Move or rename file/directory | `source`, `target` |
| `copy_file` | Recursively copy file/directory | `source`, `target` |
| `delete_file` | Delete file or directory | `source` |
| `search_files` | Search files by pattern | `source`, `command` (glob) |
| `shell_command` | Run a shell command | `command`, `workingDir` (optional) |
| `shell_script` | Run a multi-line script | `command` |

> Prefer native types (`create_dir`, `write_file`, etc.) — they are cross-platform, safer, and
> don't require a shell. Use `shell_command` only for tools like `git`, `npm`, `brew`, etc.

#### Interactive Mode (Multi-Iteration)

Real output from testing a project scaffolding request:

```
> Create ~/myproject with subdirs src, tests, docs and a README

*** Command plan requires confirmation ***
Description: Create project directory structure and README
Commands (3):
  1. create_dir → ~/myproject/src
  2. create_dir → ~/myproject/tests
  3. create_dir → ~/myproject/docs

Type /confirm to execute, /cancel to abort.
> /confirm
Executing...
  [1] OK  (5ms)
  [2] OK  (3ms)
  [3] OK  (4ms)

--- Command plan finished: 3/3 succeeded ---

# AI inspects results, decides more work is needed, generates new plan:
*** Command plan requires confirmation ***
Description: Create README.md
Commands (1):
  1. write_file → ~/myproject/README.md

> /confirm
Executing...
  [1] OK  (2ms)

--- Command plan finished: 1/1 succeeded ---

[TASK_COMPLETE] Project scaffold created with src, tests, docs and README.
--- Task completed ---
```

#### Ask Mode (One-shot, with confirmation)

```bash
$ ./LocalAIAssistant-CLI ask "Create ~/test/hello.txt"

*** Command plan requires confirmation ***
Commands (1):
  1. write_file → ~/test/hello.txt

Execute? [Y/n]: y
Executing...

[TASK_COMPLETE] File created.
--- Task completed ---
```

#### Ask Mode with `--yes` (Scripting)

```bash
$ ./LocalAIAssistant-CLI ask --yes "Create ~/test/hello.txt"

Auto-confirming (--yes)...
Executing...
[TASK_COMPLETE] File created.
```

#### Performance Notes

- **Enable streaming mode** in Settings for responsive CLI output. Non-streaming mode causes the
  entire AI response to arrive at once, which can take 30-60 seconds before any output appears.
- **Task execution time** varies by model — some models generate TASK_PLANs faster than others.
- **First iteration** includes the initial API call (30-60s). Subsequent iterations within the same
  Agent Loop reuse the conversation context with shorter API calls.
- Some AI models have their own safety layer and may refuse to generate commands like `sudo` —
  this provides defense in depth on top of the app's own SafetyChecker.

> **Important**: `--yes` only bypasses Tier 2 (path confirmation). Tier 1 (dangerous commands like
> `sudo`, `rm -rf`, `eval`) is **always blocked**, even with `--yes`.

---

## Security

### Three-Tier Safety Architecture

The SafetyChecker validates every file operation and shell command before execution:

| Tier | Name | Behavior | Examples | `--yes` effect |
|------|------|----------|----------|----------------|
| **Tier 1** | Blocked | Immediately rejected with a specific reason message. No user override possible. | `sudo`, `rm -rf`, `eval`, backtick injection, `cmd /c` | **No effect** — Tier 1 is never bypassed |
| **Tier 2** | Needs Confirmation | User must explicitly approve. Options: Allow Once / Always Allow / Deny | System paths (`/etc`, `C:\Windows`), paths outside whitelist | **Auto-confirms** — all violations temporarily allowed |
| **Tier 3** | Approved | Auto-executes. No prompt needed. | `~/`, `/tmp`, Desktop, Documents, current directory | N/A — already auto-approved |

**Tier 1 block reason messages** (actual output, language depends on app locale setting):

| Pattern Detected | Block Reason (Chinese locale) |
|------------------|------------------------------|
| Command injection (eval, backticks, etc.) | `检测到潜在的命令注入` |
| `sudo` prefix | `禁止使用 sudo 提权` |
| `rm -rf /` or `rm -rf /*` | `禁止递归删除根目录或系统目录` |
| `runas` or privilege escalation | `禁止使用提权命令` |
| Disk operations (`dd`, `format`) | `禁止磁盘操作命令` |
| System service manipulation | `禁止操作系统服务` |
| Forced shutdown/reboot | `禁止强制关机/重启` |
| Firewall disabling | `禁止关闭防火墙` |

> Some AI models also refuse to generate dangerous commands at their own safety layer. This provides
> defense in depth: even if the AI generates a command, the SafetyChecker blocks it; conversely, if
> the SafetyChecker had a bug, the model's own refusal would prevent execution.

### Path Violation Responses

When the AI tries to access a path outside the whitelist, the app shows detailed warnings.

**Operation type labels:**
- `[READ]` — read-only operations (`ls`, `cat`, `grep`, `dir`, `find`) — shown in yellow
- `[WRITE]` — write operations (`touch`, `mkdir`, `rm`, file creation) — shown in red

**Path category labels:**
- `[SYSTEM PATH]` — protected system directories (`/etc`, `/bin`, `C:\Windows`, etc.)
- `[OUTSIDE WHITELIST]` — paths not in the default whitelist or user-approved list

#### CLI ask mode (`ask` command)

```
*** Path access warnings ***
  1. [READ] [SYSTEM PATH] /etc/
  2. [WRITE] [OUTSIDE WHITELIST] /opt/config.ini
Execute? [Y/n]: y
```

- `y` or Enter — allow all violations for this session, execute plan
- `n` — cancel the plan. Prints "Cancelled."

#### CLI interactive mode (`chat` command) — per-violation toggling

```
*** Path access toggles ***
  1. [READ] [SYSTEM PATH] /etc/ → ALLOW ONCE
  2. [WRITE] [OUTSIDE WHITELIST] /opt/config.ini → ALLOW ONCE
a=allow all once  p=always allow all  d=deny all  number=toggle single

Type /confirm to execute, /cancel to abort.
```

| Key | Action |
|-----|--------|
| `a` | Set all to **Allow Once** (session-scoped, list re-renders) |
| `p` | Set all to **Always Allow** (persisted to QSettings, list re-renders) |
| `d` | Set all to **Deny** (list re-renders, plan still pending — not cancelled) |
| `1`–`9` | Cycle single violation: Deny → Allow Once → Always Allow (list re-renders) |
| `/confirm` | Apply per-violation choices and execute plan |
| `/cancel` | Cancel plan and clear pending state |
| (chat message) | Multi-character input falls through — stops pending plan, starts new conversation |

Invalid single-character inputs (like `x`) print: `"Invalid input. Use a/p/d/number, /confirm, or /cancel."` — the plan remains pending.

#### GUI confirmation dialog

Shows each path violation with individual `[Allow Once]` / `[Always Allow]` / `[Deny]` buttons.
Yellow icon for read operations, red for write operations on system paths.

### Path Allow Modes

| Mode | Scope | Persists? |
|------|-------|-----------|
| **Allow Once** | Current app session | No — resets on restart |
| **Always Allow** | Saved to QSettings | Yes — survives restarts |

### Operation Confirmation

**All task plans require user confirmation before execution.** In CLI interactive mode, type
`/confirm` or `/cancel`. In CLI ask mode, respond to the inline `[Y/n]` prompt. In GUI, a
confirmation dialog is shown. Use the `--yes` flag to auto-confirm Tier 2 warnings in CLI ask mode
for scripting. Tier 1 (dangerous commands) is never bypassed by `--yes`.

### Resetting Persistent Path Approvals

When you choose "Always Allow" for a path, it is saved to QSettings and survives app restarts.
To reset:

- **GUI**: Open Settings → Security tab, find the path in the list and remove it
- **CLI / Manual**: Delete the `SafetyChecker/PersistentlyAllowedPaths` key from QSettings:
  - macOS: `~/Library/Preferences/LocalAIAssistant.plist` (NativeFormat) or
    `~/.config/LocalAIAssistant/Settings.conf` (IniFormat)
  - Windows: Registry under `HKEY_CURRENT_USER\Software\LocalAIAssistant\Settings`
  - Linux: `~/.config/LocalAIAssistant/Settings.conf`

---

## Themes

Three themes available from Settings → General:

- **Light**: clean white interface
- **Dark**: eye-friendly dark mode
- **System**: follows your OS preference automatically

---

## Troubleshooting

### App won't open on macOS

Right-click the app → **Open**. This is needed only the first time.

### No response from AI

Check your API URL, API Key, and network connection in Settings.

### Non-streaming mode causes slow CLI output

If the CLI appears to hang during a TASK_PLAN request, check if **streaming** is enabled in
Settings → General. In non-streaming mode, the AI's full response arrives at once, which can take
30-60 seconds with no visible progress. Enable streaming for responsive, real-time output.

### Pipe-based CLI testing limitations

Using shell pipes with the CLI (e.g., `echo y | ./LocalAIAssistant-CLI ask "..."` ) may not work
reliably for task execution:
- After confirmation, the app enters an Agent Loop and stdin behavior can conflict with the pipe
- Use the `--yes` flag instead for automated scripting: `./LocalAIAssistant-CLI ask --yes "..."`
- Interactive mode (`chat`) cannot be tested via pipes — use a real terminal

### AI generates text about doing something but no TASK_PLAN

Some models may describe what they plan to do in prose without generating an actual `[TASK_PLAN]`
JSON block. If you see the AI saying "I'll create the file..." but nothing executes:
- Try being more explicit: "Use [TASK_PLAN] to create the file..."
- Some models are better at TASK_PLAN generation than others

### AI refuses to generate dangerous commands

Some AI models (particularly newer ones) have their own safety layer and will refuse to generate
commands like `sudo`, even if you explicitly request them. This is expected behavior and provides
defense in depth — use this as a safety feature, not a bug.

### Voice not working

1. Verify Xunfei credentials in ⚙️ → **Configure Voice...**
2. Ensure the services are enabled in your Xunfei account
3. Check your network can reach Xunfei servers
