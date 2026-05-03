#!/bin/bash

# ============================================================
# LocalAIAssistant - First-time Setup Script
# ============================================================
# Run this after cloning the repository for the first time
# Usage: ./scripts/setup.sh
# ============================================================

set -e

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

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
if [ -d "$HOME/Qt" ]; then
    qt_versions=$(ls -1 "$HOME/Qt" 2>/dev/null | grep -E '^[0-9]+\.[0-9]+' | head -3)
    if [ -n "$qt_versions" ]; then
        echo "  ✅ Qt 已安装在 ~/Qt/"
        echo "     版本: $qt_versions"
        echo "  ⚠️  请确保已勾选 Multimedia 和 WebSockets 模块（语音功能必需）"
    else
        echo "  ⚠️  ~/Qt 目录存在但未找到 Qt 版本"
    fi
elif command -v qmake6 &> /dev/null || command -v qmake &> /dev/null; then
    echo "  ✅ Qt 已安装 (系统包管理器)"
    # Check Multimedia and WebSockets on Linux
    if [[ "$OSTYPE" == "linux"* ]]; then
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
        fi
    fi
else
    echo "  ❌ Qt 6 未安装"
    missing_deps+=("Qt 6 (+ Multimedia + WebSockets)")
fi

# Check Poppler (optional)
echo ""
echo "  Poppler (PDF支持，可选):"
if command -v pkg-config &> /dev/null && pkg-config --exists poppler-cpp 2>/dev/null; then
    echo "  ✅ Poppler 已安装"
else
    echo "  ⚠️  Poppler 未安装，PDF功能将被禁用"
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
    echo "  安装指南:"
    echo ""
    case "$OSTYPE" in
        darwin*)
            echo "     macOS:"
            echo "       xcode-select --install"
            echo "       brew install cmake qt@6 poppler"
            echo "       # Homebrew 的 qt@6 已包含 Multimedia 和 WebSockets"
            ;;
        linux*)
            echo "     Linux (Ubuntu/Debian):"
            echo "       sudo apt install build-essential cmake qt6-base-dev qt6-multimedia-dev qt6-websockets-dev libpoppler-cpp-dev"
            ;;
        msys*|cygwin*|win32*)
            echo "     Windows:"
            echo "       1. 安装 Git for Windows: https://git-scm.com/download/win"
            echo "       2. 安装 CMake: https://cmake.org/download/"
            echo "       3. 安装 Qt 6: https://www.qt.io/download"
            echo "       4. ⚠️ 在 Qt Maintenance Tool 中勾选 Multimedia 和 WebSockets 模块"
            ;;
    esac
    echo ""
    echo "  安装依赖后重新运行此脚本"
else
    echo "  ✅ 所有必需依赖已安装"
fi

echo "==================================="
echo ""
echo "下一步:"
echo ""
echo "  1. 编辑 .env 文件配置讯飞凭证（如需语音功能）"
echo "  2. 运行构建: ./scripts/build.sh"
echo ""