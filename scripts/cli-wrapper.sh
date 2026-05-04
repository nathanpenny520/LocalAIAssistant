#!/bin/bash
# ============================================================
# LocalAIAssistant-CLI Wrapper Script
# ============================================================
# Detects iTerm2 and uses it if available for better
# Chinese character input handling (Backspace issue)
# ============================================================

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CLI_BIN="${SCRIPT_DIR}/LocalAIAssistant-CLI-bin"

# Check if iTerm2 is installed
ITERM2_PATH="/Applications/iTerm.app"
if [ ! -d "$ITERM2_PATH" ]; then
    ITERM2_PATH="$HOME/Applications/iTerm.app"
fi

# Function to run in Terminal.app
run_in_terminal() {
    # Terminal.app will automatically run the executable
    exec "$CLI_BIN" "$@"
}

# Function to run in iTerm2
run_in_iterm2() {
    # Use iTerm2's AppleScript API to open a new terminal with our CLI
    osascript <<EOF
tell application "iTerm"
    activate
    create window with default profile
    tell current session of current window
        write text "'$CLI_BIN'"
    end tell
end tell
EOF
    # The AppleScript opens iTerm2, we exit this launcher
    exit 0
}

# Main logic
if [ -d "$ITERM2_PATH" ]; then
    # iTerm2 is installed, use it
    run_in_iterm2 "$@"
else
    # iTerm2 not found, fall back to Terminal.app
    # Note: Terminal.app has Chinese character Backspace issues
    run_in_terminal "$@"
fi