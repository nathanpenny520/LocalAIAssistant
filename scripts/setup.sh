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
        read -p "按 Enter 键退出..." -r
    fi
}

# Trap errors to pause before exit
trap 'echo ""; echo "❌ 脚本执行出错"; pause_if_interactive; exit 1' ERR

echo ""
echo "==================================="
echo "  LocalAIAssistant - 初始化设置"
echo "==================================="
echo ""

# ------------------------------------------------------------
# Step 1: Create .env file from template
# ------------------------------------------------------------

echo "[Step 1] 配置讯飞语音凭证"
echo ""

if [ -f ".env" ]; then
    echo "  ✅ .env 文件已存在"
else
    if [ -f ".env.example" ]; then
        echo "  正在复制 .env.example -> .env"
        cp .env.example .env
        echo ""
        echo "  ⚠️  请编辑 .env 文件，填入你的讯飞语音凭证："
        echo ""
        echo "      XFYUN_APP_ID=你的APPID"
        echo "      XFYUN_API_KEY=你的APIKey"
        echo "      XFYUN_API_SECRET=你的APISecret"
        echo ""
        echo "  获取凭证: https://www.xfyun.cn"
        echo "  开通服务: 语音听写(流式版) + 超拟人语音合成"
        echo ""
    else
        echo "  ❌ .env.example 文件不存在，请检查项目完整性"
    fi
fi

# ------------------------------------------------------------
# Step 2: Check build dependencies
# ------------------------------------------------------------

echo "[Step 2] 检查构建依赖"
echo ""

missing_deps=()

# Check CMake
if command -v cmake &> /dev/null; then
    cmake_version=$(cmake --version | head -1)
    echo "  ✅ CMake: $cmake_version"
else
    echo "  ❌ CMake 未安装"
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
    echo "  ✅ 编译器: $gpp_version"
    found_compiler=true
elif command -v clang++ &> /dev/null; then
    clang_version=$(clang++ --version | head -1)
    echo "  ✅ 编译器: $clang_version"
    found_compiler=true
elif command -v cl &> /dev/null; then
    echo "  ✅ 编译器: MSVC cl"
    found_compiler=true
fi

# Windows: Try to find MinGW in Qt Tools directory (not in PATH by default)
if [ "$found_compiler" = false ] && [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then
    mingw_versions=("1310" "1120" "1110" "100" "90" "81" "73")
    for drive in "c" "d" "e"; do
        for ver in "${mingw_versions[@]}"; do
            mingw_path="/$drive/Qt/Tools/mingw${ver}_64/bin/g++.exe"
            if [ -f "$mingw_path" ]; then
                echo "  ✅ 编译器: MinGW g++ (检测到但未加入 PATH)"
                echo "     位置: $mingw_path"
                echo "     提示: build.sh 会自动添加到 PATH"
                found_compiler=true
                break 2
            fi
        done
    done
fi

if [ "$found_compiler" = false ]; then
    echo "  ❌ C++ 编译器未安装"
    echo "     注: Qt Creator 是 IDE，不是编译器"
    case "$OSTYPE" in
        msys*|cygwin*|win32*)
            echo "     Windows: 安装 MinGW 版 Qt 会自带 g++，或安装 Visual Studio"
            ;;
        darwin*)
            echo "     macOS: 运行 xcode-select --install"
            ;;
        linux*)
            echo "     Linux: 运行 sudo apt install build-essential"
            ;;
    esac
    missing_deps+=("C++ compiler")
fi

# Check Qt (basic check)
echo ""
echo "  Qt 6 检查:"

# macOS/Linux: Check ~/Qt directory (official Qt installation)
if [ -d "$HOME/Qt" ]; then
    qt_versions=$(ls -1 "$HOME/Qt" 2>/dev/null | grep -E '^[0-9]+\.[0-9]+' | head -3)
    if [ -n "$qt_versions" ]; then
        echo "  ✅ Qt 已安装在 ~/Qt/"
        echo "     版本: $qt_versions"
        echo "  ⚠️  请确保已勾选 Multimedia 和 WebSockets 模块（语音功能必需）"
    else
        echo "  ⚠️  ~/Qt 目录存在但未找到 Qt 版本"
    fi
# macOS/Linux: Check system package manager
elif command -v qmake6 &> /dev/null || command -v qmake &> /dev/null; then
    echo "  ✅ Qt 已安装 (系统包管理器)"
    # Check Multimedia and WebSockets on Linux
    if [[ "$OSTYPE" == "linux"* ]]; then
        # Debian/Ubuntu (dpkg)
        if command -v dpkg &> /dev/null; then
            if dpkg -l qt6-multimedia-dev &> /dev/null 2>&1; then
                echo "  ✅ Multimedia 模块已安装"
            else
                echo "  ⚠️  请安装 qt6-multimedia-dev（语音功能必需）"
            fi
            if dpkg -l qt6-websockets-dev &> /dev/null 2>&1; then
                echo "  ✅ WebSockets 模块已安装"
            else
                echo "  ⚠️  请安装 qt6-websockets-dev（语音功能必需）"
            fi
        # Fedora/RHEL (rpm)
        elif command -v rpm &> /dev/null; then
            if rpm -q qt6-qtmultimedia-devel &> /dev/null 2>&1; then
                echo "  ✅ Multimedia 模块已安装"
            else
                echo "  ⚠️  请安装 qt6-qtmultimedia-devel（语音功能必需）"
            fi
            if rpm -q qt6-qtwebsockets-devel &> /dev/null 2>&1; then
                echo "  ✅ WebSockets 模块已安装"
            else
                echo "  ⚠️  请安装 qt6-qtwebsockets-devel（语音功能必需）"
            fi
        # Arch Linux (pacman)
        elif command -v pacman &> /dev/null; then
            if pacman -Q qt6-multimedia &> /dev/null 2>&1; then
                echo "  ✅ Multimedia 模块已安装"
            else
                echo "  ⚠️  请安装 qt6-multimedia（语音功能必需）"
            fi
            if pacman -Q qt6-websockets &> /dev/null 2>&1; then
                echo "  ✅ WebSockets 模块已安装"
            else
                echo "  ⚠️  请安装 qt6-websockets（语音功能必需）"
            fi
        else
            echo "  ⚠️  无法检测 Qt 模块，请手动确认 Multimedia 和 WebSockets 已安装"
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
                                echo "  ✅ Qt 已安装在 /$drive/Qt/"
                                echo "     版本: $version_name"
                                echo "     编译器: $compiler"
                                echo "     位置: $qt_path"
                                echo "  ⚠️  请确保在 Qt Maintenance Tool 中已勾选 Multimedia 和 WebSockets"
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
        echo "  ❌ Qt 6 未安装"
        missing_deps+=("Qt 6 (+ Multimedia + WebSockets)")
    fi
else
    echo "  ❌ Qt 6 未安装"
    missing_deps+=("Qt 6 (+ Multimedia + WebSockets)")
fi

# Check Poppler (optional)
echo ""
echo "  Poppler (PDF支持，可选):"
poppler_installed=false
if command -v pkg-config &> /dev/null && pkg-config --exists poppler-cpp 2>/dev/null; then
    echo "  ✅ Poppler 已安装"
    poppler_installed=true
elif [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then
    # Windows: Check MSYS2 poppler
    if command -v pacman &> /dev/null && pacman -Q poppler &> /dev/null 2>&1; then
        echo "  ✅ Poppler 已安装 (MSYS2)"
        poppler_installed=true
    fi
fi

if [ "$poppler_installed" = false ]; then
    echo "  ⚠️  Poppler 未安装，PDF功能将被禁用"
    echo "     说明: Poppler 是可选依赖，不安装不影响其他功能"
fi

# ------------------------------------------------------------
# Summary
# ------------------------------------------------------------

echo ""
echo "==================================="

if [ ${#missing_deps[@]} -gt 0 ]; then
    echo "  ❌ 缺少依赖:"
    for dep in "${missing_deps[@]}"; do
        echo "     - $dep"
    done
    echo ""
    echo "  ================================================"
    echo "  安装指南 (当前系统: $OSTYPE)"
    echo "  ================================================"
    echo ""
    case "$OSTYPE" in
        darwin*)
            echo "  【macOS】"
            echo ""
            echo "  1. 安装 Xcode 命令行工具（编译器）"
            echo "     命令: xcode-select --install"
            echo ""
            echo "  2. 安装 Homebrew（包管理器）"
            echo "     网址: https://brew.sh"
            echo "     命令: /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
            echo ""
            echo "  3. 安装依赖"
            echo "     命令: brew install cmake qt@6 poppler"
            echo "     说明: Homebrew 的 qt@6 已包含 Multimedia 和 WebSockets"
            echo ""
            echo "  【备选方案: 官网安装 Qt】"
            echo "     网址: https://www.qt.io/download"
            echo "     安装后运行 Maintenance Tool 勾选 Multimedia 和 WebSockets"
            ;;
        linux*)
            echo "  【Linux】"
            echo ""
            if command -v apt &> /dev/null; then
                echo "  Ubuntu/Debian:"
                echo "     sudo apt update"
                echo "     sudo apt install build-essential cmake qt6-base-dev qt6-base-dev-tools qt6-multimedia-dev qt6-websockets-dev libpoppler-cpp-dev"
            elif command -v dnf &> /dev/null; then
                echo "  Fedora/RHEL:"
                echo "     sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qtwebsockets-devel poppler-cpp-devel"
            elif command -v pacman &> /dev/null; then
                echo "  Arch Linux:"
                echo "     sudo pacman -S base-devel cmake qt6-base qt6-multimedia qt6-websockets poppler"
            else
                echo "  请使用您的包管理器安装以下依赖:"
                echo "     - C++ 编译器 (GCC 9+ 或 Clang 10+)"
                echo "     - CMake 3.16+"
                echo "     - Qt 6 (base + multimedia + websockets)"
                echo "     - Poppler (可选，PDF 支持)"
            fi
            echo ""
            echo "  【备选方案: 官网安装 Qt】"
            echo "     网址: https://www.qt.io/download"
            echo "     安装后运行 Maintenance Tool 勾选 Multimedia 和 WebSockets"
            ;;
        msys*|cygwin*|win32*)
            echo "  【Windows】"
            echo ""
            echo "  方式一: MinGW（推荐，无需 Visual Studio）"
            echo "  ─────────────────────────────────────────────"
            echo ""
            echo "  1. Git for Windows（包含 Git Bash）"
            echo "     网址: https://git-scm.com/download/win"
            echo ""
            echo "  2. CMake"
            echo "     网址: https://cmake.org/download/"
            echo "     选择: cmake-x.x.x-windows-x86_64.msi"
            echo "     安装时勾选 'Add CMake to system PATH'"
            echo ""
            echo "  3. Qt 6"
            echo "     网址: https://www.qt.io/download"
            echo "     选择: Qt 6.x.x for MinGW 11.2 64-bit"
            echo "     ⚠️ 重要: 在 Maintenance Tool 中勾选:"
            echo "        - Qt Multimedia（语音功能必需）"
            echo "        - Qt WebSockets（语音功能必需）"
            echo ""
            echo "  4. Poppler（可选，PDF 支持）"
            echo "     网址: https://www.msys2.org"
            echo "     安装 MSYS2 后运行: pacman -S poppler"
            echo "     说明: 不安装不影响其他功能，仅 PDF 上传不可用"
            echo ""
            echo "  方式二: MSVC（需 Visual Studio）"
            echo "  ─────────────────────────────────────────────"
            echo ""
            echo "  1. Visual Studio 2019+"
            echo "     网址: https://visualstudio.microsoft.com/downloads"
            echo "     选择: Visual Studio Community（免费）"
            echo "     安装时勾选 'Desktop development with C++'"
            echo ""
            echo "  2. CMake"
            echo "     网址: https://cmake.org/download/"
            echo ""
            echo "  3. Qt 6"
            echo "     网址: https://www.qt.io/download"
            echo "     选择: Qt 6.x.x for MSVC 2019 64-bit"
            echo "     ⚠️ 重要: 在 Maintenance Tool 中勾选 Multimedia 和 WebSockets"
            echo ""
            echo "  4. Poppler（可选）"
            echo "     同上，通过 MSYS2 安装"
            ;;
    esac
    echo ""
    echo "  ================================================"
    echo "  安装依赖后重新运行此脚本验证"
    echo "  ================================================"
else
    echo "  ✅ 所有必需依赖已安装"
    if [ "$poppler_installed" = false ]; then
        echo ""
        echo "  ================================================"
        echo "  Poppler 安装指南（可选，PDF 支持）"
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
                echo "  Windows: 安装 MSYS2 后运行 pacman -S poppler"
                echo "  网址: https://www.msys2.org"
                ;;
        esac
        echo "  说明: Poppler 是可选依赖，不安装不影响其他功能"
    fi
fi

echo "==================================="
echo ""
echo "下一步:"
echo ""
echo "  1. 编辑 .env 文件配置讯飞凭证（如需语音功能）"
echo "  2. 运行构建: ./scripts/build.sh"
echo ""

# Pause before exit (interactive terminal only)
pause_if_interactive