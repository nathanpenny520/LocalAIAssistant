# LocalAIAssistant And AI Girlfriend 

[中文](README.md) | **English**

A cross-platform AI assistant desktop application based on Qt 6, supporting both GUI and CLI modes, with a built-in AI Girlfriend voice interaction module.

Github repo: https://github.com/nathanpenny520/LocalAIAssistant.git
Gitee repo: https://gitee.com/nathanpenny520/LocalAIAssistant.git

![Level 1 Demo](AIGirlfriend/level-1-belle/demo-belle.png)

![Level 2 Demo](AIGirlfriend/level-2-hot/demo-hot.png)

## Features

### LocalAIAssistant Core Features
- **Dual Mode Support** — GUI interface + CLI command line
- **File Upload** — Support for text, image(multi-model necessary), and PDF file attachments
- **Streaming Output** — SSE real-time display, AI responses appear character by character
- **Session Management** — Multi-session switching, history persistence
- **Multi-language** — Simplified Chinese / English switching
- **Theme Switching** — Light / Dark / Follow System
- **Cross-platform** — macOS / Windows / Linux (Linux not tested)

### AI Girlfriend Module 🎀
- **Independent Window** — Immersive full-screen avatar background, 9:16 window ratio
- **Avatar Level System** — Three levels available:
  - Level 1 (Belle): PNG static images, classic style
  - Level 2 (Hot): PNG static images, hotter than you can imagine
  - Level 3 (Hotter): MP4 dynamic video, dancing before your eyes
- **Emotion System** — 14 expressions real-time switching (happy, shy, loving, playful, crying, travelling, etc.)
- **Mood Display** — Real-time mood progress bar and percentage at top-left corner
- **Mood Influence Level** — Configurable mood influence on emotion detection (Low/Med/High)
- **Memory System** — Automatically records user information via text markers, long-term memory persistence
- **Multi-session Management** — Create, switch, delete multiple independent sessions
- **Voice Interaction** — Voice input (ASR) + Voice output (TTS)
- **Personality Customization** — Modify personality.md to customize character
- **Voice Output Toggle** — Enable/disable voice playback in settings
- **Video Sound Toggle** — Enable/disable background sound in Level 3 video mode
- **Shortcut Key** — Command/Ctrl+G to quickly open/close girlfriend window

> ⚠️ **Platform Compatibility**:
> - **macOS**: Full voice input/output support ✅
> - **Windows**: Voice output (TTS) works normally, voice input (ASR) not supported ⚠️
> - **Linux**: Author has no Linux laptop, not tested yet

## Tech Stack

| Item | Technology |
|------|------------|
| Language | C++17 |
| Framework | [Qt 6.x](https://www.qt.io) (Widgets, Network, Multimedia, WebSockets) |
| Build | [CMake](https://cmake.org) 3.16+ |
| PDF Parsing | [Poppler](https://poppler.freedesktop.org) 26.x (PDF upload not supported if not installed) |
| Voice Service | [iFlytek Open Platform](https://www.xfyun.cn) (WebSocket API) |

## Project Structure

```
sourcecode-ai-assistant/
├── src/
│   ├── core/           # Core business logic (network, session, file handling)
│   │   └── datamodels.h    # Data model definitions
│   ├── ui/             # GUI interface (main window, settings dialog)
│   ├── cli/            # CLI command line interface
│   └── girlfriend/     # AI Girlfriend module
│       ├── girlfriendwindow.cpp   # Girlfriend window
│       ├── girlfriendwindow.h     # Girlfriend window header
│       ├── avatarwidget.cpp       # Avatar/expression/video component
│       ├── avatarwidget.h         # Avatar widget header
│       ├── personalityengine.cpp  # Personality engine, emotion detection, mood calculation
│       ├── personalityengine.h    # Personality engine header
│       ├── voicemanager.cpp       # Voice management (iFlytek ASR/TTS)
│       ├── voicemanager.h         # Voice manager header
│       ├── memorymanager.cpp      # Long-term memory management
│       ├── memorymanager.h        # Memory manager header
│       ├── girlfriendsettings.cpp # Settings management (avatar level, mood influence, etc.)
│       ├── girlfriendsettings.h   # Settings header
│       ├── girlfriendsessionmanager.cpp # Multi-session management
│       ├── girlfriendsessionmanager.h   # Session manager header
│       ├── girlfriendsession.cpp  # Single session data
│       ├── girlfriendsession.h    # Session data header
│       ├── girlfriend_translations.h # Translation helper class
│       ├── personality.md         # Personality Prompt (customizable)
│       └── memory.md              # User memory archive
├── AIGirlfriend/       # Avatar resources directory
│   ├── level-1-belle/  # Level 1 PNG images
│   ├── level-2-hot/    # Level 2 PNG images
│   └── level-3-hotter/ # Level 3 MP4 videos
├── scripts/            # Build scripts
│   ├── build.sh        # Unified cross-platform build script
│   ├── setup.sh        # First-time clone initialization script
│   └── cli-wrapper.sh  # macOS CLI launcher (detects iTerm2)
├── translations/       # Internationalization translation files
├── resources/          # Resource files (icons, configs)
├── cmake/              # CMake configuration templates
│   ├── Info.plist.in   # GUI .app bundle configuration
│   └── CLI-Info.plist.in # CLI .app bundle configuration
├── CMakeLists.txt      # CMake main configuration file
├── .gitattributes      # Git line ending configuration
├── .gitignore          # Git ignore rules
├── .env.example        # iFlytek voice credential template
├── LICENSE             # MIT License
├── README.md           # Chinese documentation
└── README_EN.md        # English documentation
```

---

## First-time Setup

After cloning the project, run the initialization script to check your environment:

```bash
./scripts/setup.sh
```

This script will:
1. Copy `.env.example` → `.env` (iFlytek voice credential template)
2. Check build dependencies (CMake, compiler, Qt, Poppler)
3. Show missing dependencies and installation guides

> **Tip**: Run this script to quickly verify if your environment meets build requirements.

---

## Build Steps

### 1. Install Dependencies

| Software | Version | macOS | Windows | Linux |
|----------|---------|-------|---------|-------|
| C++ Compiler | C++17 | Xcode CLT | MinGW (Qt bundled) or MSVC | GCC 9+ |
| Qt | 6.x | Official or [Homebrew](https://brew.sh) | Official (MinGW or MSVC) | Package Manager |
| Qt Multimedia | ⚠️ Extra selection | Homebrew auto-install | Qt Maintenance Tool select | `qt6-multimedia-dev` |
| Qt WebSockets | ⚠️ Extra selection | Homebrew auto-install | Qt Maintenance Tool select | `qt6-websockets-dev` |
| CMake | 3.16+ | `brew install cmake` | [Official Download](https://cmake.org/download/) | `sudo apt install cmake` |
| Poppler | 26.x | `brew install poppler` | [MSYS2](https://www.msys2.org) or [vcpkg](https://vcpkg.io) | `sudo apt install poppler` |

> **Qt Module Note**: Multimedia and WebSockets need to be manually selected in Qt Maintenance Tool (required for voice features)

#### macOS Quick Install

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew (if not installed)
# See: https://brew.sh

# Install dependencies (qt@6 includes Multimedia and WebSockets)
brew install cmake qt@6 poppler

# Note: For official Qt installation, manually select Multimedia and WebSockets in Maintenance Tool
```

#### Linux Quick Install (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-base-dev-tools qt6-multimedia-dev qt6-websockets-dev libpoppler-cpp-dev
```

#### Windows Quick Install

**Option 1: MinGW (Recommended, no Visual Studio needed)**

1. Install **[Git for Windows](https://git-scm.com/download/win)** (includes Git Bash)
2. Install **[CMake](https://cmake.org/download/)**
3. Install **[Qt 6](https://www.qt.io/download)**:
   - Select `Qt 6.x.x for MinGW 11.2 64-bit` (Qt bundles compiler, no extra Visual Studio needed)
   - ⚠️ **Important**: Manually select **Qt Multimedia** and **Qt WebSockets** in Qt Maintenance Tool (required for voice features)
4. Install **Poppler**: via [MSYS2](https://www.msys2.org) (`pacman -S poppler`) or [vcpkg](https://vcpkg.io)

**Option 2: MSVC (Requires Visual Studio)**

1. Install **[Visual Studio 2019+](https://visualstudio.microsoft.com)** (with C++ development tools)
2. Install **[CMake](https://cmake.org/download/)**
3. Install **[Qt 6](https://www.qt.io/download)**:
   - Select `Qt 6.x.x for MSVC 2019 64-bit`
   - ⚠️ **Important**: Manually select **Qt Multimedia** and **Qt WebSockets** in Qt Maintenance Tool (required for voice features)
4. Install **Poppler**: via [MSYS2](https://www.msys2.org) or [vcpkg](https://vcpkg.io)

> **Tip**: MinGW version is lighter, Qt installer bundles compiler; MSVC version has better debugging experience.

### 2. Build Project

```bash
cd scripts
./build.sh
```

> **Windows Note**:
> - Must run in **Git Bash** (bundled with Git for Windows)
> - Script auto-detects Qt and MinGW compiler paths, no manual environment variable setup needed

### Build Options

```bash
# Clean rebuild
./build.sh -c

# Debug build
./build.sh -d

# Specify Qt path
./build.sh -q /path/to/qt

# Build CLI only
./build.sh LocalAIAssistant-CLI

# Build GUI only
./build.sh LocalAIAssistant

# Show help
./build.sh help
```

### Build Artifacts

| Platform | GUI | CLI |
|----------|-----|-----|
| macOS | `build/LocalAIAssistant.app` | `build/LocalAIAssistant-CLI` or `build/LocalAIAssistant-CLI.app` |
| Windows | `build/LocalAIAssistant.exe` | `build/LocalAIAssistant-CLI.exe` |
| Linux | `build/LocalAIAssistant` | `build/LocalAIAssistant-CLI` |

> **macOS CLI .app**: Double-click `LocalAIAssistant-CLI.app` auto-detects iTerm2 and prefers to open with it, solving Chinese input deletion issues.

---

## Usage

### Run GUI Version

```bash
# macOS
open build/LocalAIAssistant.app

# Windows
build\LocalAIAssistant.exe

# Windows debug mode (shows log console)
build\LocalAIAssistant.exe --debug

# Linux
./build/LocalAIAssistant
```

> **Windows Debug Tip**: Use `--debug` flag to show debug console window for viewing logs. Can also set environment variable `LOCALAI_DEBUG=1`.

### Run CLI Version

```bash
# macOS / Linux
./build/LocalAIAssistant-CLI

# macOS .app bundle (double-click to run, auto-detects iTerm2)
open build/LocalAIAssistant-CLI.app

# Windows (Git Bash)
./build/LocalAIAssistant-CLI.exe

# Windows (CMD/PowerShell)
build\LocalAIAssistant-CLI.exe
```

> **macOS Terminal Recommendation**: Use [iTerm2](https://iterm2.com) instead of Terminal.app.
> The default Terminal may have issues with Chinese character deletion (Backspace doesn't delete characters completely).
> CLI .app bundle automatically detects iTerm2 and prefers to open with it.

> **readline Support**: macOS includes readline library, auto-enabled during build,
> providing better input experience (history support, proper multi-byte character editing).

**CLI Command Examples**:

```bash
# Enter interactive chat
./build/LocalAIAssistant-CLI chat

# Single query
./build/LocalAIAssistant-CLI ask "What is artificial intelligence?"

# Session management
./build/LocalAIAssistant-CLI sessions -l    # List sessions
./build/LocalAIAssistant-CLI sessions -n    # New session

# Configuration management
./build/LocalAIAssistant-CLI config --show-config
./build/LocalAIAssistant-CLI config --api-url "http://127.0.0.1:11434"
```

### CLI Interactive Commands

Available in CLI chat mode:

| Command | Function |
|---------|----------|
| `/help` | Show help |
| `/new` | New session |
| `/list` | List all sessions |
| `/switch <id>` | Switch session |
| `/delete <id>` | Delete session |
| `/config` | Show configuration |
| `/file <path>` | Add file attachment |
| `/listfiles` | View pending files |
| `/clearfiles` | Clear file list |
| `/exit` | Exit program |

---

## Configure AI Service

The program needs to connect to an AI service to work.

### Local Deployment: [Ollama](https://ollama.com/download) (Some features may not work)

1. Download and install Ollama: https://ollama.com/download
2. Download model: `ollama pull llama3`
3. Configure in program settings:
   - API URL: `http://127.0.0.1:11434`
   - Model name: `llama3`

### Use Cloud API (Recommended, Verified)

| Service | API URL | Description |
|---------|---------|-------------|
| [OpenAI](https://openai.com) | `https://api.openai.com` | Requires API Key |
| [Paratera](https://www.paratera.com) | `https://llmapi.paratera.com` | China API proxy service |
| Other OpenAI compatible services | Configure per provider docs | — |

---

## AI Girlfriend Module Configuration

The AI Girlfriend module provides voice interaction experience, requires iFlytek voice service configuration.

### Step 1: Register [iFlytek Open Platform](https://www.xfyun.cn) Account

1. Visit iFlytek Open Platform: https://www.xfyun.cn
2. Register and login
3. Go to "Console" → "Create Application"

### Step 2: Enable Voice Services

Enable the following services in your application:

| Service | Name | Purpose |
|---------|------|---------|
| **Voice Dictation (Recognition)** | Streaming (WebSocket) | Speech to text |
| **Voice Synthesis** | Ultra-realistic (WebSocket) | Text to speech |

### Step 3: Get API Credentials

After creating application, get three credentials from console:

```
APPID     - Application ID
API Key   - API Key
API Secret - API Secret
```

### Step 4: Configure Credentials

Create `.env` file in project root:

```bash
# Copy template
cp .env.example .env

# Edit and fill in your credentials
```

`.env` file content:
```
XFYUN_APP_ID=your_app_id
XFYUN_API_KEY=your_api_key
XFYUN_API_SECRET=your_api_secret
```

> **Security Note**: `.env` file is in `.gitignore`, won't be committed to Git.

### TTS Voice Selection

Modify `.env` to select different voice tones:

| Voice Parameter | Name | Characteristics |
|-----------------|------|-----------------|
| `x6_wumeinv_pro` | Wumei Sister | Natural, rich emotion ⭐Recommended, needs manual addition |
| `x6_lingfeiyi_pro` | Lingfeiyi | Youthful warm, male voice ⭐Recommended, included after enabling |

---

## Using AI Girlfriend Module

### Open AI Girlfriend Window

Select "AI Girlfriend" from View menu, or use shortcut `Ctrl/Cmd+G`.

### Voice Interaction Flow

```
1. Click 🎤 button to start recording (button turns red 🔴)
2. Speak into microphone
3. Click button again to stop recording
4. Wait for recognition, text auto-sent
5. AI response auto-played via voice
```

> **Windows Users Note**: Voice input (ASR) currently not available on Windows. You can still use text input, voice output (TTS) works normally.

### Customize Personality

Edit `src/girlfriend/personality.md` to customize AI girlfriend's personality and response style.
Rebuild or copy file to application resource directory after modification.

### Memory System Mechanism

AI girlfriend's memory system is implemented via text markers (defined in personality.md through system prompt, **memory system code not recommended to remove**), no API tool calls needed.

---

## Dependency Installation Supplement

### Qt 6 Multimedia and WebSockets Modules

AI girlfriend voice features require Qt Multimedia (audio recording/playback) and Qt WebSockets (iFlytek API connection) modules.

**macOS (Qt Official Installation)**:
1. Open `/Applications/Qt/MaintenanceTool.app`
2. Select "Add or remove components"
3. Find Qt 6.x → Additional Libraries
4. Select "Qt Multimedia" and "Qt WebSockets"
5. Click Install

> **Note**: Homebrew installed `qt@6` already includes both modules.

**Linux (Package Manager)**:
```bash
# Ubuntu/Debian
sudo apt install qt6-multimedia-dev qt6-websockets-dev

# Fedora
sudo dnf install qt6-qtmultimedia-devel qt6-qtwebsockets-devel

# Arch Linux
sudo pacman -S qt6-multimedia qt6-websockets
```

**Windows (Qt Official Installation)**:
Same as macOS, select Multimedia and WebSockets in Qt Maintenance Tool.

---

## Development Environment

### Qt Version Requirements

- Minimum: Qt 6.10.3
- Recommended: Qt 6.10.3

### Compiler Requirements

- **C++17 support** (required)
- macOS: AppleClang 10.0+ (Xcode 10+)
- Windows: MSVC 2019+ or MinGW GCC 9+ (Qt bundled)
- Linux: GCC 9+ or Clang 10+

---

## Data Storage Location

AI girlfriend data files are stored in the `girlfriend/` subdirectory of user data directory:

| Platform | Data Directory Path |
|----------|---------------------|
| macOS | `~/Library/Application Support/LocalAIAssistant/girlfriend/` |
| Windows | `%APPDATA%\LocalAIAssistant\girlfriend\` |
| Linux | `~/.local/share/LocalAIAssistant/girlfriend/` |

Files in subdirectory:

| File | Content |
|------|---------|
| `settings.json` | Global settings (avatar level, mood influence, voice output toggle, etc.) |
| `sessions.json` | Session metadata list (ID, name, creation time) |
| `session_<id>.json` | Single session data (conversation history, emotion state) |
| `memory.md` | User memory archive (basic info, preferences, events) |

---

## FAQ

### Voice Features Not Working

**Problem**: Voice button shows "Voice not configured"

**Solution**:
1. Check if `.env` file exists and credentials are correct
2. Confirm "Voice Dictation" and "Ultra-realistic Voice Synthesis" services are enabled in iFlytek console
3. Confirm Qt Multimedia and WebSockets modules are installed
4. Rebuild application

**Windows Voice Input Issue**:

Voice input (ASR) currently not supported on Windows due to Windows Media Foundation audio subsystem compatibility with Qt 6 QAudioSource. May be fixed in future versions.

Temporary workaround:
- Use text input instead of voice input
- Voice output (TTS) should still work normally

### WebSockets Not Found During Build

**Problem**: `Could NOT find Qt6WebSockets`

**Solution**: Install Qt WebSockets module (see "Dependency Installation Supplement" above)

### iFlytek API Errors

**Problem**: Voice recognition returns error codes

**Common Error Codes**:
| Error Code | Cause | Solution |
|------------|-------|----------|
| 10005 | API Key error | Check credentials |
| 10006 | Invalid parameter | Check APPID format |
| 10007 | Illegal parameter | Check API Secret |
| 10010 | No authorization | Enable corresponding service |
| 10014 | Engine not enabled | Enable voice service in console |
| 10700 | Engine error | Contact iFlytek support |

### AI Response Too Long, Sounds Like Customer Service

**Problem**: Response exceeds 50 characters, mechanical tone

**Solution**: Edit `personality.md` to adjust personality, ensure it includes:
- Response length limit (under 30 characters)
- Colloquial expression rules
- Prohibit customer service language like "you", "according to my understanding"

### Expression Not Switching

**Problem**: Avatar expression always default state

**Solution**:
1. Check if AI response contains `[emotion:xxx]` marker
2. Confirm corresponding level's `AIGirlfriend/LevelX/` directory has complete resources
3. Level 1/2 need PNG images, Level 3 needs MP4 videos
4. Check console log to confirm emotion detection triggered

### Video Mode UI Invisible

**Problem**: In Level 3 video mode, emotion labels and chat area not visible

**Explanation**: This is a technical limitation of Qt QVideoWidget using native window rendering on macOS. Only the settings button remains visible for level switching. Recommend using Level 1 or Level 2 image modes for full UI experience.

### Multi-session Data Loss

**Problem**: Conversation history disappeared after switching sessions

**Solution**:
1. Check if `sessions.json` and `session_<id>.json` files exist
2. Confirm data was auto-saved before switching
3. Avoid manually deleting session data files

### Memory Not Recorded

**Problem**: AI doesn't remember previously shared information

**Solution**:
1. Check if `memory.md` file has content (in user data directory)
2. Confirm AI response contains `[update memory:xxx]` marker
3. Some models don't support special marker output, try different model
4. Emphasize memory rules in `personality.md` to guide AI output

---

## License

[MIT License](LICENSE)