#!/bin/bash

BUILD_DIR="build"
QT_PATH=""
BUILD_TYPE="Release"
CLEAN_BUILD=false
VERBOSE=false
JOBS=""
TARGET=""

usage() {
    cat << EOF
本地AI助手 - 构建脚本

用法: $0 [选项] [目标]

选项:
  -h, --help              显示此帮助信息
  -c, --clean             清理构建目录后重新构建
  -d, --debug             构建调试版本
  -r, --release           构建发布版本（默认）
  -v, --verbose           显示详细输出
  -j, --jobs <n>          指定并行编译任务数
  -q, --qt-path <path>    指定 Qt 安装路径
  --list-targets          列出所有可用目标

目标:
  all                     构建所有目标（默认）
  LocalAIAssistant        仅构建 GUI 版本
  LocalAIAssistant-CLI    仅构建 CLI 版本

示例:
  $0                      # 默认构建（Release 版本）
  $0 -c                   # 清理后重新构建
  $0 -d                   # 构建调试版本
  $0 -j 8                 # 使用 8 个并行任务
  $0 LocalAIAssistant-CLI # 仅构建 CLI 版本
  $0 -c -d -v             # 清理后构建调试版本，显示详细输出

环境变量:
  QT_PATH                 Qt 安装路径（可通过 -q 选项覆盖）
EOF
}

detect_qt_path() {
    # macOS: Qt 官网安装
    if [[ "$OSTYPE" == "darwin"* ]]; then
        if [ -d "$HOME/Qt" ]; then
            QT_VERSION=$(ls -1 "$HOME/Qt" | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | sort -V | tail -1)
            if [ -n "$QT_VERSION" ]; then
                QT_PATH="$HOME/Qt/$QT_VERSION/macos"
                echo "✓ 检测到 Qt 安装: $QT_PATH"
                return 0
            fi
        fi

        # macOS: Homebrew
        if command -v brew &> /dev/null; then
            BREW_QT=$(brew --prefix qt@6 2>/dev/null)
            if [ -d "$BREW_QT" ]; then
                QT_PATH="$BREW_QT"
                echo "✓ 检测到 Homebrew Qt 安装: $QT_PATH"
                return 0
            fi
        fi
    fi

    # Linux: Qt 官网安装
    if [[ "$OSTYPE" == "linux"* ]]; then
        if [ -d "$HOME/Qt" ]; then
            QT_VERSION=$(ls -1 "$HOME/Qt" | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | sort -V | tail -1)
            if [ -n "$QT_VERSION" ]; then
                # 检测 Linux 发行版类型
                if [ -f /etc/debian_version ]; then
                    QT_PATH="$HOME/Qt/$QT_VERSION/gcc_64"
                elif [ -f /etc/redhat-release ]; then
                    QT_PATH="$HOME/Qt/$QT_VERSION/gcc_64"
                else
                    QT_PATH="$HOME/Qt/$QT_VERSION/gcc_64"
                fi
                if [ -d "$QT_PATH" ]; then
                    echo "✓ 检测到 Qt 安装: $QT_PATH"
                    return 0
                fi
            fi
        fi

        # Linux: 包管理器安装
        if command -v qmake6 &> /dev/null; then
            QT_PATH=$(qmake6 -query QT_INSTALL_PREFIX 2>/dev/null)
            if [ -d "$QT_PATH" ]; then
                echo "✓ 检测到系统 Qt 安装: $QT_PATH"
                return 0
            fi
        fi

        if command -v qmake &> /dev/null; then
            QT_VERSION_QUERY=$(qmake -query QT_VERSION 2>/dev/null)
            if [[ "$QT_VERSION_QUERY" == 6* ]]; then
                QT_PATH=$(qmake -query QT_INSTALL_PREFIX 2>/dev/null)
                if [ -d "$QT_PATH" ]; then
                    echo "✓ 检测到系统 Qt 安装: $QT_PATH"
                    return 0
                fi
            fi
        fi

        # Linux: 常见安装路径
        for path in "/usr/lib/qt6" "/usr" "/opt/qt6"; do
            if [ -f "$path/lib/libQt6Widgets.so" ] || [ -f "$path/lib64/libQt6Widgets.so" ]; then
                QT_PATH="$path"
                echo "✓ 检测到 Qt 安装: $QT_PATH"
                return 0
            fi
        done
    fi

    return 1
}

check_dependencies() {
    local missing_deps=()
    
    echo "检查依赖..."
    
    if ! command -v cmake &> /dev/null; then
        missing_deps+=("cmake")
    else
        echo "  ✓ CMake: $(cmake --version | head -1)"
    fi
    
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        missing_deps+=("C++ 编译器")
    else
        local compiler=$(command -v g++ &> /dev/null && echo "g++" || echo "clang++")
        echo "  ✓ 编译器: $compiler"
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        echo ""
        echo "❌ 缺少以下依赖:"
        for dep in "${missing_deps[@]}"; do
            echo "  - $dep"
        done
        echo ""
        echo "请安装缺少的依赖后重试。"
        return 1
    fi
    
    return 0
}

list_targets() {
    echo "可用目标:"
    echo "  - all                   构建所有目标"
    echo "  - LocalAIAssistant      GUI 版本"
    echo "  - LocalAIAssistant-CLI  CLI 版本"
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                usage
                exit 0
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
            --list-targets)
                list_targets
                exit 0
                ;;
            LocalAIAssistant|LocalAIAssistant-CLI|all)
                TARGET="$1"
                shift
                ;;
            *)
                echo "❌ 未知选项: $1"
                echo "使用 -h 或 --help 查看帮助信息"
                exit 1
                ;;
        esac
    done
    
    if [ -z "$TARGET" ]; then
        TARGET="all"
    fi
}

print_banner() {
    echo ""
    echo "==================================="
    echo "  本地AI助手 - 构建脚本"
    echo "==================================="
    echo ""
}

main() {
    parse_args "$@"
    print_banner
    
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
    
    if [ ! -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
        echo "❌ 错误: 未找到项目根目录"
        exit 1
    fi
    
    cd "$PROJECT_ROOT"
    
    if ! check_dependencies; then
        exit 1
    fi
    
    if [ -z "$QT_PATH" ]; then
        if ! detect_qt_path; then
            echo ""
            echo "❌ 错误: 未找到 Qt6 安装"
            echo ""
            echo "请通过以下方式之一指定 Qt 路径:"
            echo "  1. 使用 -q 选项: $0 -q /path/to/qt"
            echo "  2. 设置环境变量: QT_PATH=/path/to/qt $0"
            echo ""
            echo "安装 Qt6:"
            if [[ "$OSTYPE" == "darwin"* ]]; then
                echo "  macOS:   从 https://www.qt.io/download 下载安装"
                echo "           或使用 Homebrew: brew install qt@6"
            elif [[ "$OSTYPE" == "linux"* ]]; then
                echo "  Ubuntu/Debian: sudo apt install qt6-base-dev qt6-base-dev-tools"
                echo "  Fedora/RHEL:   sudo dnf install qt6-qtbase-devel"
                echo "  Arch Linux:    sudo pacman -S qt6-base"
            fi
            exit 1
        fi
    else
        echo "✓ 使用指定的 Qt 路径: $QT_PATH"
    fi
    
    if [ "$CLEAN_BUILD" = true ]; then
        echo ""
        echo "清理旧的构建文件..."
        rm -rf "$BUILD_DIR"
    fi
    
    mkdir -p "$BUILD_DIR"
    
    echo ""
    echo "配置项目..."
    echo "  构建类型: $BUILD_TYPE"
    echo "  目标: $TARGET"
    
    local cmake_args=(
        "-DCMAKE_PREFIX_PATH=$QT_PATH"
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    )
    
    if [ "$VERBOSE" = true ]; then
        cmake_args+=("-DCMAKE_VERBOSE_MAKEFILE=ON")
    fi
    
    cd "$BUILD_DIR"
    
    if ! cmake .. "${cmake_args[@]}"; then
        echo ""
        echo "❌ 错误: CMake 配置失败"
        exit 1
    fi
    
    echo ""
    echo "编译项目..."
    
    local build_args=("--build" ".")
    
    if [ -n "$JOBS" ]; then
        build_args+=("-j$JOBS")
    else
        if [[ "$OSTYPE" == "darwin"* ]]; then
            build_args+=("-j$(sysctl -n hw.ncpu)")
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
        echo ""
        echo "❌ 错误: 编译失败"
        exit 1
    fi

    # Wait briefly for bundle to be fully created
    sleep 1

    # Copy icon to macOS bundle Resources (if on macOS)
    if [[ "$OSTYPE" == "darwin"* ]]; then
        if [ -f "$PROJECT_ROOT/resources/icons/app.icns" ]; then
            if [ -d "$BUILD_DIR/LocalAIAssistant.app/Contents" ]; then
                mkdir -p "$BUILD_DIR/LocalAIAssistant.app/Contents/Resources"
                cp "$PROJECT_ROOT/resources/icons/app.icns" "$BUILD_DIR/LocalAIAssistant.app/Contents/Resources/app.icns"
                echo "  ✓ 已复制应用图标"
            fi
        fi
    fi

    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    echo "==================================="
    echo "  ✓ 构建成功! (耗时: ${duration}s)"
    echo "==================================="
    echo ""
    echo "可执行文件位置:"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "  GUI版本: $BUILD_DIR/LocalAIAssistant.app/Contents/MacOS/LocalAIAssistant"
        echo "  CLI版本: $BUILD_DIR/LocalAIAssistant-CLI"
    else
        echo "  GUI版本: $BUILD_DIR/LocalAIAssistant"
        echo "  CLI版本: $BUILD_DIR/LocalAIAssistant-CLI"
    fi
    echo ""
    echo "运行命令:"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "  GUI版本: open $BUILD_DIR/LocalAIAssistant.app"
    else
        echo "  GUI版本: $BUILD_DIR/LocalAIAssistant"
    fi
    echo "  CLI版本: $BUILD_DIR/LocalAIAssistant-CLI --help"
    echo ""
    
    if [ "$BUILD_TYPE" = "Debug" ]; then
        echo "提示: 这是调试版本，包含调试符号但运行较慢"
        echo "      如需发布版本，请使用 -r 或 --release 选项"
        echo ""
    fi
}

main "$@"
