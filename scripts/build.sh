#!/bin/bash

# ============================================================
# LocalAIAssistant - Unified Cross-Platform Build Script
# ============================================================
# Usage: ./build.sh [command] [options]
#
# Commands:
#   build    - Build the project (default)
#   run      - Run compiled executable directly
#   test     - Build and run unit tests (ctest)
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
PACKAGE=false

# ============================================================
# Pause function for interactive terminal
# ============================================================

pause_if_interactive() {
    if [ -t 0 ]; then
        echo ""
        read -p "Press Enter to exit..." -r
    fi
}

# ============================================================
# Platform Detection
# ============================================================

detect_platform() {
    case "$OSTYPE" in
        darwin*) PLATFORM="macos" ;;
        linux*) PLATFORM="linux" ;;
        msys* | cygwin* | win32*) PLATFORM="windows" ;;
        *) PLATFORM="unknown" ;;
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
        if command -v brew &>/dev/null; then
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
        if command -v qmake6 &>/dev/null; then
            QT_PATH=$(qmake6 -query QT_INSTALL_PREFIX 2>/dev/null)
            if [ -d "$QT_PATH" ]; then
                echo "  Detected system Qt: $QT_PATH"
                return 0
            fi
        fi

        if command -v qmake &>/dev/null; then
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
        local qt_compilers=("mingw_64" "msvc2019_64" "msvc2022_64") # Qt library directories
        local drives=("C:" "D:" "E:")

        for drive in "${drives[@]}"; do
            for version in "${qt_versions[@]}"; do
                for compiler in "${qt_compilers[@]}"; do
                    # Build Windows path and convert to Unix
                    local win_path="$drive/Qt/$version/$compiler"
                    # Use cygpath if available, otherwise manual conversion
                    local unix_path=""
                    if command -v cygpath &>/dev/null; then
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
    if command -v g++ &>/dev/null; then
        local gpp_path=$(command -v g++)
        echo "  OK Compiler: g++ at $gpp_path"
    elif command -v cl &>/dev/null; then
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

    if ! command -v cmake &>/dev/null; then
        missing_deps+=("cmake")
    else
        echo "  OK CMake: $(cmake --version | head -1)"
    fi

    # Check compiler based on platform
    if [[ "$PLATFORM" == "windows" ]]; then
        # Windows: check for MinGW or MSVC (via cmake)
        if ! command -v g++ &>/dev/null && ! command -v cl &>/dev/null; then
            missing_deps+=("C++ compiler (MinGW g++ or MSVC cl)")
        else
            local compiler=$(command -v g++ &>/dev/null && echo "g++" || echo "MSVC cl")
            echo "  OK Compiler: $compiler"
        fi
    else
        # macOS/Linux
        if ! command -v g++ &>/dev/null && ! command -v clang++ &>/dev/null; then
            missing_deps+=("C++ compiler (g++ or clang++)")
        else
            local compiler=$(command -v g++ &>/dev/null && echo "g++" || echo "clang++")
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

    # Package if requested
    if [ "$PACKAGE" = true ]; then
        cmd_package
    fi

    # Prompt to open program
    if [ "$NO_RUN_PROMPT" = false ]; then
        prompt_open_program "$has_gui" "$has_cli"
    fi

    return 0
}

# ============================================================
# Package — Create distributable release artifacts
# ============================================================

get_version() {
    grep -oE '[0-9]+\.[0-9]+\.[0-9]+' "$PROJECT_ROOT/CMakeLists.txt" | head -1
}

package_macos() {
    local version
    version=$(get_version)
    local app_path="$BUILD_DIR/LocalAIAssistant.app"
    local cli_bin="$BUILD_DIR/LocalAIAssistant-CLI"
    local release_dir="$PROJECT_ROOT/release"
    local dmg_name="LocalAIAssistant-${version}-macOS.dmg"
    local macdeployqt="$QT_PATH/bin/macdeployqt"

    echo ""
    echo "========================================="
    echo "  Packaging for macOS"
    echo "========================================="

    if [ ! -d "$app_path" ]; then
        echo "Error: $app_path not found. Build first."
        return 1
    fi

    rm -rf "$release_dir"
    mkdir -p "$release_dir"

    # Run macdeployqt to bundle Qt frameworks
    if [ -f "$macdeployqt" ]; then
        echo ""
        echo "[1/3] Bundling Qt frameworks with macdeployqt..."
        "$macdeployqt" "$app_path" -verbose=1 -no-strip 2>&1 | sed 's/^/  /'
        if [ $? -ne 0 ]; then
            echo "  Warning: macdeployqt reported issues, continuing anyway..."
        fi
    else
        echo ""
        echo "[1/3] macdeployqt not found at $macdeployqt"
        echo "  Qt frameworks will NOT be bundled. The app will only"
        echo "  run on machines with Qt installed."
    fi

    # Create staging directory for DMG
    echo ""
    echo "[2/3] Creating DMG staging directory..."
    local staging="$release_dir/staging"
    mkdir -p "$staging"
    cp -R "$app_path" "$staging/"

    # Remove developer .env from app bundle (avoid leaking credentials)
    rm -f "$staging/LocalAIAssistant.app/Contents/Resources/.env" 2>/dev/null || true
    echo "  .env removed from app bundle (use Settings UI or .env.example)"

    # Copy .env.example as a template for users
    if [ -f "$PROJECT_ROOT/.env.example" ]; then
        cp "$PROJECT_ROOT/.env.example" "$staging/.env.example"
        echo "  .env.example template included"
    fi
    ln -s /Applications "$staging/Applications"

    # Copy CLI binary into staging (alongside .app)
    if [ -f "$cli_bin" ]; then
        cp "$cli_bin" "$staging/"
        echo "  CLI binary included"
    fi

    # Create DMG
    echo ""
    echo "[3/3] Creating DMG..."
    local dmg_path="$release_dir/$dmg_name"
    hdiutil create -volname "LocalAIAssistant" \
        -srcfolder "$staging" \
        -ov -format UDZO \
        "$dmg_path" 2>&1 | sed 's/^/  /'

    if [ -f "$dmg_path" ]; then
        rm -rf "$staging"
        echo ""
        echo "  === macOS package created ==="
        echo "  $dmg_path"
        echo ""
        echo "  Note: This app is not code-signed."
        echo "  Users must right-click → Open to launch the first time."
    else
        echo "  Error: DMG creation failed"
        return 1
    fi
}

package_windows() {
    local version
    version=$(get_version)
    local gui_exe="$BUILD_DIR/LocalAIAssistant.exe"
    local cli_exe="$BUILD_DIR/LocalAIAssistant-CLI.exe"
    local release_dir="$PROJECT_ROOT/release"
    local zip_name="LocalAIAssistant-${version}-Windows.zip"

    echo ""
    echo "========================================="
    echo "  Packaging for Windows"
    echo "========================================="

    if [ ! -f "$gui_exe" ] && [ ! -f "$cli_exe" ]; then
        echo "Error: No executables found in $BUILD_DIR. Build first."
        return 1
    fi

    rm -rf "$release_dir"
    mkdir -p "$release_dir"

    # Create staging directory
    local staging="$release_dir/LocalAIAssistant"
    mkdir -p "$staging"

    echo ""
    echo "[1/2] Collecting files..."

    # Copy GUI executable and all adjacent files (DLLs, resources)
    if [ -f "$gui_exe" ]; then
        cp "$gui_exe" "$staging/" 2>/dev/null
        echo "  GUI executable copied"
    fi

    # Copy CLI executable
    if [ -f "$cli_exe" ]; then
        cp "$cli_exe" "$staging/" 2>/dev/null
        echo "  CLI executable copied"
    fi

    # Copy resource directories from build
    for dir in core AIGirlfriend girlfriend translations models; do
        if [ -d "$BUILD_DIR/$dir" ]; then
            cp -R "$BUILD_DIR/$dir" "$staging/"
            echo "  $dir/ copied"
        fi
    done

    # Copy individual resource files (skip .env to avoid leaking developer credentials)
    for file in soul.md personality.md memory.md; do
        if [ -f "$BUILD_DIR/$file" ]; then
            cp "$BUILD_DIR/$file" "$staging/"
        fi
    done

    # Copy .env.example as a template for users
    if [ -f "$PROJECT_ROOT/.env.example" ]; then
        cp "$PROJECT_ROOT/.env.example" "$staging/.env.example"
        echo "  .env.example template included"
    fi

    # Copy usage docs
    if [ -d "$PROJECT_ROOT/docs" ]; then
        mkdir -p "$staging/docs"
        cp "$PROJECT_ROOT/docs/USAGE.md" "$staging/docs/" 2>/dev/null || true
        cp "$PROJECT_ROOT/docs/USAGE_zh_CN.md" "$staging/docs/" 2>/dev/null || true
        echo "  docs/ copied"
    fi

    echo ""
    echo "[2/2] Creating zip archive..."

    cd "$release_dir"
    if command -v zip &>/dev/null; then
        zip -rq "$zip_name" "LocalAIAssistant"
        echo "  === Windows package created ==="
        echo "  $release_dir/$zip_name"
    else
        echo "  Error: 'zip' command not found"
        cd "$PROJECT_ROOT"
        return 1
    fi
    cd "$PROJECT_ROOT"

    rm -rf "$staging"
}

package_linux() {
    local version
    version=$(get_version)
    local gui_bin="$BUILD_DIR/LocalAIAssistant"
    local cli_bin="$BUILD_DIR/LocalAIAssistant-CLI"
    local release_dir="$PROJECT_ROOT/release"
    local archive_name="LocalAIAssistant-${version}-Linux.tar.gz"

    echo ""
    echo "========================================="
    echo "  Packaging for Linux"
    echo "========================================="

    if [ ! -f "$gui_bin" ] && [ ! -f "$cli_bin" ]; then
        echo "Error: No executables found in $BUILD_DIR. Build first."
        return 1
    fi

    rm -rf "$release_dir"
    mkdir -p "$release_dir"

    local staging="$release_dir/LocalAIAssistant-${version}"
    mkdir -p "$staging"

    echo ""
    echo "[1/2] Collecting files..."

    # Copy binaries
    if [ -f "$gui_bin" ]; then
        cp "$gui_bin" "$staging/"
        echo "  GUI binary copied"
    fi
    if [ -f "$cli_bin" ]; then
        cp "$cli_bin" "$staging/"
        echo "  CLI binary copied"
    fi

    # Copy resource directories
    for dir in core AIGirlfriend girlfriend translations models; do
        if [ -d "$BUILD_DIR/$dir" ]; then
            cp -R "$BUILD_DIR/$dir" "$staging/"
            echo "  $dir/ copied"
        fi
    done

    # Copy individual resource files (skip .env to avoid leaking developer credentials)
    for file in soul.md personality.md memory.md; do
        if [ -f "$BUILD_DIR/$file" ]; then
            cp "$BUILD_DIR/$file" "$staging/"
        fi
    done

    # Copy .env.example as a template for users
    if [ -f "$PROJECT_ROOT/.env.example" ]; then
        cp "$PROJECT_ROOT/.env.example" "$staging/.env.example"
        echo "  .env.example template included"
    fi

    # Copy usage docs
    if [ -d "$PROJECT_ROOT/docs" ]; then
        mkdir -p "$staging/docs"
        cp "$PROJECT_ROOT/docs/USAGE.md" "$staging/docs/" 2>/dev/null || true
        cp "$PROJECT_ROOT/docs/USAGE_zh_CN.md" "$staging/docs/" 2>/dev/null || true
        echo "  docs/ copied"
    fi

    # Copy desktop file
    if [ -f "$PROJECT_ROOT/resources/localaiassistant.desktop" ]; then
        cp "$PROJECT_ROOT/resources/localaiassistant.desktop" "$staging/"
        echo "  .desktop file copied"
    fi

    # Create install script
    cat >"$staging/install.sh" <<'INSTALL_SCRIPT'
#!/bin/bash
INSTALL_DIR="$HOME/.local"
echo "Installing LocalAIAssistant..."
mkdir -p "$INSTALL_DIR/bin"
mkdir -p "$INSTALL_DIR/share/localaiassistant"
mkdir -p "$INSTALL_DIR/share/applications"
mkdir -p "$INSTALL_DIR/share/icons/hicolor/256x256/apps"

cp LocalAIAssistant "$INSTALL_DIR/bin/" 2>/dev/null || true
cp LocalAIAssistant-CLI "$INSTALL_DIR/bin/" 2>/dev/null || true
cp -R core AIGirlfriend girlfriend translations models "$INSTALL_DIR/share/localaiassistant/" 2>/dev/null || true

if [ -f "localaiassistant.desktop" ]; then
    sed -i "s|^Exec=.*|Exec=$INSTALL_DIR/bin/LocalAIAssistant|" localaiassistant.desktop
    cp localaiassistant.desktop "$INSTALL_DIR/share/applications/"
fi

echo "Done. Run 'LocalAIAssistant' from terminal or find it in your app launcher."
echo "Add $INSTALL_DIR/bin to your PATH if it is not already."
INSTALL_SCRIPT
    chmod +x "$staging/install.sh"
    echo "  install.sh created"

    # Create tar.gz
    echo ""
    echo "[2/2] Creating tar.gz archive..."

    cd "$release_dir"
    tar -czf "$archive_name" "LocalAIAssistant-${version}"
    echo "  === Linux package created ==="
    echo "  $release_dir/$archive_name"

    cd "$PROJECT_ROOT"
    rm -rf "$staging"
}

cmd_package() {
    # Detect platform if not already set
    if [ -z "$PLATFORM" ]; then
        detect_platform
    fi

    # Ensure Qt path is set (needed for macdeployqt)
    if [ -z "$QT_PATH" ] && [[ "$PLATFORM" == "macos" ]]; then
        detect_qt_path || true
    fi

    echo ""
    echo "==================================="
    echo "  LocalAIAssistant - Package"
    echo "==================================="

    case "$PLATFORM" in
        macos)
            package_macos
            ;;
        windows)
            package_windows
            ;;
        linux)
            package_linux
            ;;
        *)
            echo "Error: Unknown platform '$PLATFORM'"
            return 1
            ;;
    esac
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
        while true; do
            read -p "Which to open? [G]ui / [C]li: " -n 1 -r
            echo ""
            if [[ $REPLY =~ ^[Gg]$ ]]; then
                open_target="gui"
                break
            elif [[ $REPLY =~ ^[Cc]$ ]]; then
                open_target="cli"
                break
            else
                echo "Invalid input. Please enter G or C."
            fi
        done
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
    local cli_mode="$2" # "chat" or "help"

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
# Test Command (build and run unit tests)
# ============================================================

cmd_test() {
    echo ""
    echo "Running unit tests (ctest)..."
    echo ""

    local build_dir="${BUILD_DIR:-build}"

    if [ ! -d "$build_dir" ]; then
        echo "Build directory not found. Running build first..."
        if ! cmd_build; then
            echo "Build failed, cannot run tests."
            return 1
        fi
    fi

    cd "$build_dir"
    if ctest --output-on-failure "$@"; then
        echo ""
        echo "All tests passed."
    else
        echo ""
        echo "Some tests failed. Check output above for details."
        return 1
    fi
    cd "$PROJECT_ROOT"
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
    local cli_mode="chat" # Default to chat mode

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
            build | run | test | help | package)
                COMMAND="$1"
                shift
                ;;
        esac
    fi

    while [[ $# -gt 0 ]]; do
        case $1 in
            -h | --help)
                COMMAND="help"
                shift
                ;;
            -c | --clean)
                CLEAN_BUILD=true
                shift
                ;;
            -d | --debug)
                BUILD_TYPE="Debug"
                shift
                ;;
            -r | --release)
                BUILD_TYPE="Release"
                shift
                ;;
            -v | --verbose)
                VERBOSE=true
                shift
                ;;
            -j | --jobs)
                JOBS="$2"
                shift 2
                ;;
            -q | --qt-path)
                QT_PATH="$2"
                shift 2
                ;;
            --no-run)
                NO_RUN_PROMPT=true
                shift
                ;;
            -p | --package)
                PACKAGE=true
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
            LocalAIAssistant | LocalAIAssistant-CLI | all)
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
    cat <<EOF
LocalAIAssistant - Unified Cross-Platform Build Script

Usage: ./build.sh [command] [options] [target]

Commands:
  build    Build the project (default command)
  run      Run compiled executable directly
  test     Build and run unit tests (ctest)
  package  Package build artifacts for distribution
  help     Show this help message

Build Options:
  -c, --clean       Clean build directory first
  -d, --debug       Build debug version
  -r, --release     Build release version (default)
  -v, --verbose     Show verbose output
  -j, --jobs <n>    Parallel compile jobs (default: auto-detect)
  -q, --qt-path <path>  Specify Qt installation path
  -p, --package     Create platform package after successful build
  --no-run          Skip "open program" prompt after build

Package Output:
  macOS:   release/LocalAIAssistant-x.x.x-macOS.dmg
  Windows: release/LocalAIAssistant-x.x.x-Windows.zip
  Linux:   release/LocalAIAssistant-x.x.x-Linux.tar.gz

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
  ./build.sh test                   # Build and run unit tests
  ./build.sh build -p               # Build and create platform package
  ./build.sh package                # Package existing build artifacts
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

    if [ "$COMMAND" == "test" ]; then
        cmd_test
        exit $?
    fi

    if [ "$COMMAND" == "package" ]; then
        cmd_package
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
        echo "❌ Dependency check failed. Install missing dependencies and retry."
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
        echo "❌ Build failed. Check error messages above."
        pause_if_interactive
        exit 1
    fi
}

main "$@"
