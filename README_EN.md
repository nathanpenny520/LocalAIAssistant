# LocalAIAssistant And AI Girlfriend

[中文](README.md) | **English**

A cross-platform AI assistant desktop application based on Qt 6, supporting both GUI and CLI modes,
with a built-in AI Girlfriend voice interaction module.

Github repo: https://github.com/nathanpenny520/LocalAIAssistant.git 

Gitee repo:https://gitee.com/nathanpenny520/LocalAIAssistant.git

![Level 1 Demo](resources/girlfriend/level-1-belle/demo-belle.png)


## Features

### LocalAIAssistant Core Features

- **Dual Mode Support** — GUI interface + CLI command line
- **File Upload** — Support for text, image(multi-model necessary), PDF, and DOCX file attachments
- **Streaming Output** — SSE real-time display, AI responses appear character by character
- **Session Management** — Multi-session switching, history persistence
- **Multi-language** — Simplified Chinese / English switching
- **Theme Switching** — Light / Dark / Follow System
- **Cross-platform** — macOS / Windows / Linux

### AI Girlfriend Module 🎀

- **Independent Window** — Immersive full-screen avatar background, 9:16 window ratio
- **Avatar Level System** — Three levels available:
    - Level 1 (Belle): PNG static images, classic style
    - Level 2 (Hot): PNG static images, more charming
    - Level 3 (Hotter): MP4 dynamic video, dancing before your eyes
- **Emotion System** — 14 expressions real-time switching (happy, shy, loving, playful, crying,
  travelling, etc.)
- **Mood Display** — Real-time mood progress bar and percentage at top-left corner
- **Mood Influence Level** — Configurable mood influence on emotion detection (Low/Med/High)
- **Memory System** — Automatically records user information via text markers, long-term memory
  persistence
- **Multi-session Management** — Create, switch, delete multiple independent sessions
- **Voice Interaction** — Voice input (ASR) + Voice output (TTS)
- **Personality Customization** — Modify girlfriend.md in prompts directory to customize character
- **Voice Output Toggle** — Enable/disable voice playback in settings
- **Video Sound Toggle** — Enable/disable background sound in Level 3 video mode
- **Shortcut Key** — Command/Ctrl+G to quickly open/close girlfriend window

### Knowledge Base Module 📚

- **Document Import** — Supports TXT/MD/PDF/DOCX file import with automatic chunking and
  vectorization
- **Semantic Search** — Intelligent search based on vector similarity
- **Embedding Model** — Supports ONNX Runtime local inference, no internet required
- **HNSW Index** — High-performance approximate nearest neighbor search
- **Async Import** — Background thread processing, non-blocking UI
- **Memory Enhancement** — Cross-session memory extraction, semantic retrieval, context injection

### Task Execution Module 🔧

- **Agent Iteration Loop** — AI observes execution results and autonomously continues working via
  `[ITERATION_FEEDBACK]` → new `[TASK_PLAN]` → execute → ... → `[TASK_COMPLETE]` cycle (max 10
  iterations by default)
- **JSON Task Plans** — AI generates operation plans in JSON format (`create_dir`/`write_file`/
  `move_file`/`delete_file`/`search_files`/`shell_command`), auto-parsed and executed by the app
- **Native File Operations** — Execute file operations via Qt APIs, no shell dependency
  (cross-platform, safer)
- **Cross-platform Shell Support** — Auto-detect available shell (Windows: pwsh→powershell→cmd,
  Unix: $SHELL→zsh→bash→sh)
- **Three-Tier Safety** — Tier 1: Blocked (dangerous commands like `sudo`, `eval`, `rm -rf /` —
  permanent, even `--yes` cannot bypass). Tier 2: Needs Confirmation (system paths, outside-whitelist
  paths — user chooses Allow Once/Always/Deny with per-violation toggling). Tier 3: Approved
  (whitelist paths — auto-execute)
- **Command Injection Prevention** — Detects eval, backtick substitution, PowerShell injection,
  Living-off-the-Land attacks, disk operations, system service manipulation, firewall disabling,
  and 30+ other dangerous patterns
- **Operation Undo** — Supports undoing executed file operations
- **User Confirmation** — All task plans require user review before execution. CLI interactive mode
  uses single-key toggling (a/p/d/number) + `/confirm`. Ask mode uses inline `[Y/n]` prompt.
  `--yes` flag auto-confirms Tier 2 warnings (Tier 1 never bypassed). GUI confirmation dialog
  with per-path buttons.

## Tech Stack

| Item            | Technology                                                                                             |
| --------------- | ------------------------------------------------------------------------------------------------------ |
| Language        | C++17                                                                                                  |
| Framework       | [Qt 6.x](https://www.qt.io) (Widgets, Network, Multimedia, WebSockets, Sql, Concurrent)                |
| Build           | [CMake](https://cmake.org) 3.16+                                                                       |
| PDF Parsing     | [Poppler](https://poppler.freedesktop.org) (PDF parsing disabled if not installed)                     |
| DOCX Parsing    | [libzip](https://libzip.org) + [pugixml](https://pugixml.org) (DOCX parsing disabled if not installed) |
| Embedding Model | [ONNX Runtime](https://onnxruntime.ai) ≥1.16 (optional, uses placeholder vectors if not installed)     |
| Vector Search   | [hnswlib](https://github.com/nmslib/hnswlib) (header-only, auto-included)                              |
| Voice Service   | [iFlytek Open Platform](https://www.xfyun.cn) (WebSocket API)                                          |

## Project Structure

```
sourcecode-ai-assistant/
├── .github/
│   ├── ISSUE_TEMPLATE/
│   │   ├── bug_report.md         # Bug report template
│   │   └── feature_request.md    # Feature request template
│   ├── PULL_REQUEST_TEMPLATE.md  # PR template
│   └── workflows/
│       ├── build.yml             # CI build and release
│       └── format-check.yml      # Code format check
├── cmake/
│   ├── Info.plist.in             # GUI .app bundle configuration
│   └── CLI-Info.plist.in         # CLI .app bundle configuration
├── docs/
│   ├── README.md                 # Documentation index
│   ├── development/              # Development docs
│   │   ├── ROADMAP.md                 # Roadmap
│   │   ├── code-improvement-plan.md   # Code improvement plan
│   │   ├── code-review-report.md      # Code review report
│   │   ├── mcp-integration-plan.md    # MCP integration plan
│   │   ├── agent-runtime-review.md    # Agent runtime review
│   │   └── known-issues.md            # Known issues
│   └── user/                     # User docs
│       ├── USAGE.md              # English usage guide
│       └── USAGE_zh_CN.md        # Chinese usage guide
├── resources/
│   ├── girlfriend/               # AI Girlfriend avatar resources
│   │   ├── level-1-belle/        # Level 1 PNG images
│   │   ├── level-2-hot/          # Level 2 PNG images
│   │   └── level-3-hotter/       # Level 3 MP4 videos
│   ├── icons/                    # App icons
│   │   ├── app.icns              # macOS icon
│   │   ├── app.ico               # Windows icon
│   │   ├── app.png               # Linux icon
│   │   └── app.rc                # Windows resource file
│   ├── installer/
│   │   └── installer.nsi.in      # NSIS installer template
│   ├── models/                   # ONNX embedding model files
│   ├── prompts/                  # AI prompt templates (synced to app resources at build)
│   │   ├── en/                   #   English prompts
│   │   │   ├── girlfriend.md     #     Girlfriend persona
│   │   │   ├── knowledge.md      #     Knowledge base prompt
│   │   │   ├── system.md         #     System prompt
│   │   │   └── task.md           #     Task prompt
│   │   ├── zh_CN/                #   Chinese prompts
│   │   │   ├── girlfriend.md     #     Girlfriend persona
│   │   │   ├── knowledge.md      #     Knowledge base prompt
│   │   │   ├── system.md         #     System prompt
│   │   │   └── task.md           #     Task prompt
│   │   └── girlfriend_memory.md  # Memory enhancement template
│   ├── localaiassistant.desktop  # Linux desktop entry
│   ├── en.lproj/                 # macOS English localization
│   └── zh_CN.lproj/              # macOS Chinese localization
├── scripts/
│   ├── README.md                 # Script usage documentation
│   ├── build.sh                  # Unified cross-platform build script
│   ├── package.sh                # Cross-platform packaging script
│   ├── setup.sh                  # First-time clone initialization script
│   ├── format.sh                 # Code formatting tool
│   ├── version.sh                # Shared version extraction
│   └── cli-wrapper.sh            # macOS CLI launcher (detects iTerm2)
├── src/
│   ├── cli/
│   │   ├── cli_main.cpp          # CLI entry point
│   │   ├── cli_application.cpp   # CLI application logic
│   │   └── cli_application.h     # CLI application header
│   ├── core/                     # Core business logic
│   │   ├── apiprovider.cpp/h     #   API provider base (OpenAI/Ollama/Anthropic/LlamaCpp)
│   │   ├── networkmanager.cpp/h  #   Network request manager (SSE streaming)
│   │   ├── sessionmanager.cpp/h  #   Session management (CRUD, persistence)
│   │   ├── filemanager.cpp/h     #   File attachment manager
│   │   ├── envconfig.cpp/h       #   .env configuration loader
│   │   ├── datamodels.h          #   Data model definitions
│   │   ├── openai_provider.cpp/h     #   OpenAI API adapter
│   │   ├── ollama_provider.cpp/h     #   Ollama API adapter
│   │   ├── anthropic_provider.cpp/h  #   Anthropic API adapter
│   │   ├── llamacpp_provider.cpp/h   #   LlamaCpp API adapter
│   │   └── version.h.in          #   Version number template
│   ├── prompts/                  # Prompt manager
│   │   ├── promptmanager.cpp/h   #   Prompt loading and management
│   │   ├── en/                   #   English prompts (source)
│   │   └── zh_CN/                #   Chinese prompts (source)
│   ├── parsers/                  # File parsers
│   │   ├── fileparser.cpp        #   PDF/DOCX/text parsing
│   │   └── fileparser.h          #   Parser header
│   ├── ui/                       # GUI interface
│   │   ├── main.cpp              #   GUI entry point
│   │   ├── mainwindow.cpp/h      #   Main window (chat, sessions, girlfriend integration)
│   │   ├── settingsdialog.cpp/h  #   Settings dialog (API, theme, language)
│   │   ├── apptheme.cpp/h        #   Theme manager (light/dark/follow system)
│   │   ├── markdownrenderer.cpp/h    #   Markdown renderer
│   │   ├── stylesheetmanager.cpp/h   #   Qt stylesheet manager
│   │   ├── translationmanager.cpp/h  #   Multi-language switcher
│   │   └── operationconfirmdialog.cpp/h  # Task confirmation dialog
│   ├── tasks/                    # Task execution module
│   │   ├── taskengine.cpp/h      #   Task engine (AI response parsing, plan dispatch)
│   │   ├── agentloop.cpp/h       #   Agent iteration loop (plan→execute→feedback→continue)
│   │   ├── commandexecutor.cpp/h #   Command executor (native file ops + shell)
│   │   ├── safetychecker.cpp/h   #   Safety checker (three-tier security architecture)
│   │   ├── operationplan.cpp/h   #   Operation plan definitions
│   │   └── operationundo.cpp/h   #   Operation undo
│   ├── knowledge/                # Knowledge base module
│   │   ├── textchunker.cpp/h     #   Text chunker
│   │   ├── embedder.cpp/h        #   Text-to-vector (ONNX)
│   │   ├── vectordb.cpp/h        #   Vector database (HNSW)
│   │   ├── docimporter.cpp/h     #   Document importer (TXT/MD/PDF/DOCX)
│   │   ├── knowledgebase.cpp/h   #   Knowledge base manager
│   │   └── memoryenhancer.cpp/h  #   Conversation memory enhancer
│   └── girlfriend/               # AI Girlfriend module
│       ├── girlfriendwindow.cpp/h    #   Girlfriend standalone window
│       ├── avatarwidget.cpp/h        #   Avatar/expression/video widget
│       ├── personalityengine.cpp/h   #   Personality engine, emotion detection, mood
│       ├── voicemanager.cpp/h        #   Voice manager (iFlytek ASR/TTS)
│       ├── memorymanager.cpp/h       #   Long-term memory manager
│       ├── girlfriendsettings.cpp/h  #   Settings (avatar level, mood influence, etc.)
│       ├── girlfriendsessionmanager.cpp/h  #   Multi-session manager
│       ├── girlfriendsession.cpp/h   #   Single session data
│       └── girlfriend_translations.h #   Translation helper
├── tests/                        # Unit tests
│   ├── CMakeLists.txt            #   Test build configuration
│   ├── test_agentloop.cpp        #   Agent loop test
│   ├── test_apiprovider.cpp      #   API provider test
│   ├── test_apptheme.cpp         #   Theme test
│   ├── test_commandexecutor.cpp  #   Command executor test
│   ├── test_embedder.cpp         #   Embedder test
│   ├── test_fileparser.cpp       #   File parser test
│   ├── test_knowledgebase.cpp    #   Knowledge base test
│   ├── test_markdownrenderer.cpp #   Markdown renderer test
│   ├── test_safetychecker.cpp    #   Safety checker test
│   ├── test_sessionmanager.cpp   #   Session manager test
│   ├── test_stylesheetmanager.cpp #  Stylesheet manager test
│   └── test_vectordb.cpp         #   Vector database test
├── third_party/
│   └── hnswlib/                  # High-performance vector search (header-only)
├── translations/
│   ├── localai_en.ts             # English translation source
│   └── localai_zh_CN.ts          # Chinese translation source
├── .clang-format                 # C++ code formatting config
├── .clangd                       # clangd LSP config
├── .cmake-format.json            # CMake formatting config
├── .editorconfig                 # Editor universal config
├── .gitattributes                # Git line ending config
├── .gitignore                    # Git ignore rules
├── .prettierignore               # Prettier ignore rules
├── .prettierrc                   # Prettier formatting config
├── .env.example                  # iFlytek voice credential template
├── CMakeLists.txt                # CMake main build file
├── CHANGELOG.md                  # Changelog
├── CLAUDE.md                     # Claude Code configuration
├── CONTRIBUTING.md               # Contribution guide
├── LICENSE                       # MIT License
├── README.md                     # Chinese documentation
└── README_EN.md                  # English documentation
```

---

## First-time Setup

```bash
./scripts/setup.sh
```

Checks build dependencies (CMake, compiler, Qt, Poppler, etc.) and reports missing items with install guides.

> See [scripts/README.md](scripts/README.md) for detailed script usage.

---

## Build Steps

### Development Requirements

- **Qt**: Minimum 6.x, recommended 6.10.3
- **Compiler**: Must support C++17 (macOS: AppleClang 10.0+ / Windows: MSVC 2019+ or MinGW GCC 9+ / Linux: GCC 9+ or Clang 10+)

### 1. Install Dependencies

| Software      | Version            | macOS                                   | Windows                                                             | Linux                                 |
| ------------- | ------------------ | --------------------------------------- | ------------------------------------------------------------------- | ------------------------------------- |
| C++ Compiler  | C++17              | Xcode CLT                               | MinGW (Qt bundled) or MSVC                                          | GCC 9+                                |
| Qt            | 6.x                | Official or [Homebrew](https://brew.sh) | Official (MinGW or MSVC)                                            | Package Manager                       |
| Qt Multimedia | ⚠️ Extra selection | Homebrew auto-install                   | Qt Maintenance Tool select                                          | `qt6-multimedia-dev`                  |
| Qt WebSockets | ⚠️ Extra selection | Homebrew auto-install                   | Qt Maintenance Tool select                                          | `qt6-websockets-dev`                  |
| CMake         | 3.16+              | `brew install cmake`                    | [Official Download](https://cmake.org/download/)                    | `sudo apt install cmake`              |
| Readline      | —                  | System built-in                         | N/A                                                                 | `sudo apt install libreadline-dev`    |
| Poppler       | —                  | `brew install poppler`                  | [MSYS2](https://www.msys2.org) or [vcpkg](https://vcpkg.io)         | `sudo apt install libpoppler-cpp-dev` |
| libzip        | ≥1.5 (optional)    | `brew install libzip`                   | [MSYS2](https://www.msys2.org) or [vcpkg](https://vcpkg.io)         | `sudo apt install libzip-dev`         |
| pugixml       | — (optional)       | `brew install pugixml`                  | [MSYS2](https://www.msys2.org) or [vcpkg](https://vcpkg.io)         | `sudo apt install libpugixml-dev`     |
| ONNX Runtime  | ≥1.16 (optional)   | `brew install onnxruntime`              | [GitHub Release](https://github.com/microsoft/onnxruntime/releases) | `sudo apt install libonnxruntime-dev` |

> **Linux Distribution Note**: The table shows `apt` commands for Ubuntu/Debian. Fedora: `dnf install qt6-qtmultimedia-devel qt6-qtwebsockets-devel libpoppler-cpp-devel libzip-devel libpugixml-devel`. Arch: `pacman -S qt6-multimedia qt6-websockets poppler libzip pugixml`. See "Dependency Installation Supplement" section below for details.
>
> **Qt Module Note**: Multimedia and WebSockets need to be manually selected in Qt Maintenance Tool
> (required for voice features) **Optional Dependencies**: Readline (CLI input enhancement), Poppler
> (PDF parsing), libzip+pugixml (DOCX parsing), ONNX Runtime (knowledge base embedding) — core
> features work without them

#### macOS Quick Install

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew (if not installed)
# See: https://brew.sh

# Install dependencies (qt@6 includes Multimedia and WebSockets)
brew install cmake qt@6 poppler libzip pugixml

# Optional: Install ONNX Runtime for real embedding inference
brew install onnxruntime

# Note: For official Qt installation, manually select Multimedia and WebSockets in Maintenance Tool
```

#### Linux Quick Install (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-base-dev-tools qt6-multimedia-dev qt6-websockets-dev libpoppler-cpp-dev libzip-dev libpugixml-dev
# Optional: sudo apt install libreadline-dev       # CLI input enhancement
# Optional: sudo apt install libonnxruntime-dev    # knowledge base embedding
```

#### Windows Quick Install

**Option 1: MinGW (Recommended, no Visual Studio needed)**

1. Install **[Git for Windows](https://git-scm.com/download/win)** (includes Git Bash)
2. Install **[CMake](https://cmake.org/download/)**
3. Install **[Qt 6](https://www.qt.io/download)**:
    - Select `Qt 6.x.x for MinGW 11.2 64-bit` (Qt bundles compiler, no extra Visual Studio needed)
    - ⚠️ **Important**: Manually select **Qt Multimedia** and **Qt WebSockets** in Qt Maintenance
      Tool (required for voice features)
4. Install **Poppler / libzip / pugixml** (optional, for PDF/DOCX parsing): via
   [MSYS2](https://www.msys2.org)
   (`pacman -S mingw-w64-x86_64-poppler mingw-w64-x86_64-libzip mingw-w64-x86_64-pugixml`) or
   [vcpkg](https://vcpkg.io)

**Option 2: MSVC (Requires Visual Studio)**

1. Install **[Visual Studio 2019+](https://visualstudio.microsoft.com)** (with C++ development
   tools)
2. Install **[CMake](https://cmake.org/download/)**
3. Install **[Qt 6](https://www.qt.io/download)**:
    - Select `Qt 6.x.x for MSVC 2019 64-bit`
    - ⚠️ **Important**: Manually select **Qt Multimedia** and **Qt WebSockets** in Qt Maintenance
      Tool (required for voice features)
4. Install **Poppler / libzip / pugixml** (optional): via [MSYS2](https://www.msys2.org) or
   [vcpkg](https://vcpkg.io)

> **Tip**: MinGW version is lighter, Qt installer bundles compiler; MSVC version has better
> debugging experience.

### 2. Build Project

```bash
cd scripts && ./build.sh
```

> See [scripts/README.md](scripts/README.md) for detailed script usage.

### Build Artifacts

| Platform | GUI                          | CLI                                                              |
| -------- | ---------------------------- | ---------------------------------------------------------------- |
| macOS    | `build/LocalAIAssistant.app` | `build/LocalAIAssistant-CLI` or `build/LocalAIAssistant-CLI.app` |
| Windows  | `build/LocalAIAssistant.exe` | `build/LocalAIAssistant-CLI.exe`                                 |
| Linux    | `build/LocalAIAssistant`     | `build/LocalAIAssistant-CLI`                                     |

> **macOS CLI .app**: Double-click `LocalAIAssistant-CLI.app` auto-detects iTerm2 and prefers to
> open with it, solving Chinese input deletion issues.

### Packaging for Distribution

```bash
./build.sh build -p     # Build + package
./build.sh package      # Package existing build
```

| Platform | Format | Output Path |
|----------|--------|-------------|
| macOS | DMG | `release/LocalAIAssistant-x.x.x-macOS.dmg` |
| Windows | ZIP | `release/LocalAIAssistant-x.x.x-Windows-x64.zip` |
| Linux | tar.gz | `release/LocalAIAssistant-x.x.x-Linux-x86_64.tar.gz` |

> Release packages do **NOT** include the developer's `.env`. No code signing — macOS first launch
> requires right-click → Open. See [scripts/README.md](scripts/README.md).

### Automated Release Publishing (GitHub Actions CI)

Push a `v`-prefixed tag to trigger 3-platform build→test→package→release:

```bash
git tag v1.0.0 && git push origin v1.0.0
```

All three platforms must pass; releases appear at [GitHub Releases](https://github.com/nathanpenny520/LocalAIAssistant/releases).

---

## Usage

### Running

```bash
# GUI
open build/LocalAIAssistant.app        # macOS
build\LocalAIAssistant.exe             # Windows
./build/LocalAIAssistant               # Linux

# CLI
./build/LocalAIAssistant-CLI chat      # Interactive chat
./build/LocalAIAssistant-CLI ask "..."  # One-shot query
```

> Full usage guide, CLI commands, security settings, girlfriend config: [docs/user/USAGE.md](docs/user/USAGE.md).

## Configure AI Service

Two configuration methods are supported, loaded at startup in this order:

```
App startup
  → 1. Load .env file (baseline defaults)
  → 2. Load QSettings (GUI settings; non-empty fields override .env)
  → Active config
```

**Values set in the GUI settings dialog override the same keys in `.env`.** For CLI-only use, configuring `.env` is sufficient.

### Method 1: GUI Settings Dialog (Recommended)

Open the app → Settings → enter API URL, API Key, model name → Save. Settings are persisted to QSettings.

### Method 2: .env File (CLI / Portable Deployment / Baseline Defaults)

Create a `.env` file next to the executable or in the user data directory. Supported API types:

| `AI_API_TYPE` | Service | Notes |
|---|---|---|
| `openai` | OpenAI or compatible API | Default type |
| `ollama` | [Ollama](https://ollama.com/download) local deployment | Auto-detected when URL contains port `11434` |
| `anthropic` | [Anthropic Claude](https://www.anthropic.com) | Requires API Key |
| `llamacpp` | [LlamaCpp](https://github.com/ggerganov/llama.cpp) | OpenAI-compatible format, local |

> **Ollama Auto-Detection**: Even with `AI_API_TYPE=openai`, the app auto-switches to Ollama mode if the API URL contains `11434` (the default Ollama port).

```bash
cp .env.example .env
```

AI configuration in `.env`:

```
AI_API_TYPE=openai                     # openai | ollama | llamacpp | anthropic
AI_API_URL=http://127.0.0.1:8080       # API base URL
AI_API_KEY=sk-your-api-key-here        # API key (can be empty for Ollama/LlamaCpp)
AI_MODEL_NAME=local-model              # Model name
```

---

## AI Girlfriend Module

Open via `Ctrl/Cmd+G`. Voice interaction (ASR + TTS) requires iFlytek credentials. Personality is
customizable via `resources/prompts/<lang>/girlfriend.md`. Memory persists across sessions.

> See [docs/user/USAGE.md](docs/user/USAGE.md) for full girlfriend configuration.

---

## Data Storage Location

Data is stored in two separate locations:

| Data Type | Storage | Contents |
|---|---|---|
| App Config | **QSettings** | API URL/Key, model name, theme, language, etc. |
| Runtime Data | **AppDataLocation** | Sessions, knowledge base, girlfriend settings, memory, etc. |

### QSettings (App Configuration)

Options changed in the GUI settings dialog are stored here:

| Platform | Storage Location |
| -------- | ---------------- |
| macOS    | `~/Library/Preferences/com.localaiassistant.LocalAIAssistant.plist` |
| Windows  | Registry `HKEY_CURRENT_USER\Software\LocalAIAssistant\Settings` |
| Linux    | `~/.config/LocalAIAssistant/Settings.conf` |

Common keys: `apiBaseUrl`, `apiKey`, `modelName`, `apiType`, `temperature`, `topP`, `maxTokens`, `maxContext`, `theme`, `language`, `streamingEnabled`.

### AppDataLocation (Runtime Data)

| Platform | Directory Path |
| -------- | -------------- |
| macOS    | `~/Library/Application Support/LocalAIAssistant/` |
| Windows  | `C:\Users\<USER>\AppData\Local\LocalAIAssistant\` |
| Linux    | `~/.local/share/LocalAIAssistant/` |

Directory structure:

```
<AppDataLocation>/
├── .env                          # User .env config (search path #4)
├── sessions/                     # Main sessions JSON (SessionManager)
│   ├── sessions.json             #   Session metadata list
│   └── session_<id>.json         #   Single session (chat history)
├── girlfriend/                   # AI Girlfriend module
│   ├── settings.json             #   Global settings (avatar, mood, voice, XFYUN credentials)
│   ├── sessions/                 #   Girlfriend sessions
│   ├── session_<id>.json         #   Single girlfriend session
│   └── memory.md                 #   User memory archive
├── knowledge/                    # Knowledge base
│   ├── chunks.db                 #   SQLite text chunks and metadata
│   └── vectors.bin               #   Vector index
├── tasks/                        # Task undo stack
├── prompts/                      # User custom prompts
└── memories.json                 # MemoryEnhancer cross-session memories
```

### Why Can't I Find These Files?

The data directories are **hidden by default** on all three platforms:

- **macOS**: `~/Library/` has been hidden by the system since macOS 10.7
- **Windows**: `AppData` is a hidden folder
- **Linux**: `.local` starts with a dot, making it hidden

**How to access:**

| Platform | Method |
| -------- | ------ |
| macOS    | Finder → Go → Go to Folder... → paste `~/Library/Application Support/LocalAIAssistant/` |
| Windows  | Type `%LOCALAPPDATA%\LocalAIAssistant` in File Explorer address bar |
| Linux    | Run `xdg-open ~/.local/share/LocalAIAssistant/` in terminal |

### .env Search Order

On startup, the app searches for `.env` in the following order (first match wins):

| # | Path | Use Case |
|---|------|----------|
| 1 | `<exeDir>/.env` | Extract-and-run (Windows/Linux portable) |
| 2 | `<exe>/../Resources/.env` | macOS App Bundle |
| 3 | `<CWD>/.env` | Development |
| 4 | `<AppDataLocation>/.env` | User-level config after install |

---

## FAQ

- **Voice not working** — Check iFlytek credentials and Qt Multimedia/WebSockets
- **Session data lost** — Check `sessions.json` and `session_<id>.json` files
- **Memory not recorded** — Verify AI responses include `[update memory:xxx]` marker

> See [docs/user/USAGE.md](docs/user/USAGE.md) troubleshooting section for details.

---

## License

[MIT License](LICENSE)
