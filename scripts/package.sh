#!/bin/bash

# ============================================================
# LocalAIAssistant - Cross-Platform Packaging Script
# ============================================================
# Usage: ./package.sh [options]
#
# Options:
#   --build-dir <dir>       Build directory (default: build)
#   --qt-path <path>        Qt installation path (auto-detect if missing)
#   --output-dir <dir>      Output directory (default: release)
#   --nsis                  Create NSIS installer (Windows only, optional)
#   --appimage              Create AppImage (Linux only, optional)
#   --sign <identity>       Code sign macOS bundle (future — not implemented)
#   --notarize              Notarize macOS DMG (future — not implemented)
#   -h, --help              Show this help message
#
# Platform Output:
#   macOS:   release/LocalAIAssistant-{version}-macOS.dmg
#   Windows: release/LocalAIAssistant-{version}-Windows-x64.zip
#            release/LocalAIAssistant-{version}-Windows-x64-Setup.exe (with --nsis)
#   Linux:   release/LocalAIAssistant-{version}-Linux-x86_64.tar.gz
#            release/LocalAIAssistant-{version}-Linux-x86_64.AppImage (with --appimage)
#
# All outputs include a SHA256SUM file.
# ============================================================

set -euo pipefail

# --- Defaults ---
BUILD_DIR="build"
QT_PATH=""
OUTPUT_DIR="release"
CREATE_NSIS=false
CREATE_APPIMAGE=false
SIGN_IDENTITY=""
NOTARIZE=false

# --- Determine project root ---
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# --- Source shared version utility ---
if [ -f "$SCRIPT_DIR/version.sh" ]; then
    # shellcheck source=./version.sh
    source "$SCRIPT_DIR/version.sh"
else
    echo "Error: version.sh not found at $SCRIPT_DIR/version.sh"
    exit 1
fi

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
}

# ============================================================
# Qt Path Detection (needed for macdeployqt on macOS)
# ============================================================

detect_qt_path() {
    if [[ "$PLATFORM" == "macos" ]]; then
        if [ -d "$HOME/Qt" ]; then
            QT_VERSION=$(ls -1 "$HOME/Qt" 2>/dev/null | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | sort -V | tail -1)
            if [ -n "$QT_VERSION" ] && [ -d "$HOME/Qt/$QT_VERSION/macos" ]; then
                QT_PATH="$HOME/Qt/$QT_VERSION/macos"
                return 0
            fi
        fi
        if command -v brew &>/dev/null; then
            BREW_QT=$(brew --prefix qt@6 2>/dev/null)
            if [ -n "$BREW_QT" ] && [ -d "$BREW_QT" ]; then
                QT_PATH="$BREW_QT"
                return 0
            fi
        fi
    fi

    if [[ "$PLATFORM" == "linux" ]]; then
        if [ -d "$HOME/Qt" ]; then
            QT_VERSION=$(ls -1 "$HOME/Qt" 2>/dev/null | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | sort -V | tail -1)
            if [ -n "$QT_VERSION" ] && [ -d "$HOME/Qt/$QT_VERSION/gcc_64" ]; then
                QT_PATH="$HOME/Qt/$QT_VERSION/gcc_64"
                return 0
            fi
        fi
        if command -v qmake6 &>/dev/null; then
            QT_PATH=$(qmake6 -query QT_INSTALL_PREFIX 2>/dev/null)
            if [ -n "$QT_PATH" ] && [ -d "$QT_PATH" ]; then
                return 0
            fi
        fi
    fi

    return 1
}

# ============================================================
# Helpers
# ============================================================

find_tool() {
    # Usage: find_tool "display_name" "command_name"
    # Returns path if found, empty string otherwise.
    local name="$1"
    local cmd="$2"
    if command -v "$cmd" &>/dev/null; then
        command -v "$cmd"
    else
        echo ""
    fi
}

generate_sha256() {
    local out_dir="$1"
    echo ""
    echo "Generating SHA256 checksums..."

    if command -v shasum &>/dev/null; then
        (cd "$out_dir" && shasum -a 256 * 2>/dev/null >SHA256SUM)
        echo "  $out_dir/SHA256SUM"
    elif command -v sha256sum &>/dev/null; then
        (cd "$out_dir" && sha256sum * 2>/dev/null >SHA256SUM)
        echo "  $out_dir/SHA256SUM"
    else
        echo "  Warning: shasum/sha256sum not found, skipping checksums"
    fi
}

# ============================================================
# macOS Packaging
# ============================================================

package_macos() {
    local version
    version=$(get_version)
    local app_path="$BUILD_DIR/LocalAIAssistant.app"
    local cli_bin="$BUILD_DIR/LocalAIAssistant-CLI"
    local dmg_name="LocalAIAssistant-${version}-macOS.dmg"
    local macdeployqt

    echo ""
    echo "========================================="
    echo "  Packaging for macOS"
    echo "========================================="

    if [ ! -d "$app_path" ]; then
        echo "Error: $app_path not found. Build first with: cmake --build $BUILD_DIR"
        return 1
    fi

    rm -rf "$OUTPUT_DIR"
    mkdir -p "$OUTPUT_DIR"

    # --- Run macdeployqt ---
    # Auto-detect Qt path if needed
    if [ -z "$QT_PATH" ] || [ ! -d "$QT_PATH" ]; then
        detect_qt_path || true
    fi

    if [ -n "$QT_PATH" ] && [ -f "$QT_PATH/bin/macdeployqt" ]; then
        macdeployqt="$QT_PATH/bin/macdeployqt"
    else
        macdeployqt=$(find_tool "macdeployqt" "macdeployqt")
    fi

    if [ -n "$macdeployqt" ]; then
        echo ""
        echo "[1/4] Bundling Qt frameworks with macdeployqt..."
        "$macdeployqt" "$app_path" -verbose=1 -no-strip 2>&1 | sed 's/^/  /' \
            || echo "  Warning: macdeployqt reported issues, continuing anyway..."

        # Re-sign with ad-hoc signature (macdeployqt invalidates the original)
        echo ""
        echo "[2/4] Re-signing app bundle (ad-hoc)..."
        codesign --force --deep --sign - "$app_path" 2>&1 | sed 's/^/  /' \
            || echo "  Warning: ad-hoc signing failed, app may not launch"
    else
        echo ""
        echo "[1/4] macdeployqt not found — Qt frameworks will NOT be bundled."
        echo "  The app will only run on machines with Qt installed."
    fi

    # --- Create DMG staging directory ---
    echo ""
    echo "[3/4] Creating DMG staging directory..."
    local staging="$OUTPUT_DIR/staging"
    mkdir -p "$staging"
    cp -R "$app_path" "$staging/"

    # Remove developer .env from app bundle (avoid credential leakage)
    rm -f "$staging/LocalAIAssistant.app/Contents/Resources/.env" 2>/dev/null || true
    echo "  .env removed from app bundle (use Settings UI or .env.example)"

    # Copy .env.example as user template
    if [ -f "$PROJECT_ROOT/.env.example" ]; then
        cp "$PROJECT_ROOT/.env.example" "$staging/.env.example"
        echo "  .env.example template included"
    fi

    # Set custom DMG volume icon
    if [ -f "$PROJECT_ROOT/resources/icons/app.icns" ]; then
        cp "$PROJECT_ROOT/resources/icons/app.icns" "$staging/.VolumeIcon.icns"
        SetFile -a C "$staging" 2>/dev/null || true
        echo "  DMG volume icon set"
    fi

    # Applications folder symlink (drag-to-install)
    ln -s /Applications "$staging/Applications"

    # Copy CLI binary alongside .app
    if [ -f "$cli_bin" ]; then
        cp "$cli_bin" "$staging/"
        echo "  CLI binary included"
    fi

    # --- Optional code signing ---
    if [ -n "$SIGN_IDENTITY" ]; then
        echo ""
        echo "  Code signing is not yet implemented."
        echo "  This feature requires an Apple Developer ID certificate."
        echo "  See docs/CODE_SIGNING.md for manual signing instructions."
        echo "  Continuing with unsigned DMG..."
    fi

    # --- Optional notarization ---
    if [ "$NOTARIZE" = true ]; then
        echo ""
        echo "  Notarization is not yet implemented."
        echo "  This feature requires an Apple Developer account."
        echo "  See docs/CODE_SIGNING.md for manual notarization instructions."
        echo "  Continuing without notarization..."
    fi

    # --- Create DMG ---
    echo ""
    echo "[4/4] Creating DMG..."
    local dmg_path="$OUTPUT_DIR/$dmg_name"
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

# ============================================================
# Windows Packaging
# ============================================================

package_windows() {
    local version
    version=$(get_version)
    local gui_exe="$BUILD_DIR/LocalAIAssistant.exe"
    local cli_exe="$BUILD_DIR/LocalAIAssistant-CLI.exe"
    local zip_name="LocalAIAssistant-${version}-Windows-x64.zip"
    local windeployqt

    echo ""
    echo "========================================="
    echo "  Packaging for Windows"
    echo "========================================="

    if [ ! -f "$gui_exe" ] && [ ! -f "$cli_exe" ]; then
        echo "Error: No executables found in $BUILD_DIR. Build first with: cmake --build $BUILD_DIR"
        return 1
    fi

    rm -rf "$OUTPUT_DIR"
    mkdir -p "$OUTPUT_DIR"

    # --- Find windeployqt ---
    windeployqt=$(find_tool "windeployqt" "windeployqt")
    if [ -z "$windeployqt" ] && [ -n "$QT_PATH" ] && [ -f "$QT_PATH/bin/windeployqt.exe" ]; then
        windeployqt="$QT_PATH/bin/windeployqt.exe"
    fi

    # --- Run windeployqt (self-sufficient: does not depend on prior build step) ---
    if [ -n "$windeployqt" ]; then
        echo ""
        echo "[1/3] Deploying Qt DLLs with windeployqt..."
        if [ -f "$gui_exe" ]; then
            "$windeployqt" --no-translations "$gui_exe" 2>&1 | sed 's/^/  /'
            echo "  GUI: Qt DLLs deployed"
        fi
        if [ -f "$cli_exe" ]; then
            "$windeployqt" --no-translations "$cli_exe" 2>&1 | sed 's/^/  /'
            echo "  CLI: Qt DLLs deployed"
        fi
    else
        echo ""
        echo "[1/3] windeployqt not found — Qt DLLs will NOT be bundled."
        echo "  The app will only run on machines with Qt installed."
    fi

    # --- Collect files ---
    echo ""
    echo "[2/3] Collecting files..."
    local staging="$OUTPUT_DIR/staging"
    mkdir -p "$staging"

    # Copy executables and all adjacent DLLs (the bug fix)
    if [ -f "$gui_exe" ]; then
        cp "$BUILD_DIR"/*.exe "$staging/" 2>/dev/null || true
        cp "$BUILD_DIR"/*.dll "$staging/" 2>/dev/null || true
        cp "$BUILD_DIR"/*.ico "$staging/" 2>/dev/null || true
        # Copy Qt plugin directories deployed by windeployqt
        for dir in platforms styles sqldrivers tls networkinformation multimedia iconengines imageformats generic; do
            if [ -d "$BUILD_DIR/$dir" ]; then
                cp -R "$BUILD_DIR/$dir" "$staging/"
            fi
        done
        echo "  Executables and DLLs copied"
    fi

    # Copy resource directories from build
    for dir in core AIGirlfriend girlfriend translations models prompts; do
        if [ -d "$BUILD_DIR/$dir" ]; then
            cp -R "$BUILD_DIR/$dir" "$staging/"
            echo "  $dir/ copied"
        fi
    done

    # Copy individual resource files
    for file in soul.md personality.md memory.md; do
        if [ -f "$BUILD_DIR/$file" ]; then
            cp "$BUILD_DIR/$file" "$staging/"
        fi
    done

    # Copy .env.example and auto-create .env
    if [ -f "$PROJECT_ROOT/.env.example" ]; then
        cp "$PROJECT_ROOT/.env.example" "$staging/.env.example"
        cp "$PROJECT_ROOT/.env.example" "$staging/.env"
        echo "  .env.example template and .env created"
    fi

    # Copy docs
    if [ -d "$PROJECT_ROOT/docs" ]; then
        mkdir -p "$staging/docs"
        cp "$PROJECT_ROOT/docs/USAGE.md" "$staging/docs/" 2>/dev/null || true
        cp "$PROJECT_ROOT/docs/USAGE_zh_CN.md" "$staging/docs/" 2>/dev/null || true
        echo "  docs/ copied"
    fi

    # --- Create ZIP ---
    echo ""
    echo "[3/3] Creating zip archive..."
    local zip_path="$OUTPUT_DIR/$zip_name"

    cd "$OUTPUT_DIR"
    if command -v zip &>/dev/null; then
        zip -rq "$zip_name" "staging"
    elif command -v powershell &>/dev/null; then
        powershell -Command "Compress-Archive -Path staging\* -DestinationPath $zip_name -Force"
    else
        echo "  Error: neither 'zip' nor 'powershell' found"
        cd "$PROJECT_ROOT"
        return 1
    fi
    echo "  === Windows package created ==="
    echo "  $OUTPUT_DIR/$zip_name"
    cd "$PROJECT_ROOT"

    # --- Optional NSIS installer ---
    if [ "$CREATE_NSIS" = true ]; then
        create_nsis_installer "$version"
    fi

    rm -rf "$staging"
}

create_nsis_installer() {
    local version="$1"
    local nsis_template="$PROJECT_ROOT/resources/installer/installer.nsi.in"
    local nsi_file="$OUTPUT_DIR/installer.nsi"
    local makensis_bin
    makensis_bin=$(find_tool "makensis" "makensis")

    if [ ! -f "$nsis_template" ]; then
        echo "  Warning: NSIS template not found at $nsis_template, skipping installer"
        return 0
    fi

    if [ -z "$makensis_bin" ]; then
        echo ""
        echo "  Warning: makensis not found, skipping NSIS installer."
        echo "  To create Windows installer, install NSIS:"
        echo "    macOS:   brew install makensis"
        echo "    Linux:   apt install nsis"
        echo "    Windows: choco install nsis"
        echo "  ZIP archive is available as fallback."
        return 0
    fi

    echo ""
    echo "--- Creating NSIS installer ---"

    # Create staging directory with flat structure for NSIS File /r
    local nsis_staging="$OUTPUT_DIR/nsis_staging"
    rm -rf "$nsis_staging"
    mkdir -p "$nsis_staging"

    # Collect all files into the NSIS staging dir (from build dir, re-collect)
    cp "$BUILD_DIR"/*.exe "$nsis_staging/" 2>/dev/null || true
    cp "$BUILD_DIR"/*.dll "$nsis_staging/" 2>/dev/null || true
    cp "$BUILD_DIR"/*.ico "$nsis_staging/" 2>/dev/null || true
    # Copy Qt plugin directories deployed by windeployqt
    for dir in platforms styles sqldrivers tls networkinformation multimedia iconengines imageformats generic; do
        if [ -d "$BUILD_DIR/$dir" ]; then
            cp -R "$BUILD_DIR/$dir" "$nsis_staging/"
        fi
    done
    for dir in core AIGirlfriend girlfriend translations models prompts; do
        if [ -d "$BUILD_DIR/$dir" ]; then
            cp -R "$BUILD_DIR/$dir" "$nsis_staging/"
        fi
    done
    for file in soul.md personality.md memory.md; do
        if [ -f "$BUILD_DIR/$file" ]; then
            cp "$BUILD_DIR/$file" "$nsis_staging/"
        fi
    done
    if [ -f "$PROJECT_ROOT/.env.example" ]; then
        cp "$PROJECT_ROOT/.env.example" "$nsis_staging/.env.example"
    fi
    if [ -d "$PROJECT_ROOT/docs" ]; then
        mkdir -p "$nsis_staging/docs"
        cp "$PROJECT_ROOT/docs/USAGE.md" "$nsis_staging/docs/" 2>/dev/null || true
        cp "$PROJECT_ROOT/docs/USAGE_zh_CN.md" "$nsis_staging/docs/" 2>/dev/null || true
    fi

    # Substitute version in template
    sed "s/@VERSION@/$version/g" "$nsis_template" >"$nsi_file"

    # Build installer. NSIS File /r uses relative paths, so cd to output dir
    # and rename nsis_staging to "staging" so the template's "File /r staging\*" works.
    cd "$OUTPUT_DIR"
    rm -rf staging
    mv nsis_staging staging

    echo "  Running makensis..."
    "$makensis_bin" -V2 "$nsi_file" 2>&1 | sed 's/^/  /'

    local setup_exe="LocalAIAssistant-${version}-Windows-x64-Setup.exe"
    if [ -f "$OUTPUT_DIR/$setup_exe" ]; then
        echo "  === NSIS installer created ==="
        echo "  $OUTPUT_DIR/$setup_exe"
    else
        echo "  Warning: NSIS installer was not created (check makensis output)"
    fi

    rm -rf staging "$nsi_file"
    cd "$PROJECT_ROOT"
}

# ============================================================
# Linux Packaging
# ============================================================

package_linux() {
    local version
    version=$(get_version)
    local gui_bin="$BUILD_DIR/LocalAIAssistant"
    local cli_bin="$BUILD_DIR/LocalAIAssistant-CLI"
    local archive_name="LocalAIAssistant-${version}-Linux-x86_64.tar.gz"

    echo ""
    echo "========================================="
    echo "  Packaging for Linux"
    echo "========================================="

    if [ ! -f "$gui_bin" ] && [ ! -f "$cli_bin" ]; then
        echo "Error: No executables found in $BUILD_DIR. Build first with: cmake --build $BUILD_DIR"
        return 1
    fi

    rm -rf "$OUTPUT_DIR"
    mkdir -p "$OUTPUT_DIR"

    local staging="$OUTPUT_DIR/LocalAIAssistant-${version}"
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

    # --- Bundle Qt libraries with linuxdeployqt ---
    local linuxdeployqt
    linuxdeployqt=$(find_tool "linuxdeployqt" "linuxdeployqt")

    if [ -n "$linuxdeployqt" ] && [ -f "$staging/LocalAIAssistant" ]; then
        echo ""
        echo "  Bundling Qt libraries with linuxdeployqt..."
        # linuxdeployqt works best with a .desktop file; create a minimal one if needed
        if [ -f "$PROJECT_ROOT/resources/localaiassistant.desktop" ]; then
            cp "$PROJECT_ROOT/resources/localaiassistant.desktop" "$staging/"
        fi
        "$linuxdeployqt" "$staging/LocalAIAssistant" -verbose=1 -no-strip \
            -bundle-non-qt-libs 2>&1 | sed 's/^/  /' \
            || echo "  Warning: linuxdeployqt reported issues, continuing anyway..."
        echo "  Qt libraries bundled"
    elif [ -f "$staging/LocalAIAssistant" ]; then
        echo ""
        echo "  linuxdeployqt not found — Qt libraries will NOT be bundled."
        echo "  The GUI requires Qt6 to be installed on the target system."
    fi

    # Copy resource directories
    for dir in core AIGirlfriend girlfriend translations models prompts; do
        if [ -d "$BUILD_DIR/$dir" ]; then
            cp -R "$BUILD_DIR/$dir" "$staging/"
            echo "  $dir/ copied"
        fi
    done

    # Copy individual resource files
    for file in soul.md personality.md memory.md; do
        if [ -f "$BUILD_DIR/$file" ]; then
            cp "$BUILD_DIR/$file" "$staging/"
        fi
    done

    # Copy .env.example
    if [ -f "$PROJECT_ROOT/.env.example" ]; then
        cp "$PROJECT_ROOT/.env.example" "$staging/.env.example"
        echo "  .env.example template included"
    fi

    # Copy docs
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

    # Create embedded install script
    cat >"$staging/install.sh" <<'INSTALL_SCRIPT'
#!/bin/bash
set -euo pipefail

# ── Dependency detection ──────────────────────────────────────────
detect_distro() {
    if command -v apt-get &>/dev/null; then
        echo "apt"
    elif command -v dnf &>/dev/null; then
        echo "dnf"
    elif command -v pacman &>/dev/null; then
        echo "pacman"
    else
        echo "unknown"
    fi
}

# ── Install system runtime dependencies ───────────────────────────
install_system_deps() {
    local distro
    distro=$(detect_distro)
    echo "Detected distribution: $distro"
    echo "Checking runtime dependencies..."

    # Quick probe: if libQt6Core is already in the linker cache, deps are installed
    if ldconfig -p 2>/dev/null | grep -q libQt6Core; then
        echo "  Qt6 runtime libraries already present, skipping."
        return 0
    fi

    echo "Installing Qt6 runtime dependencies..."
    echo "(You may be prompted for your sudo password.)"
    case "$distro" in
        apt)
            sudo apt-get update -qq
            sudo apt-get install -y qt6-base-dev qt6-multimedia-dev qt6-websockets-dev \
                libpoppler-cpp-dev libzip-dev libpugixml-dev 2>/dev/null || true
            ;;
        dnf)
            sudo dnf install -y qt6-qtbase qt6-qtmultimedia qt6-qtwebsockets \
                poppler-cpp libzip pugixml 2>/dev/null || true
            ;;
        pacman)
            sudo pacman -S --noconfirm qt6-base qt6-multimedia qt6-websockets \
                poppler libzip pugixml 2>/dev/null || true
            ;;
        *)
            echo "Warning: Could not detect package manager."
            echo "Please install dependencies manually:"
            echo "  Qt6 (base, multimedia, websockets), Poppler, libzip, pugixml"
            ;;
    esac
}

# ── Fix library soname symlinks for version mismatches ────────────
fix_library_links() {
    echo "Checking library compatibility..."
    local libdirs=("/usr/lib" "/usr/lib/x86_64-linux-gnu" "/usr/lib64" "/usr/local/lib")
    local libs_to_check="libpoppler-cpp.so.0|libpoppler-cpp.so libzip.so.4|libzip.so"

    for entry in $libs_to_check; do
        local expected="${entry%%|*}"
        local actual_base="${entry##*|}"
        for libdir in "${libdirs[@]}"; do
            [ -d "$libdir" ] || continue
            [ -f "$libdir/$expected" ] && continue
            local actual
            actual=$(find "$libdir" -maxdepth 1 -name "${actual_base}*" -type f 2>/dev/null | head -1)
            if [ -n "$actual" ] && [ "$(basename "$actual")" != "$expected" ]; then
                echo "  Creating symlink: $expected -> $(basename "$actual")"
                sudo ln -sf "$(basename "$actual")" "$libdir/$expected" 2>/dev/null || \
                    echo "  (Could not create symlink; may need sudo)"
            fi
        done
    done
}

# ── Detect WSL / headless environment ─────────────────────────────
HAS_DISPLAY=true
detect_display() {
    if [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
        HAS_DISPLAY=false
        echo ""
        echo "WARNING: No graphical display detected."
        echo "  (\$DISPLAY and \$WAYLAND_DISPLAY are unset)"
        echo "If you are in WSL or a headless server, use the CLI version:"
        echo "  LocalAIAssistant-CLI chat"
        echo ""
        echo "To enable GUI support in WSL:"
        echo "  1. Install an X server on Windows (VcXsrv, X410, etc.)"
        echo "  2. Run: export DISPLAY=:0"
        echo "  3. Or use WSLg if your distro supports it"
        echo ""
    fi
}

# ── Main ──────────────────────────────────────────────────────────
echo "========================================"
echo "  LocalAIAssistant Installer"
echo "========================================"

detect_display
install_system_deps
fix_library_links

INSTALL_DIR="${HOME}/.local"
echo ""
echo "Installing to $INSTALL_DIR..."

mkdir -p "$INSTALL_DIR/bin"
mkdir -p "$INSTALL_DIR/share/localaiassistant"
mkdir -p "$INSTALL_DIR/share/applications"
mkdir -p "$INSTALL_DIR/share/icons/hicolor/256x256/apps"

# Binaries
cp LocalAIAssistant "$INSTALL_DIR/bin/" 2>/dev/null && echo "  GUI binary installed" || true
cp LocalAIAssistant-CLI "$INSTALL_DIR/bin/" 2>/dev/null && echo "  CLI binary installed" || true

# Resource directories
for dir in core AIGirlfriend girlfriend translations models prompts; do
    if [ -d "$dir" ]; then
        cp -R "$dir" "$INSTALL_DIR/share/localaiassistant/"
        echo "  $dir/ copied"
    fi
done

# Individual resource files
for file in soul.md personality.md memory.md; do
    if [ -f "$file" ]; then
        cp "$file" "$INSTALL_DIR/share/localaiassistant/"
        echo "  $file copied"
    fi
done

# .env.example and auto-create .env
if [ -f ".env.example" ]; then
    cp ".env.example" "$INSTALL_DIR/share/localaiassistant/.env.example"
    echo "  .env.example template copied"
    if [ ! -f "$INSTALL_DIR/share/localaiassistant/.env" ]; then
        cp ".env.example" "$INSTALL_DIR/share/localaiassistant/.env"
        echo "  .env created from .env.example — edit it with your API settings"
    fi
fi

# Docs
if [ -d "docs" ]; then
    mkdir -p "$INSTALL_DIR/share/localaiassistant/docs"
    cp docs/*.md "$INSTALL_DIR/share/localaiassistant/docs/" 2>/dev/null || true
    echo "  docs/ copied"
fi

# Desktop file
if [ -f "localaiassistant.desktop" ]; then
    sed -i.bak "s|^Exec=.*|Exec=$INSTALL_DIR/bin/LocalAIAssistant|" localaiassistant.desktop
    rm -f localaiassistant.desktop.bak
    cp localaiassistant.desktop "$INSTALL_DIR/share/applications/"
    echo "  .desktop file installed"
fi

echo ""
echo "========================================"
echo "  Installation complete!"
echo "========================================"
echo ""
echo "To run:"
echo "  GUI:  LocalAIAssistant"
echo "  CLI:  LocalAIAssistant-CLI chat"
echo ""
echo "Add to PATH (add to ~/.bashrc or ~/.zshrc):"
echo "  export PATH=\"\$HOME/.local/bin:\$PATH\""
echo ""
if [ "$HAS_DISPLAY" = false ]; then
    echo "NOTE: No display detected. Use CLI mode only."
fi
echo "NOTE: Edit $INSTALL_DIR/share/localaiassistant/.env to configure AI and voice services."
echo ""
INSTALL_SCRIPT
    chmod +x "$staging/install.sh"
    echo "  install.sh created"

    # Create tar.gz
    echo ""
    echo "[2/2] Creating tar.gz archive..."

    cd "$OUTPUT_DIR"
    tar -czf "$archive_name" "LocalAIAssistant-${version}"
    echo "  === Linux package created ==="
    echo "  $OUTPUT_DIR/$archive_name"
    cd "$PROJECT_ROOT"

    # --- Optional AppImage ---
    if [ "$CREATE_APPIMAGE" = true ]; then
        create_appimage "$version"
    fi

    rm -rf "$staging"
}

create_appimage() {
    local version="$1"
    local appdir="$OUTPUT_DIR/LocalAIAssistant.AppDir"
    local linuxdeployqt_bin
    local appimagetool_bin

    linuxdeployqt_bin=$(find_tool "linuxdeployqt" "linuxdeployqt")
    appimagetool_bin=$(find_tool "appimagetool" "appimagetool")

    if [ -z "$linuxdeployqt_bin" ]; then
        echo ""
        echo "  Warning: linuxdeployqt not found, skipping AppImage."
        echo "  Download from: https://github.com/probonopd/linuxdeployqt/releases"
        echo "  tar.gz archive is available as fallback."
    fi

    if [ -z "$appimagetool_bin" ]; then
        echo ""
        echo "  Warning: appimagetool not found, skipping AppImage."
        echo "  Download from: https://github.com/AppImage/AppImageKit/releases"
        echo "  tar.gz archive is available as fallback."
    fi

    if [ -z "$linuxdeployqt_bin" ] || [ -z "$appimagetool_bin" ]; then
        return 0
    fi

    echo ""
    echo "--- Creating AppImage ---"

    # Create AppDir structure
    rm -rf "$appdir"
    mkdir -p "$appdir/usr/bin"
    mkdir -p "$appdir/usr/share/applications"
    mkdir -p "$appdir/usr/share/icons/hicolor/256x256/apps"

    # Copy binaries
    cp "$BUILD_DIR/LocalAIAssistant" "$appdir/usr/bin/" 2>/dev/null || true
    cp "$BUILD_DIR/LocalAIAssistant-CLI" "$appdir/usr/bin/" 2>/dev/null || true

    # Copy resources
    for dir in core AIGirlfriend girlfriend translations models prompts; do
        if [ -d "$BUILD_DIR/$dir" ]; then
            mkdir -p "$appdir/usr/$dir"
            cp -R "$BUILD_DIR/$dir/"* "$appdir/usr/$dir/" 2>/dev/null || true
        fi
    done
    for file in soul.md personality.md memory.md; do
        if [ -f "$BUILD_DIR/$file" ]; then
            cp "$BUILD_DIR/$file" "$appdir/usr/" 2>/dev/null || true
        fi
    done

    # Copy desktop file and icon
    if [ -f "$PROJECT_ROOT/resources/localaiassistant.desktop" ]; then
        cp "$PROJECT_ROOT/resources/localaiassistant.desktop" "$appdir/usr/share/applications/"
        cp "$PROJECT_ROOT/resources/localaiassistant.desktop" "$appdir/"
    fi
    if [ -f "$PROJECT_ROOT/resources/icons/app.png" ]; then
        cp "$PROJECT_ROOT/resources/icons/app.png" \
            "$appdir/usr/share/icons/hicolor/256x256/apps/localaiassistant.png"
        cp "$PROJECT_ROOT/resources/icons/app.png" "$appdir/localaiassistant.png"
    fi

    # Run linuxdeployqt
    echo "  Running linuxdeployqt..."
    "$linuxdeployqt_bin" "$appdir/usr/share/applications/localaiassistant.desktop" \
        -verbose=1 -no-strip -bundle-non-qt-libs 2>&1 | sed 's/^/  /' || true

    # Run appimagetool
    echo "  Running appimagetool..."
    local appimage_path="$OUTPUT_DIR/LocalAIAssistant-${version}-Linux-x86_64.AppImage"
    "$appimagetool_bin" "$appdir" "$appimage_path" 2>&1 | sed 's/^/  /'

    if [ -f "$appimage_path" ]; then
        chmod +x "$appimage_path"
        echo "  === AppImage created ==="
        echo "  $appimage_path"
    else
        echo "  Warning: AppImage creation failed, check tool output above"
    fi

    rm -rf "$appdir"
}

# ============================================================
# Usage
# ============================================================

usage() {
    cat <<EOF
LocalAIAssistant - Cross-Platform Packaging Script

Usage: ./package.sh [options]

Options:
  --build-dir <dir>       Build directory containing compiled artifacts
                          (default: build)
  --qt-path <path>        Qt installation path for macdeployqt/windeployqt
                          (auto-detected if missing)
  --output-dir <dir>      Directory for output artifacts (default: release)
  --nsis                  Create NSIS installer on Windows (optional)
  --appimage              Create AppImage on Linux (optional)
  --sign <identity>       Code sign macOS bundle (not yet implemented)
  --notarize              Notarize macOS DMG (not yet implemented)
  -h, --help              Show this help message

Platform Output:
  macOS:   LocalAIAssistant-{version}-macOS.dmg
  Windows: LocalAIAssistant-{version}-Windows-x64.zip
           LocalAIAssistant-{version}-Windows-x64-Setup.exe (with --nsis)
  Linux:   LocalAIAssistant-{version}-Linux-x86_64.tar.gz
           LocalAIAssistant-{version}-Linux-x86_64.AppImage (with --appimage)

All outputs include SHA256SUM checksums.

Examples:
  ./package.sh                                    # Auto-detect platform, create archive
  ./package.sh --build-dir build --output-dir release
  ./package.sh --nsis                             # Windows: also create NSIS installer
  ./package.sh --appimage                         # Linux: also create AppImage

Notes:
  - NSIS and AppImage features are optional. If the required tools are not
    found, the script falls back to the basic archive format.
  - Code signing and notarization are future features (not yet implemented).
EOF
}

# ============================================================
# Argument Parsing
# ============================================================

parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h | --help)
                usage
                exit 0
                ;;
            --build-dir)
                BUILD_DIR="$2"
                shift 2
                ;;
            --qt-path)
                QT_PATH="$2"
                shift 2
                ;;
            --output-dir)
                OUTPUT_DIR="$2"
                shift 2
                ;;
            --nsis)
                CREATE_NSIS=true
                shift
                ;;
            --appimage)
                CREATE_APPIMAGE=true
                shift
                ;;
            --sign)
                SIGN_IDENTITY="$2"
                shift 2
                ;;
            --notarize)
                NOTARIZE=true
                shift
                ;;
            *)
                echo "Error: Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
}

# ============================================================
# Main
# ============================================================

main() {
    parse_args "$@"

    detect_platform

    local version
    version=$(get_version)

    echo ""
    echo "==================================="
    echo "  LocalAIAssistant - Package"
    echo "  Version:  $version"
    echo "  Platform: $PLATFORM"
    echo "  Output:   $OUTPUT_DIR"
    echo "==================================="

    # Verify build directory exists
    if [ ! -d "$BUILD_DIR" ]; then
        echo "Error: Build directory '$BUILD_DIR' not found."
        echo "Build the project first: cmake --build $BUILD_DIR"
        exit 1
    fi

    # Dispatch to platform-specific packaging
    case "$PLATFORM" in
        macos)
            # Auto-detect Qt path on macOS (needed for macdeployqt)
            if [ -z "$QT_PATH" ] || [ ! -d "$QT_PATH" ]; then
                detect_qt_path || true
            fi
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
            echo "This script supports: macOS, Windows (Git Bash/MSYS2), Linux"
            exit 1
            ;;
    esac

    # Generate checksums
    generate_sha256 "$OUTPUT_DIR"

    echo ""
    echo "Packaging complete. Artifacts in $OUTPUT_DIR/"
}

main "$@"
