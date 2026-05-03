#!/bin/bash

# ============================================================
# LocalAIAssistant - Unified Cross-Platform Build Script
# ============================================================
# Usage: ./build.sh [command] [options]
#
# Commands:
#   build    - Build the project (default)
#   run      - Run compiled executable directly
#   help     - Show this help message
#
# Build options:
#   -c, --clean       Clean build directory first
#   -d, --debug       Build debug version
#   -r, --release     Build release version (default)
#   -v, --verbose     Show verbose output
#   -j, --jobs <n>    Parallel compile jobs
#   -q, --qt-path     Specify Qt installation path
#   --no-run          Skip "open program" prompt after build
#
# Targets:
#   all               Build all targets (default)
#   LocalAIAssistant  Build GUI version only
#   LocalAIAssistant-CLI  Build CLI version only
#
# Run options:
#   --gui             Run GUI version (default if exists)
#   --cli             Run CLI version
# ============================================================

BUILD_DIR="build"
QT_PATH=""
BUILD_TYPE="Release"
CLEAN_BUILD=false
VERBOSE=false
JOBS=""
TARGET=""
COMMAND="build"
NO_RUN_PROMPT=false
RUN_TARGET=""
CLI_HELP_ONLY=false

# ============================================================
# Pause function for interactive terminal
# ============================================================

pause_if_interactive() {
    if [ -t 0 ]; then
        echo ""
        read -p "按 Enter 键退出..." -r
    fi
}

# ============================================================
# Platform Detection
# ============================================================

detect_platform() {
    case "$OSTYPE" in
        darwin*)  PLATFORM="macos" ;;
        linux*)   PLATFORM="linux" ;;
        msys*|cygwin*|win32*) PLATFORM="windows" ;;
        *)        PLATFORM="unknown" ;;
    esac
    echo "  Platform: $PLATFORM"
}

# ============================================================
# Qt Path Detection
# ============================================================

detect_qt_path() {
    # macOS: Qt official installation
    if [[ "$PLATFORM" == "macos" ]]; then
        if [ -d "$HOME/Qt" ]; then
            QT_VERSION=$(ls -1 "$HOME/Qt" | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | sort -V | tail -1)
            if [ -n "$QT_VERSION" ]; then
                QT_PATH="$HOME/Qt/$QT_VERSION/macos"
                echo "  Detected Qt: $QT_PATH"
                return 0
            fi
        fi

        # macOS: Homebrew
        if command -v brew &> /dev/null; then
            BREW_QT=$(brew --prefix qt@6 2>/dev/null)
            if [ -d "$BREW_QT" ]; then
                QT_PATH="$BREW_QT"
                echo "  Detected Homebrew Qt: $QT_PATH"
                return 0
            fi
        fi
    fi

    # Linux: Qt official installation
    if [[ "$PLATFORM" == "linux" ]]; then
        if [ -d "$HOME/Qt" ]; then
            QT_VERSION=$(ls -1 "$HOME/Qt" | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | sort -V | tail -1)
            if [ -n "$QT_VERSION" ]; then
                QT_PATH="$HOME/Qt/$QT_VERSION/gcc_64"
                if [ -d "$QT_PATH" ]; then
                    echo "  Detected Qt: $QT_PATH"
                    return 0
                fi
            fi
        fi

        # Linux: System Qt
        if command -v qmake6 &> /dev/null; then
            QT_PATH=$(qmake6 -query QT_INSTALL_PREFIX 2>/dev/null)
            if [ -d "$QT_PATH" ]; then
                echo "  Detected system Qt: $QT_PATH"
                return 0
            fi
        fi

        if command -v qmake &> /dev/null; then
            QT_VERSION_QUERY=$(qmake -query QT_VERSION 2>/dev/null)
            if [[ "$QT_VERSION_QUERY" == 6* ]]; then
                QT_PATH=$(qmake -query QT_INSTALL_PREFIX 2>/dev/null)
                if [ -d "$QT_PATH" ]; then
                    echo "  Detected system Qt: $QT_PATH"
                    return 0
                fi
            fi
        fi

        # Linux: Common paths
        for path in "/usr/lib/qt6" "/usr" "/opt/qt6"; do
            if [ -f "$path/lib/libQt6Widgets.so" ] || [ -f "$path/lib64/libQt6Widgets.so" ]; then
                QT_PATH="$path"
                echo "  Detected Qt: $QT_PATH"
                return 0
            fi
        done
    fi

    # Windows: Git Bash / MSYS2 / Cygwin
    if [[ "$PLATFORM" == "windows" ]]; then
        # Check environment variables
        if [ -n "$QT_PATH" ] && [ -d "$QT_PATH" ]; then
            echo "  Detected QT_PATH env: $QT_PATH"
            return 0
        fi

        if [ -n "$Qt6_DIR" ]; then
            QT_PATH=$(cygpath -u "$Qt6_DIR/../.." 2>/dev/null || echo "$Qt6_DIR/../..")
            if [ -d "$QT_PATH" ]; then
                echo "  Detected Qt6_DIR env: $QT_PATH"
                return 0
            fi
        fi

        # Common Windows Qt paths (convert to Unix path for Git Bash)
        # Qt library directories use: mingw_64, msvc2019_64, msvc2022_64
        # MinGW compiler tools are in: C:\Qt\Tools\mingw1310_64\ (separate from Qt lib)
        local qt_versions=("6.11.0" "6.10.2" "6.10.1" "6.10.0" "6.9.2" "6.9.1" "6.9.0" "6.8.2" "6.8.1" "6.8.0" "6.7.3" "6.7.2" "6.7.1" "6.7.0")
        local qt_compilers=("mingw_64" "msvc2019_64" "msvc2022_64")  # Qt library directories
        local drives=("C:" "D:" "E:")

        for drive in "${drives[@]}"; do
            for version in "${qt_versions[@]}"; do
                for compiler in "${qt_compilers[@]}"; do
                    # Build Windows path and convert to Unix
                    local win_path="$drive/Qt/$version/$compiler"
                    # Use cygpath if available, otherwise manual conversion
                    local unix_path=""
                    if command -v cygpath &> /dev/null; then
                        unix_path=$(cygpath -u "$win_path" 2>/dev/null)
                    else
                        # Manual conversion: C:/Qt -> /c/Qt (Git Bash format)
                        local drive_lower=$(echo "${drive%:}" | tr '[:upper:]' '[:lower:]')
                        unix_path="/$drive_lower/Qt/$version/$compiler"
                    fi

                    if [ -d "$unix_path" ]; then
                        QT_PATH="$unix_path"
                        echo "  Detected Qt: $QT_PATH"
                        return 0
                    fi
                done
            done
        done
    fi

    return 1
}

# ============================================================
# Windows PATH Setup (Add Qt and MinGW to PATH)
# ============================================================

setup_windows_path() {
    if [[ "$PLATFORM" != "windows" ]]; then
        return 0
    fi

    echo "Setting up Windows PATH..."

    # Add Qt bin directory to PATH
    if [ -d "$QT_PATH/bin" ]; then
        export PATH="$QT_PATH/bin:$PATH"
        echo "  Added Qt bin: $QT_PATH/bin"
    fi

    # For MinGW builds, also add the MinGW compiler tools
    # Qt library is in: C:\Qt\6.x.x\mingw_64\
    # MinGW compiler is in: C:\Qt\Tools\mingw1310_64\ (separate location!)
    if [[ "$QT_PATH" == *"mingw"* ]]; then
        # Calculate Qt root directory
        # QT_PATH = /c/Qt/6.11.0/mingw_64 -> qt_root = /c/Qt
        local qt_root=$(dirname "$(dirname "$QT_PATH")")

        # MinGW compiler tool paths (these are SEPARATE from Qt library path)
        # Qt installs MinGW compiler in C:\Qt\Tools\mingwXXX_64\bin
        local mingw_versions=("1310" "1120" "1110" "100" "90" "81" "73")
        local mingw_tools_paths=()

        # Build possible MinGW tool paths
        for drive in "c" "d" "e"; do
            for ver in "${mingw_versions[@]}"; do
                mingw_tools_paths+=("/$drive/Qt/Tools/mingw${ver}_64/bin")
            done
        done

        # Also check relative to Qt root
        for ver in "${mingw_versions[@]}"; do
            mingw_tools_paths+=("$qt_root/Tools/mingw${ver}_64/bin")
        done

        # Find and add first available MinGW tools path
        for mingw_path in "${mingw_tools_paths[@]}"; do
            if [ -d "$mingw_path" ]; then
                export PATH="$mingw_path:$PATH"
                echo "  Added MinGW tools: $mingw_path"
                break
            fi
        done
    fi

    # Verify compiler is now available
    if command -v g++ &> /dev/null; then
        local gpp_path=$(command -v g++)
        echo "  OK Compiler: g++ at $gpp_path"
    elif command -v cl &> /dev/null; then
        echo "  OK Compiler: MSVC cl"
    else
        echo "  Warning: No C++ compiler found in PATH"
        echo "  For MinGW: Ensure Qt Tools directory exists (e.g., C:\\Qt\\Tools\\mingw1310_64)"
        echo "  For MSVC: Run from Visual Studio Developer Command Prompt"
    fi
}

# ============================================================
# Dependency Check
# ============================================================

check_dependencies() {
    local missing_deps=()

    echo "Checking dependencies..."

    if ! command -v cmake &> /dev/null; then
        missing_deps+=("cmake")
    else
        echo "  OK CMake: $(cmake --version | head -1)"
    fi

    # Check compiler based on platform
    if [[ "$PLATFORM" == "windows" ]]; then
        # Windows: check for MinGW or MSVC (via cmake)
        if ! command -v g++ &> /dev/null && ! command -v cl &> /dev/null; then
            missing_deps+=("C++ compiler (MinGW g++ or MSVC cl)")
        else
            local compiler=$(command -v g++ &> /dev/null && echo "g++" || echo "MSVC cl")
            echo "  OK Compiler: $compiler"
        fi
    else
        # macOS/Linux
        if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
            missing_deps+=("C++ compiler (g++ or clang++)")
        else
            local compiler=$(command -v g++ &> /dev/null && echo "g++" || echo "clang++")
            echo "  OK Compiler: $compiler"
        fi
    fi

    if [ ${#missing_deps[@]} -ne 0 ]; then
        echo ""
        echo "Error: Missing dependencies:"
        for dep in "${missing_deps[@]}"; do
            echo "  - $dep"
        done
        echo ""
        echo "Please install missing dependencies and retry."
        return 1
    fi

    return 0
}

# ============================================================
# Windows DLL Deployment (windeployqt)
# ============================================================

deploy_windows_dlls() {
    local exe_name="$1"
    local exe_path="$BUILD_DIR/$exe_name.exe"

    if [ ! -f "$exe_path" ]; then
        echo "  Warning: $exe_path not found, skipping DLL deployment"
        return 1
    fi

    echo ""
    echo "Deploying Qt DLLs for Windows..."

    # Find windeployqt
    local windeployqt=""
    if [ -n "$QT_PATH" ]; then
        windeployqt="$QT_PATH/bin/windeployqt.exe"
        if [[ "$PLATFORM" == "windows" ]]; then
            windeployqt=$(cygpath -w "$windeployqt" 2>/dev/null || echo "$windeployqt")
        fi
    fi

    # Also check Qt6_DIR environment variable
    if [ ! -f "$windeployqt" ] && [ -n "$Qt6_DIR" ]; then
        local qt_bin=$(cygpath -u "$Qt6_DIR/../bin" 2>/dev/null || echo "$Qt6_DIR/../bin")
        windeployqt="$qt_bin/windeployqt.exe"
        if [[ "$PLATFORM" == "windows" ]]; then
            windeployqt=$(cygpath -w "$windeployqt" 2>/dev/null || echo "$windeployqt")
        fi
    fi

    if [ ! -f "$(cygpath -u "$windeployqt" 2>/dev/null || echo "$windeployqt")" ]; then
        echo "  Warning: windeployqt not found in Qt bin directory"
        echo "  You may need to manually copy Qt DLLs or run windeployqt"
        return 1
    fi

    # Convert paths for Windows
    local exe_win_path=$(cygpath -w "$exe_path" 2>/dev/null || echo "$exe_path")

    echo "  Running: windeployqt $exe_win_path"

    # Run windeployqt
    if "$windeployqt" --no-translations "$exe_win_path" 2>&1; then
        echo "  OK Qt DLLs deployed successfully"
        return 0
    else
        echo "  Warning: windeployqt reported issues"
        return 1
    fi
}

# ============================================================
# Build Command
# ============================================================

cmd_build() {
    echo ""
    echo "Configuring project..."
    echo "  Build type: $BUILD_TYPE"
    echo "  Target: $TARGET"
    echo "  Build directory: $BUILD_DIR"

    local cmake_args=(
        "-DCMAKE_PREFIX_PATH=$QT_PATH"
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    )

    # Windows MinGW requires explicit generator specification
    # Otherwise CMake may default to Visual Studio generator
    if [[ "$PLATFORM" == "windows" ]]; then
        if [[ "$QT_PATH" == *"mingw"* ]]; then
            cmake_args+=("-G" "MinGW Makefiles")
            echo "  Generator: MinGW Makefiles"
        else
            # MSVC uses default generator (Visual Studio)
            echo "  Generator: Visual Studio (MSVC)"
        fi
    fi

    if [ "$VERBOSE" = true ]; then
        cmake_args+=("-DCMAKE_VERBOSE_MAKEFILE=ON")
    fi

    cd "$BUILD_DIR"

    if ! cmake .. "${cmake_args[@]}"; then
        cd "$PROJECT_ROOT"
        echo ""
        echo "Error: CMake configuration failed"
        return 1
    fi

    echo ""
    echo "Compiling project..."

    local build_args=("--build" ".")

    if [ -n "$JOBS" ]; then
        build_args+=("-j$JOBS")
    else
        if [[ "$PLATFORM" == "macos" ]]; then
            build_args+=("-j$(sysctl -n hw.ncpu)")
        elif [[ "$PLATFORM" == "windows" ]]; then
            # Windows: use NUMBER_OF_PROCESSORS env var or default
            build_args+=("-j${NUMBER_OF_PROCESSORS:-4}")
        else
            build_args+=("-j$(nproc)")
        fi
    fi

    if [ "$TARGET" != "all" ]; then
        build_args+=("--target" "$TARGET")
    fi

    if [ "$VERBOSE" = true ]; then
        build_args+=("-v")
    fi

    local start_time=$(date +%s)

    if ! cmake "${build_args[@]}"; then
        cd "$PROJECT_ROOT"
        echo ""
        echo "Error: Compilation failed"
        return 1
    fi

    # Wait briefly for bundle to be fully created (macOS)
    if [[ "$PLATFORM" == "macos" ]]; then
        sleep 1
    fi

    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    cd "$PROJECT_ROOT"

    # macOS: Copy icon to bundle Resources
    if [[ "$PLATFORM" == "macos" ]]; then
        if [ -f "$PROJECT_ROOT/resources/icons/app.icns" ]; then
            if [ -d "$BUILD_DIR/LocalAIAssistant.app/Contents" ]; then
                mkdir -p "$BUILD_DIR/LocalAIAssistant.app/Contents/Resources"
                cp "$PROJECT_ROOT/resources/icons/app.icns" "$BUILD_DIR/LocalAIAssistant.app/Contents/Resources/app.icns"
                echo "  App icon copied to bundle"
            fi
        fi
    fi

    # Windows: Deploy Qt DLLs
    if [[ "$PLATFORM" == "windows" ]]; then
        if [ "$TARGET" == "all" ] || [ "$TARGET" == "LocalAIAssistant" ]; then
            deploy_windows_dlls "LocalAIAssistant"
        fi
        if [ "$TARGET" == "all" ] || [ "$TARGET" == "LocalAIAssistant-CLI" ]; then
            deploy_windows_dlls "LocalAIAssistant-CLI"
        fi
    fi

    echo ""
    echo "==================================="
    echo "  Build succeeded! (Time: ${duration}s)"
    echo "==================================="

    # Show executable locations
    echo ""
    echo "Executables:"

    local has_gui=false
    local has_cli=false

    if [[ "$PLATFORM" == "macos" ]]; then
        if [ -d "$BUILD_DIR/LocalAIAssistant.app" ]; then
            echo "  GUI: $BUILD_DIR/LocalAIAssistant.app"
            has_gui=true
        fi
        if [ -f "$BUILD_DIR/LocalAIAssistant-CLI" ]; then
            echo "  CLI: $BUILD_DIR/LocalAIAssistant-CLI"
            has_cli=true
        fi
    elif [[ "$PLATFORM" == "windows" ]]; then
        if [ -f "$BUILD_DIR/LocalAIAssistant.exe" ]; then
            echo "  GUI: $BUILD_DIR/LocalAIAssistant.exe"
            has_gui=true
        fi
        if [ -f "$BUILD_DIR/LocalAIAssistant-CLI.exe" ]; then
            echo "  CLI: $BUILD_DIR/LocalAIAssistant-CLI.exe"
            has_cli=true
        fi
    else
        if [ -f "$BUILD_DIR/LocalAIAssistant" ]; then
            echo "  GUI: $BUILD_DIR/LocalAIAssistant"
            has_gui=true
        fi
        if [ -f "$BUILD_DIR/LocalAIAssistant-CLI" ]; then
            echo "  CLI: $BUILD_DIR/LocalAIAssistant-CLI"
            has_cli=true
        fi
    fi

    # Prompt to open program
    if [ "$NO_RUN_PROMPT" = false ]; then
        prompt_open_program "$has_gui" "$has_cli"
    fi

    return 0
}

# ============================================================
# Prompt to Open Program
# ============================================================

prompt_open_program() {
    local has_gui="$1"
    local has_cli="$2"

    if [ "$has_gui" = false ] && [ "$has_cli" = false ]; then
        echo "  No executables found to run"
        return
    fi

    echo ""
    echo "-----------------------------------"
    read -p "Open program now? [Y/n] " -n 1 -r
    echo ""

    if [[ ! $REPLY =~ ^[Yy]$ ]] && [ -n "$REPLY" ]; then
        echo "  Skipping. You can run manually later."
        return
    fi

    # Determine which to open
    local open_target=""

    if [ "$has_gui" = true ] && [ "$has_cli" = true ]; then
        read -p "Which to open? [G]ui / [C]li: " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Cc]$ ]]; then
            open_target="cli"
        else
            open_target="gui"
        fi
    elif [ "$has_gui" = true ]; then
        open_target="gui"
    else
        open_target="cli"
    fi

    run_program "$open_target" "chat"
}

# ============================================================
# Run Program
# ============================================================

run_program() {
    local target="$1"
    local cli_mode="$2"  # "chat" or "help"

    echo ""
    echo "Opening $target..."

    if [[ "$PLATFORM" == "macos" ]]; then
        if [ "$target" == "gui" ]; then
            if [ -d "$BUILD_DIR/LocalAIAssistant.app" ]; then
                open "$BUILD_DIR/LocalAIAssistant.app"
            else
                echo "  Error: GUI not found"
            fi
        else
            if [ -f "$BUILD_DIR/LocalAIAssistant-CLI" ]; then
                if [ "$cli_mode" == "help" ]; then
                    "$BUILD_DIR/LocalAIAssistant-CLI" --help
                else
                    # Enter interactive chat mode
                    "$BUILD_DIR/LocalAIAssistant-CLI" chat
                fi
            else
                echo "  Error: CLI not found"
            fi
        fi
    elif [[ "$PLATFORM" == "windows" ]]; then
        if [ "$target" == "gui" ]; then
            if [ -f "$BUILD_DIR/LocalAIAssistant.exe" ]; then
                local exe_path="$BUILD_DIR/LocalAIAssistant.exe"

                echo "  Launching GUI..."

                # Direct execution in background (most reliable in Git Bash)
                # The & puts it in background so script can continue/exit
                "$exe_path" &
                disown 2>/dev/null || true

                echo "  GUI started in background"
            else
                echo "  Error: GUI not found"
                ls -la "$BUILD_DIR/" 2>/dev/null | head -10
            fi
        else
            if [ -f "$BUILD_DIR/LocalAIAssistant-CLI.exe" ]; then
                if [ "$cli_mode" == "help" ]; then
                    "$BUILD_DIR/LocalAIAssistant-CLI.exe" --help
                else
                    # Enter interactive chat mode
                    "$BUILD_DIR/LocalAIAssistant-CLI.exe" chat
                fi
            else
                echo "  Error: CLI not found"
            fi
        fi
    else
        # Linux
        if [ "$target" == "gui" ]; then
            if [ -f "$BUILD_DIR/LocalAIAssistant" ]; then
                "$BUILD_DIR/LocalAIAssistant" &
            else
                echo "  Error: GUI not found"
            fi
        else
            if [ -f "$BUILD_DIR/LocalAIAssistant-CLI" ]; then
                if [ "$cli_mode" == "help" ]; then
                    "$BUILD_DIR/LocalAIAssistant-CLI" --help
                else
                    # Enter interactive chat mode
                    "$BUILD_DIR/LocalAIAssistant-CLI" chat
                fi
            else
                echo "  Error: CLI not found"
            fi
        fi
    fi
}

# ============================================================
# Run Command (directly run without rebuild)
# ============================================================

cmd_run() {
    local has_gui=false
    local has_cli=false

    # Check what exists
    if [[ "$PLATFORM" == "macos" ]]; then
        [ -d "$BUILD_DIR/LocalAIAssistant.app" ] && has_gui=true
        [ -f "$BUILD_DIR/LocalAIAssistant-CLI" ] && has_cli=true
    elif [[ "$PLATFORM" == "windows" ]]; then
        [ -f "$BUILD_DIR/LocalAIAssistant.exe" ] && has_gui=true
        [ -f "$BUILD_DIR/LocalAIAssistant-CLI.exe" ] && has_cli=true
    else
        [ -f "$BUILD_DIR/LocalAIAssistant" ] && has_gui=true
        [ -f "$BUILD_DIR/LocalAIAssistant-CLI" ] && has_cli=true
    fi

    if [ "$has_gui" = false ] && [ "$has_cli" = false ]; then
        echo "Error: No compiled executables found"
        echo "Please run './build.sh build' first"
        return 1
    fi

    # Determine target
    local target=""
    local cli_mode="chat"  # Default to chat mode

    if [ "$CLI_HELP_ONLY" = true ]; then
        cli_mode="help"
    fi

    if [ -n "$RUN_TARGET" ]; then
        target="$RUN_TARGET"
    elif [ "$has_gui" = true ] && [ "$has_cli" = true ]; then
        # Default to GUI if both exist
        target="gui"
    elif [ "$has_gui" = true ]; then
        target="gui"
    else
        target="cli"
    fi

    run_program "$target" "$cli_mode"
    return 0
}

# ============================================================
# Argument Parsing
# ============================================================

parse_args() {
    # First argument might be a command
    if [[ $# -gt 0 ]]; then
        case $1 in
            build|run|help)
                COMMAND="$1"
                shift
                ;;
        esac
    fi

    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                COMMAND="help"
                shift
                ;;
            -c|--clean)
                CLEAN_BUILD=true
                shift
                ;;
            -d|--debug)
                BUILD_TYPE="Debug"
                shift
                ;;
            -r|--release)
                BUILD_TYPE="Release"
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -j|--jobs)
                JOBS="$2"
                shift 2
                ;;
            -q|--qt-path)
                QT_PATH="$2"
                shift 2
                ;;
            --no-run)
                NO_RUN_PROMPT=true
                shift
                ;;
            --gui)
                RUN_TARGET="gui"
                shift
                ;;
            --cli)
                RUN_TARGET="cli"
                shift
                ;;
            --help-only)
                CLI_HELP_ONLY=true
                shift
                ;;
            LocalAIAssistant|LocalAIAssistant-CLI|all)
                TARGET="$1"
                shift
                ;;
            *)
                echo "Error: Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done

    # Set defaults
    if [ -z "$TARGET" ]; then
        TARGET="all"
    fi
}

# ============================================================
# Help
# ============================================================

show_help() {
    cat << EOF
LocalAIAssistant - Unified Cross-Platform Build Script

Usage: ./build.sh [command] [options] [target]

Commands:
  build    Build the project (default command)
  run      Run compiled executable directly
  help     Show this help message

Build Options:
  -c, --clean       Clean build directory first
  -d, --debug       Build debug version
  -r, --release     Build release version (default)
  -v, --verbose     Show verbose output
  -j, --jobs <n>    Parallel compile jobs (default: auto-detect)
  -q, --qt-path <path>  Specify Qt installation path
  --no-run          Skip "open program" prompt after build

Build Targets:
  all               Build all targets (default)
  LocalAIAssistant  Build GUI version only
  LocalAIAssistant-CLI  Build CLI version only

Run Options:
  --gui             Run GUI version (default if exists)
  --cli             Run CLI version (enters interactive chat mode)
  --help-only       Show CLI help instead of entering chat mode

Examples:
  ./build.sh                        # Build all, prompt to open
  ./build.sh build -c -d            # Clean debug build
  ./build.sh LocalAIAssistant-CLI   # Build CLI only
  ./build.sh -j 8 --no-run          # 8 parallel jobs, no prompt
  ./build.sh run                    # Run GUI (default)
  ./build.sh run --cli              # Run CLI in chat mode
  ./build.sh run --cli --help-only  # Show CLI help only

Environment Variables:
  QT_PATH           Qt installation path
  Qt6_DIR           Qt6 cmake directory

Windows Notes:
  - Requires Git Bash, MSYS2, or WSL
  - Automatically adds Qt bin and MinGW tools to PATH
  - Automatically runs windeployqt to deploy Qt DLLs
  - MinGW compiler (g++) is detected from Qt/Tools directory

Platform-specific Qt detection:
  macOS:   ~/Qt/<version>/macos, Homebrew qt@6
  Linux:   ~/Qt/<version>/gcc_64, system qt6-base-dev
  Windows: C:/Qt/<version>/<compiler>, QT_PATH env
EOF
}

# ============================================================
# Main
# ============================================================

main() {
    parse_args "$@"

    # Get script directory and project root
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

    if [ ! -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
        echo "Error: Project root directory not found"
        echo "Please run this script from the scripts directory"
        exit 1
    fi

    cd "$PROJECT_ROOT"

    if [ "$COMMAND" == "help" ]; then
        show_help
        exit 0
    fi

    echo ""
    echo "==================================="
    echo "  LocalAIAssistant - Build Script"
    echo "==================================="
    echo ""

    detect_platform

    if [ "$COMMAND" == "run" ]; then
        cmd_run
        exit $?
    fi

    # Detect Qt path FIRST (before checking dependencies)
    # This ensures MinGW compiler path is available for dependency check
    if [ -z "$QT_PATH" ]; then
        if ! detect_qt_path; then
            echo ""
            echo "Error: Qt6 installation not found"
            echo ""
            echo "Please specify Qt path via one of these methods:"
            echo "  1. Use -q option: ./build.sh -q /path/to/qt"
            echo "  2. Set environment variable: QT_PATH=/path/to/qt ./build.sh"
            echo ""
            echo "Install Qt6:"
            if [[ "$PLATFORM" == "macos" ]]; then
                echo "  macOS:   Download from https://www.qt.io/download"
                echo "           Or use Homebrew: brew install qt@6"
            elif [[ "$PLATFORM" == "linux" ]]; then
                echo "  Ubuntu/Debian: sudo apt install qt6-base-dev qt6-base-dev-tools"
                echo "  Fedora/RHEL:   sudo dnf install qt6-qtbase-devel"
                echo "  Arch Linux:    sudo pacman -S qt6-base"
            elif [[ "$PLATFORM" == "windows" ]]; then
                echo "  Windows: Download from https://www.qt.io/download"
                echo "           Select MinGW or MSVC version"
            fi
            exit 1
        fi
    else
        echo "  Using specified Qt path: $QT_PATH"
    fi

    # Setup Windows PATH (add Qt bin and MinGW tools to PATH)
    # This must happen BEFORE check_dependencies so compiler is found
    setup_windows_path

    # NOW check dependencies (after PATH is set up)
    if ! check_dependencies; then
        echo ""
        echo "❌ 依赖检查失败，请安装缺少的依赖后重试"
        pause_if_interactive
        exit 1
    fi

    # Clean build if requested
    if [ "$CLEAN_BUILD" = true ]; then
        echo ""
        echo "Cleaning old build files..."
        rm -rf "$BUILD_DIR"
    fi

    # Create build directory
    mkdir -p "$BUILD_DIR"

    # Run build
    if cmd_build; then
        pause_if_interactive
        exit 0
    else
        echo ""
        echo "❌ 编译失败，请检查错误信息"
        pause_if_interactive
        exit 1
    fi
}

main "$@"