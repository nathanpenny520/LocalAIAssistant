#!/bin/bash

# ============================================================
# LocalAIAssistant - First-time Setup Script
# ============================================================
# Run this after cloning the repository for the first time
# Usage: ./scripts/setup.sh
# ============================================================

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# Pause function for interactive terminal
pause_if_interactive() {
    if [ -t 0 ]; then
        echo ""
        read -p "Press Enter to exit..." -r
    fi
}

# Trap errors to pause before exit
trap 'echo ""; echo "❌ Script execution error"; pause_if_interactive; exit 1' ERR

echo ""
echo "==================================="
echo "  LocalAIAssistant - Initial Setup"
echo "==================================="
echo ""

# ------------------------------------------------------------
# Step 1: Create .env file from template
# ------------------------------------------------------------

echo "[Step 1] Configure iFLYTEK voice credentials"
echo ""

if [ -f ".env" ]; then
    echo "  ✅ .env file already exists"
else
    if [ -f ".env.example" ]; then
        echo "  Copying .env.example -> .env"
        cp .env.example .env
        echo ""
        echo "  ⚠️  Edit .env file and fill in your iFLYTEK voice credentials:"
        echo ""
        echo "      XFYUN_APP_ID=your_app_id"
        echo "      XFYUN_API_KEY=your_api_key"
        echo "      XFYUN_API_SECRET=your_api_secret"
        echo ""
        echo "  Get credentials: https://www.xfyun.cn"
        echo "  Required services: Voice Dictation (streaming) + Ultra-realistic TTS"
        echo ""
    else
        echo "  ❌ .env.example file not found, check project integrity"
    fi
fi

# ------------------------------------------------------------
# Step 2: Check build dependencies
# ------------------------------------------------------------

echo "[Step 2] Check build dependencies"
echo ""

missing_deps=()

# Check CMake
if command -v cmake &> /dev/null; then
    cmake_version=$(cmake --version | head -1)
    echo "  ✅ CMake: $cmake_version"
else
    echo "  ❌ CMake not installed"
    missing_deps+=("cmake")
fi

# Check C++ compiler
# Note: Qt Creator is an IDE, NOT a compiler.
# - Windows MinGW: Qt installation includes g++ compiler (in Qt/Tools/mingwXXX_64/bin)
# - Windows MSVC: Requires Visual Studio (includes cl compiler)
# - macOS: Requires Xcode Command Line Tools (includes clang++)
# - Linux: Requires GCC (g++)

# On Windows, MinGW g++ is NOT in PATH by default, need to search Qt Tools directory
found_compiler=false

if command -v g++ &> /dev/null; then
    gpp_version=$(g++ --version | head -1)
    echo "  ✅ Compiler: $gpp_version"
    found_compiler=true
elif command -v clang++ &> /dev/null; then
    clang_version=$(clang++ --version | head -1)
    echo "  ✅ Compiler: $clang_version"
    found_compiler=true
elif command -v cl &> /dev/null; then
    echo "  ✅ Compiler: MSVC cl"
    found_compiler=true
fi

# Windows: Try to find MinGW in Qt Tools directory (not in PATH by default)
if [ "$found_compiler" = false ] && [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then
    mingw_versions=("1310" "1120" "1110" "100" "90" "81" "73")
    for drive in "c" "d" "e"; do
        for ver in "${mingw_versions[@]}"; do
            mingw_path="/$drive/Qt/Tools/mingw${ver}_64/bin/g++.exe"
            if [ -f "$mingw_path" ]; then
                echo "  ✅ Compiler: MinGW g++ (detected but not in PATH)"
                echo "     Location: $mingw_path"
                echo "     Note: build.sh will auto-add it to PATH"
                found_compiler=true
                break 2
            fi
        done
    done
fi

if [ "$found_compiler" = false ]; then
    echo "  ❌ C++ compiler not installed"
    echo "     Note: Qt Creator is an IDE, not a compiler"
    case "$OSTYPE" in
        msys*|cygwin*|win32*)
            echo "     Windows: Install MinGW Qt (includes g++) or Visual Studio"
            ;;
        darwin*)
            echo "     macOS: Run xcode-select --install"
            ;;
        linux*)
            echo "     Linux: Run sudo apt install build-essential"
            ;;
    esac
    missing_deps+=("C++ compiler")
fi

# Check Qt (basic check)
echo ""
echo "  Qt 6 check:"

# macOS/Linux: Check ~/Qt directory (official Qt installation)
if [ -d "$HOME/Qt" ]; then
    qt_versions=$(ls -1 "$HOME/Qt" 2>/dev/null | grep -E '^[0-9]+\.[0-9]+' | head -3)
    if [ -n "$qt_versions" ]; then
        echo "  ✅ Qt installed at ~/Qt/"
        echo "     Versions: $qt_versions"
        echo "  ⚠️  Ensure Multimedia and WebSockets modules are installed (required for voice)"
    else
        echo "  ⚠️  ~/Qt directory exists but no Qt versions found"
    fi
# macOS/Linux: Check system package manager
elif command -v qmake6 &> /dev/null || command -v qmake &> /dev/null; then
    echo "  ✅ Qt installed (system package manager)"
    # Check Multimedia and WebSockets on Linux
    if [[ "$OSTYPE" == "linux"* ]]; then
        # Debian/Ubuntu (dpkg)
        if command -v dpkg &> /dev/null; then
            if dpkg -l qt6-multimedia-dev &> /dev/null 2>&1; then
                echo "  ✅ Multimedia module installed"
            else
                echo "  ⚠️  Install qt6-multimedia-dev (required for voice)"
            fi
            if dpkg -l qt6-websockets-dev &> /dev/null 2>&1; then
                echo "  ✅ WebSockets module installed"
            else
                echo "  ⚠️  Install qt6-websockets-dev (required for voice)"
            fi
        # Fedora/RHEL (rpm)
        elif command -v rpm &> /dev/null; then
            if rpm -q qt6-qtmultimedia-devel &> /dev/null 2>&1; then
                echo "  ✅ Multimedia module installed"
            else
                echo "  ⚠️  Install qt6-qtmultimedia-devel (required for voice)"
            fi
            if rpm -q qt6-qtwebsockets-devel &> /dev/null 2>&1; then
                echo "  ✅ WebSockets module installed"
            else
                echo "  ⚠️  Install qt6-qtwebsockets-devel (required for voice)"
            fi
        # Arch Linux (pacman)
        elif command -v pacman &> /dev/null; then
            if pacman -Q qt6-multimedia &> /dev/null 2>&1; then
                echo "  ✅ Multimedia module installed"
            else
                echo "  ⚠️  Install qt6-multimedia (required for voice)"
            fi
            if pacman -Q qt6-websockets &> /dev/null 2>&1; then
                echo "  ✅ WebSockets module installed"
            else
                echo "  ⚠️  Install qt6-websockets (required for voice)"
            fi
        else
            echo "  ⚠️  Cannot detect Qt modules, manually verify Multimedia and WebSockets"
        fi
    fi
# Windows: Check Qt installation directory
elif [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then
    found_qt=false
    qt_compilers=("mingw_64" "msvc2019_64" "msvc2022_64")

    for drive in "c" "d" "e"; do
        qt_root="/$drive/Qt"
        if [ -d "$qt_root" ]; then
            # Dynamically find Qt versions (directories starting with 6.)
            for version_dir in "$qt_root"/*; do
                if [ -d "$version_dir" ]; then
                    version_name=$(basename "$version_dir")
                    # Check if it's a version directory (starts with number)
                    if [[ "$version_name" =~ ^[0-9]+\.[0-9]+ ]]; then
                        for compiler in "${qt_compilers[@]}"; do
                            qt_path="$version_dir/$compiler"
                            if [ -d "$qt_path" ]; then
                                echo "  ✅ Qt installed at /$drive/Qt/"
                                echo "     Version: $version_name"
                                echo "     Compiler: $compiler"
                                echo "     Path: $qt_path"
                                echo "  ⚠️  Ensure Multimedia and WebSockets are selected in Qt Maintenance Tool"
                                found_qt=true
                                break 3
                            fi
                        done
                    fi
                fi
            done
        fi
    done

    if [ "$found_qt" = false ]; then
        echo "  ❌ Qt 6 not installed"
        missing_deps+=("Qt 6 (+ Multimedia + WebSockets)")
    fi
else
    echo "  ❌ Qt 6 not installed"
    missing_deps+=("Qt 6 (+ Multimedia + WebSockets)")
fi

# Check Poppler (optional)
echo ""
echo "  Poppler (PDF support, optional):"
poppler_installed=false
if command -v pkg-config &> /dev/null && pkg-config --exists poppler-cpp 2>/dev/null; then
    echo "  ✅ Poppler installed"
    poppler_installed=true
elif [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then
    # Windows: Check MSYS2 poppler
    if command -v pacman &> /dev/null && pacman -Q poppler &> /dev/null 2>&1; then
        echo "  ✅ Poppler installed (MSYS2)"
        poppler_installed=true
    fi
fi

if [ "$poppler_installed" = false ]; then
    echo "  ⚠️  Poppler not installed, PDF support will be disabled"
    echo "     Note: Poppler is optional, other features work without it"
fi

# Check libzip (optional, enables DOCX parsing)
echo ""
echo "  libzip (DOCX support, optional):"
libzip_installed=false
if command -v pkg-config &> /dev/null && pkg-config --exists libzip 2>/dev/null; then
    echo "  ✅ libzip installed"
    libzip_installed=true
elif [ -f "/usr/local/lib/libzip.dylib" ] || [ -f "/opt/homebrew/lib/libzip.dylib" ]; then
    echo "  ✅ libzip installed (Homebrew)"
    libzip_installed=true
elif [ -f "/usr/lib/x86_64-linux-gnu/libzip.so" ] || [ -f "/usr/lib/libzip.so" ]; then
    echo "  ✅ libzip installed (system)"
    libzip_installed=true
elif [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then
    if command -v pacman &> /dev/null && pacman -Q libzip &> /dev/null 2>&1; then
        echo "  ✅ libzip installed (MSYS2)"
        libzip_installed=true
    fi
fi

if [ "$libzip_installed" = false ]; then
    echo "  ⚠️  libzip not installed, DOCX support will be disabled"
    echo "     Install: brew install libzip (macOS)"
    echo "              sudo apt install libzip-dev (Linux)"
    echo "              MSYS2: pacman -S mingw-w64-x86_64-libzip (Windows)"
    echo "     Note: libzip is optional, other features work without it"
fi

# Check pugixml (optional, enables DOCX XML parsing)
echo ""
echo "  pugixml (DOCX/XML support, optional):"
pugixml_installed=false
if command -v pkg-config &> /dev/null && pkg-config --exists pugixml 2>/dev/null; then
    echo "  ✅ pugixml installed"
    pugixml_installed=true
elif [ -f "/usr/local/lib/libpugixml.dylib" ] || [ -f "/opt/homebrew/lib/libpugixml.dylib" ]; then
    echo "  ✅ pugixml installed (Homebrew)"
    pugixml_installed=true
elif [ -f "/usr/lib/x86_64-linux-gnu/libpugixml.so" ] || [ -f "/usr/lib/libpugixml.so" ]; then
    echo "  ✅ pugixml installed (system)"
    pugixml_installed=true
elif [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then
    if command -v pacman &> /dev/null && pacman -Q pugixml &> /dev/null 2>&1; then
        echo "  ✅ pugixml installed (MSYS2)"
        pugixml_installed=true
    fi
fi

if [ "$pugixml_installed" = false ]; then
    echo "  ⚠️  pugixml not installed, DOCX support will be disabled"
    echo "     Install: brew install pugixml (macOS)"
    echo "              sudo apt install libpugixml-dev (Linux)"
    echo "              MSYS2: pacman -S mingw-w64-x86_64-pugixml (Windows)"
    echo "     Note: pugixml is optional, other features work without it"
fi

# Check readline/libedit (optional, improves CLI input experience)
echo ""
echo "  Readline/libedit (CLI input, optional):"
readline_found=false
if [ -f "/usr/include/readline/readline.h" ] || [ -f "/usr/local/include/readline/readline.h" ]; then
    echo "  ✅ readline installed"
    readline_found=true
elif [ -f "/usr/include/editline/readline.h" ] || [ -f "/usr/local/include/editline/readline.h" ]; then
    echo "  ✅ libedit installed (readline-compatible)"
    readline_found=true
# macOS: libedit is built into the system
elif [[ "$OSTYPE" == "darwin"* ]]; then
    if [ -f "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/readline/readline.h" ]; then
        echo "  ✅ libedit installed (macOS built-in)"
        readline_found=true
    else
        echo "  ✅ libedit (macOS built-in, no header check needed)"
        readline_found=true
    fi
fi

if [ "$readline_found" = false ]; then
    echo "  ⚠️  readline/libedit not installed, CLI uses basic input mode"
    echo "     Install: sudo apt install libreadline-dev (Linux)"
    echo "     Note: Optional, but recommended for better CLI experience"
fi

# Check ONNX Runtime (optional, enables real embedding for knowledge base)
echo ""
echo "  ONNX Runtime (embedding model, optional):"
onnx_found=false
if command -v pkg-config &> /dev/null && pkg-config --exists libonnxruntime 2>/dev/null; then
    echo "  ✅ ONNX Runtime installed"
    onnx_found=true
elif [ -f "/usr/local/lib/libonnxruntime.so" ] || [ -f "/usr/lib/libonnxruntime.so" ]; then
    echo "  ✅ ONNX Runtime installed (found shared library)"
    onnx_found=true
elif [ -d "/usr/local/include/onnxruntime" ] || [ -d "/usr/include/onnxruntime" ]; then
    echo "  ✅ ONNX Runtime installed (found headers)"
    onnx_found=true
# macOS Homebrew
elif [ -f "/usr/local/lib/libonnxruntime.dylib" ] || [ -f "/opt/homebrew/lib/libonnxruntime.dylib" ]; then
    echo "  ✅ ONNX Runtime installed (Homebrew)"
    onnx_found=true
fi

if [ "$onnx_found" = false ]; then
    echo "  ⚠️  ONNX Runtime not installed, using placeholder embedding"
    echo "     Install: brew install onnxruntime (macOS)"
    echo "              sudo apt install libonnxruntime-dev (Linux)"
    echo "     Note: Optional, knowledge base works without it (returns dummy vectors)"
fi

# ------------------------------------------------------------
# Summary
# ------------------------------------------------------------

echo ""
echo "==================================="

if [ ${#missing_deps[@]} -gt 0 ]; then
    echo "  ❌ Missing dependencies:"
    for dep in "${missing_deps[@]}"; do
        echo "     - $dep"
    done
    echo ""
    echo "  ================================================"
    echo "  Installation Guide (current OS: $OSTYPE)"
    echo "  ================================================"
    echo ""
    case "$OSTYPE" in
        darwin*)
            echo "  [macOS]"
            echo ""
            echo "  1. Install Xcode Command Line Tools (compiler)"
            echo "     Command: xcode-select --install"
            echo ""
            echo "  2. Install Homebrew (package manager)"
            echo "     URL: https://brew.sh"
            echo "     Command: /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
            echo ""
            echo "  3. Install dependencies"
            echo "     Command: brew install cmake qt@6 poppler libzip pugixml"
            echo "     Optional: brew install onnxruntime"
            echo "     Note: Homebrew qt@6 includes Multimedia and WebSockets"
            echo ""
            echo "  [Alternative: Qt official installer]"
            echo "     URL: https://www.qt.io/download"
            echo "     After install, run Maintenance Tool and select Multimedia + WebSockets"
            ;;
        linux*)
            echo "  [Linux]"
            echo ""
            if command -v apt &> /dev/null; then
                echo "  Ubuntu/Debian:"
                echo "     sudo apt update"
                echo "     sudo apt install build-essential cmake qt6-base-dev qt6-base-dev-tools qt6-multimedia-dev qt6-websockets-dev libpoppler-cpp-dev libzip-dev libpugixml-dev"
                echo "     Optional: sudo apt install libonnxruntime-dev libreadline-dev"
            elif command -v dnf &> /dev/null; then
                echo "  Fedora/RHEL:"
                echo "     sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qtwebsockets-devel poppler-cpp-devel libzip-devel pugixml-devel"
                echo "     Optional: sudo dnf install onnxruntime-devel readline-devel"
            elif command -v pacman &> /dev/null; then
                echo "  Arch Linux:"
                echo "     sudo pacman -S base-devel cmake qt6-base qt6-multimedia qt6-websockets poppler libzip pugixml"
                echo "     Optional: sudo pacman -S onnxruntime readline"
            else
                echo "  Install the following using your package manager:"
                echo "     - C++ compiler (GCC 9+ or Clang 10+)"
                echo "     - CMake 3.16+"
                echo "     - Qt 6 (base + multimedia + websockets)"
                echo "     - Poppler (optional, for PDF support)"
                echo "     - libzip + pugixml (optional, for DOCX support)"
            fi
            echo ""
            echo "  [Alternative: Qt official installer]"
            echo "     URL: https://www.qt.io/download"
            echo "     After install, run Maintenance Tool and select Multimedia + WebSockets"
            ;;
        msys*|cygwin*|win32*)
            echo "  [Windows]"
            echo ""
            echo "  Option 1: MinGW (recommended, no Visual Studio required)"
            echo "  ─────────────────────────────────────────────"
            echo ""
            echo "  1. Git for Windows (includes Git Bash)"
            echo "     URL: https://git-scm.com/download/win"
            echo ""
            echo "  2. CMake"
            echo "     URL: https://cmake.org/download/"
            echo "     Select: cmake-x.x.x-windows-x86_64.msi"
            echo "     During install, check 'Add CMake to system PATH'"
            echo ""
            echo "  3. Qt 6"
            echo "     URL: https://www.qt.io/download"
            echo "     Select: Qt 6.x.x for MinGW 11.2 64-bit"
            echo "     ⚠️ Important: In Maintenance Tool, select:"
            echo "        - Qt Multimedia (required for voice)"
            echo "        - Qt WebSockets (required for voice)"
            echo ""
            echo "  4. Poppler / libzip / pugixml (optional, for PDF/DOCX support)"
            echo "     URL: https://www.msys2.org"
            echo "     After installing MSYS2: pacman -S mingw-w64-x86_64-poppler mingw-w64-x86_64-libzip mingw-w64-x86_64-pugixml"
            echo "     Note: Only needed for PDF/DOCX import feature"
            echo ""
            echo "  Option 2: MSVC (requires Visual Studio)"
            echo "  ─────────────────────────────────────────────"
            echo ""
            echo "  1. Visual Studio 2019+"
            echo "     URL: https://visualstudio.microsoft.com/downloads"
            echo "     Select: Visual Studio Community (free)"
            echo "     During install, select 'Desktop development with C++'"
            echo ""
            echo "  2. CMake"
            echo "     URL: https://cmake.org/download/"
            echo ""
            echo "  3. Qt 6"
            echo "     URL: https://www.qt.io/download"
            echo "     Select: Qt 6.x.x for MSVC 2019 64-bit"
            echo "     ⚠️ Important: In Maintenance Tool, select Multimedia + WebSockets"
            echo ""
            echo "  4. Poppler / libzip / pugixml (optional)"
            echo "     Same as above, install via MSYS2"
            ;;
    esac
    echo ""
    echo "  ================================================"
    echo "  After installing dependencies, re-run this script"
    echo "  ================================================"
else
    echo "  ✅ All required dependencies installed"
    if [ "$poppler_installed" = false ]; then
        echo ""
        echo "  ================================================"
        echo "  Poppler Installation Guide (optional, PDF support)"
        echo "  ================================================"
        case "$OSTYPE" in
            darwin*)
                echo "  macOS: brew install poppler"
                ;;
            linux*)
                if command -v apt &> /dev/null; then
                    echo "  Ubuntu/Debian: sudo apt install libpoppler-cpp-dev"
                elif command -v dnf &> /dev/null; then
                    echo "  Fedora/RHEL: sudo dnf install poppler-cpp-devel"
                elif command -v pacman &> /dev/null; then
                    echo "  Arch Linux: sudo pacman -S poppler"
                fi
                ;;
            msys*|cygwin*|win32*)
                echo "  Windows: Install MSYS2 then run pacman -S poppler"
                echo "  URL: https://www.msys2.org"
                ;;
        esac
        echo "  Note: Poppler is optional, other features work without it"
    fi
fi

echo "==================================="
echo ""
echo "Next steps:"
echo ""
echo "  1. Edit .env file to configure iFLYTEK credentials (if voice features needed)"
echo "  2. Run build: ./scripts/build.sh"
echo ""

# Pause before exit (interactive terminal only)
pause_if_interactive
