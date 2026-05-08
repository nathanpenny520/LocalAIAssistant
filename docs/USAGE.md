# Local AI Assistant — Usage Guide

## Overview

Local AI Assistant is a cross-platform desktop AI assistant powered by Qt 6. It provides AI chat through both GUI and CLI modes, plus an AI Girlfriend module with voice interaction.

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

> **macOS first launch**: Right-click the app → "Open" to bypass Gatekeeper (app is unsigned free software).
> **Windows SmartScreen**: Click "More info" → "Run anyway".

---

## Main Window (GUI)

### Chatting with AI

1. Type your message in the input box at the bottom
2. Press **Enter** or click **Send** to submit
3. The AI response will stream in real-time
4. You can type while the AI is responding — your message queues up

### File Attachments

Click the **📎** button to attach files before sending:

| Format | Notes |
|--------|-------|
| `.txt`, `.md` | Text files |
| `.png`, `.jpg`, `.jpeg` | Images (model must support vision) |
| `.pdf` | PDF documents |

### Sessions

The left panel shows your conversation sessions:
- Click **+ New Session** to start a new chat
- Click any session to switch to it
- Right-click a session to delete it
- Conversations auto-save as you chat

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl/Cmd + N` | New session |
| `Ctrl/Cmd + G` | Open/close AI Girlfriend window |
| `Enter` | Send message |

---

## Settings

Open settings from the menu bar or toolbar gear icon.

### General Tab

| Setting | Description |
|---------|-------------|
| **API Base URL** | Your AI service endpoint (e.g., `https://api.openai.com`) |
| **API Key** | Your authentication key |
| **Model Name** | AI model to use (e.g., `gpt-4o`, `claude-sonnet-4-6`) |
| **API Type** | Backend: OpenAI-compatible, Ollama, llama.cpp (local OpenAI), Anthropic-compatible |
| **Streaming** | Enable real-time token-by-token response |
| **Theme** | Light / Dark / Follow System |
| **Language** | Interface language (Chinese / English) |

### Knowledge Base Tab

See the [Knowledge Base](#knowledge-base) section below.

### Security Tab

See the [Security](#security) section below.

---

## AI Service Setup

The app requires an AI backend service. Two options:

### Option A: Cloud API (Recommended)

| Provider | API URL | Notes |
|----------|---------|-------|
| OpenAI | `https://api.openai.com` | Requires API key |
| Paratera | `https://llmapi.paratera.com` | Chinese API proxy |
| Other OpenAI-compatible | Per-provider docs | — |

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

> llama.cpp server uses the OpenAI-compatible protocol (`/v1/chat/completions`), default port 8080. The `llama.cpp` API type automatically sets local mode (HTTP, no auth).

---

## Knowledge Base

The knowledge base lets you search documents using AI-powered semantic retrieval.

### Importing Documents

1. Open Settings → **Knowledge Base** tab
2. Click **Import Documents**
3. Select `.txt`, `.md`, `.pdf`, or `.docx` files

Documents are automatically split into chunks, vectorized, and indexed.

### Searching

In the main chat, the AI automatically searches the knowledge base for relevant context when answering questions.

### Requirements

- **Without ONNX Runtime**: Uses placeholder vectors (search still works, reduced accuracy)
- **With ONNX Runtime**: Real embedding inference for accurate semantic search

---

## AI Girlfriend Module

Open the AI Girlfriend window via **Ctrl/Cmd + G** or from the menu.

### Avatar Levels

| Level | Type | Description |
|-------|------|-------------|
| Level 1 (Belle) | Static PNG | Classic style |
| Level 2 (Hot) | Static PNG | Hotter style |
| Level 3 (Hotter) | MP4 Video | Animated, comes alive |

> **Level 3 video mode**: Chat input is intentionally hidden — video mode is for enjoying the visual experience. The settings button, emotion label, and mood bar remain visible on top of the video. This is by design, not a bug.

### Emotion System

The avatar expresses **14 different emotions** in real-time based on conversation:
Happy, Shy, Love, Angry, Sad, Crying, Afraid, Worried, Dislike, Expecting, Speaking, Thinking, Default, Travelling.

Emotions are selected based on keyword detection in your messages. When no specific keywords match, the avatar picks a **random emotion** from a mood-appropriate pool — so the same conversation won't always look the same.

**Idle cycling**: When you haven't sent a message for 30 seconds, the avatar gently cycles through Default, Thinking, and Expecting emotions (every 5 seconds). Sending a new message immediately stops the idle cycling.

### Mood System

- **Mood bar**: displayed at the top-left with a percentage. Color transitions from dim gray (low mood) → neutral gray → warm pink → bright pink (high mood) — the happier, the brighter.
- **Mood influence**: configurable as Low / Medium / High — controls how much emotion detection affects mood
- Mood evolves naturally through conversation

### Settings Menu (⚙️)

Open from the top-right of the girlfriend window:

| Menu Item | Description |
|-----------|-------------|
| **Voice Output** | Toggle text-to-speech on/off |
| **Configure Voice...** | Set Xunfei (iFlytek) voice credentials |
| **Clear History** | Delete current session messages |

### Voice Interaction

The AI Girlfriend supports voice input (speech-to-text) and voice output (text-to-speech) via Xunfei (iFlytek) WebSocket API.

#### Setting Up Voice

1. Register at [Xunfei Open Platform](https://www.xfyun.cn)
2. Create an app and enable services:
   - **Speech Recognition** (WebSocket streaming version)
   - **Speech Synthesis** (Ultra-realistic version)
3. Get your credentials: **APP ID**, **API Key**, **API Secret**
4. In the girlfriend window, click ⚙️ → **Configure Voice...** and fill in the credentials
5. APP ID, API Key, and API Secret are required; URL and Voice Type are optional (defaults provided)

#### Voice Input

Click the **microphone button** in the girlfriend window to start speaking. Your voice is transcribed and sent as a text message.

#### Voice Output

When enabled, the AI's text replies are automatically synthesized into speech and played through your speakers.

> **Platform support**:
> - **macOS**: Voice input + output ✅
> - **Windows**: Voice output only (TTS); voice input not yet supported ⚠️
> - **Linux**: Voice output only (TTS); voice input untested

### Customizing Personality

Edit `personality.md` to customize the AI girlfriend's personality, speaking style, and behavior. The file supports template variables (auto-replaced by the app):

| Variable | Description |
|----------|-------------|
| `{{user_nickname}}` | User's nickname, defaults to "你" |
| `{{mood_hint}}` | Auto-filled mood hint based on mood value |
| `{{time_context}}` | Auto-filled time-of-day context |
| `{{user_memories}}` | Injected user memory archive from `memory.md` |

The `<!-- CONFIG_START -->` block at the bottom of the file allows customizing mood and time hint text in `key=value` format:

| Config Key | Trigger | Example Default |
|-----------|---------|-----------------|
| `mood_low` | mood < 0.3 | 心情很差，说话带着哭腔... |
| `mood_mid` | mood < 0.5 | 有点不开心，说话简短... |
| `mood_high` | mood > 0.8 | 开开心心，语气特别甜... |
| `time_morning` | 6-10 AM | 早上%1点，用户刚起床... |
| `time_noon` | 10 AM-2 PM | 中午%1点，该吃午饭了 |
| `time_evening` | 6-10 PM | 晚上%1点，用户可能在休息 |
| `time_night` | 10 PM-6 AM | 深夜%1点，用户该睡觉了... |
| `time_afternoon` | 2-6 PM | 下午%1点 |

Edit the text after `=` to customize hints. `%1` is replaced with the current hour. If the entire CONFIG block is removed, built-in defaults are used.

### Memory System

The AI girlfriend remembers information about you through a persistent memory system. Memories are stored in `memory.md` and referenced across sessions.

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

| Command | Function |
|---------|----------|
| `/help` | Show help |
| `/new` | New session |
| `/list` | List all sessions |
| `/switch <id>` | Switch session |
| `/delete <id>` | Delete session |
| `/config` | Show current config |
| `/file <path>` | Attach a file |
| `/listfiles` | List attached files |
| `/clearfiles` | Clear file list |
| `/exit` | Exit program |

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

---

## Security

### Path Whitelist

In Settings → **Security**, you can configure a path whitelist. When the AI performs file operations, it is restricted to these directories.

### Operation Confirmation

Enable **Operation Confirmation** in Security settings to require your approval before the AI executes high-risk file operations.

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

### Voice not working

1. Verify Xunfei credentials in ⚙️ → **Configure Voice...**
2. Ensure the services are enabled in your Xunfei account
3. Check your network can reach Xunfei servers
