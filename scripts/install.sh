#!/bin/bash

INSTALL_PREFIX="/usr/local"
EXECUTABLE_NAME="ai"
INSTALL_GUI=false
INSTALL_CLI=true
UNINSTALL=false
FORCE=false

usage() {
    cat << EOF
本地AI助手 - 安装/卸载脚本

用法: $0 [选项]

选项:
  -h, --help              显示此帮助信息
  --prefix <path>         安装路径前缀 (默认: /usr/local)
  --cli                   安装 CLI 版本 (默认)
  --gui                   安装 GUI 版本到 /Applications
  --all                   安装 CLI 和 GUI 版本
  --uninstall             卸载已安装的程序
  -f, --force             强制安装/卸载，不询问确认

示例:
  $0                      # 安装 CLI 版本到 /usr/local/bin/ai
  $0 --cli                # 同上
  $0 --gui                # 安装 GUI 版本到 /Applications
  $0 --all                # 安装 CLI 和 GUI 版本
  $0 --prefix ~/.local    # 安装到 ~/.local/bin/ai
  $0 --uninstall          # 卸载 CLI 版本
  $0 --uninstall --all    # 卸载所有版本

注意:
  - 安装到系统目录需要管理员权限 (sudo)
  - 安装到用户目录不需要管理员权限
EOF
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                usage
                exit 0
                ;;
            --prefix)
                INSTALL_PREFIX="$2"
                shift 2
                ;;
            --cli)
                INSTALL_CLI=true
                shift
                ;;
            --gui)
                INSTALL_GUI=true
                shift
                ;;
            --all)
                INSTALL_CLI=true
                INSTALL_GUI=true
                shift
                ;;
            --uninstall)
                UNINSTALL=true
                shift
                ;;
            -f|--force)
                FORCE=true
                shift
                ;;
            *)
                echo "❌ 未知选项: $1"
                echo "使用 -h 或 --help 查看帮助信息"
                exit 1
                ;;
        esac
    done
}

check_executables() {
    local missing=()
    
    if [ "$INSTALL_CLI" = true ]; then
        if [ ! -f "build/LocalAIAssistant-CLI" ]; then
            missing+=("CLI版本: build/LocalAIAssistant-CLI")
        fi
    fi
    
    if [ "$INSTALL_GUI" = true ]; then
        if [ ! -d "build/LocalAIAssistant.app" ]; then
            missing+=("GUI版本: build/LocalAIAssistant.app")
        fi
    fi
    
    if [ ${#missing[@]} -ne 0 ]; then
        echo "❌ 错误: 未找到以下可执行文件:"
        for file in "${missing[@]}"; do
            echo "  - $file"
        done
        echo ""
        echo "请先运行 ./build.sh 进行编译"
        return 1
    fi
    
    return 0
}

check_permissions() {
    if [ "$INSTALL_CLI" = true ]; then
        if [[ "$INSTALL_PREFIX" == /usr/* ]] || [[ "$INSTALL_PREFIX" == /opt/* ]]; then
            if [ "$EUID" -ne 0 ]; then
                echo "❌ 错误: 安装到系统目录需要管理员权限"
                echo ""
                echo "请使用以下方式之一:"
                echo "  1. 使用 sudo: sudo $0 $@"
                echo "  2. 安装到用户目录: $0 --prefix ~/.local"
                return 1
            fi
        fi
    fi
    
    if [ "$INSTALL_GUI" = true ]; then
        if [ "$EUID" -ne 0 ]; then
            echo "❌ 错误: 安装 GUI 版本到 /Applications 需要管理员权限"
            echo ""
            echo "请使用: sudo $0 --gui"
            return 1
        fi
    fi
    
    return 0
}

confirm_action() {
    local action="$1"
    
    if [ "$FORCE" = true ]; then
        return 0
    fi
    
    echo ""
    echo "$action"
    read -p "确认继续? [y/N] " -n 1 -r
    echo ""
    
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "操作已取消"
        return 1
    fi
    
    return 0
}

install_cli() {
    local bin_dir="$INSTALL_PREFIX/bin"
    local target="$bin_dir/$EXECUTABLE_NAME"
    
    echo ""
    echo "安装 CLI 版本..."
    echo "  目标路径: $target"
    
    mkdir -p "$bin_dir"
    
    if [ -f "$target" ]; then
        echo "  发现已存在的安装，正在替换..."
        rm -f "$target"
    fi
    
    cp build/LocalAIAssistant-CLI "$target"
    chmod +x "$target"
    
    echo "  ✓ CLI 版本安装成功"
    echo ""
    echo "现在可以使用以下命令:"
    echo "  $EXECUTABLE_NAME --help"
    echo "  $EXECUTABLE_NAME chat"
    echo "  $EXECUTABLE_NAME ask \"你的问题\""
    
    if [[ ":$PATH:" != *":$bin_dir:"* ]]; then
        echo ""
        echo "⚠️  注意: $bin_dir 不在 PATH 环境变量中"
        echo "请将以下内容添加到你的 shell 配置文件 (~/.bashrc, ~/.zshrc 等):"
        echo "  export PATH=\"$bin_dir:\$PATH\""
    fi
}

install_gui() {
    local app_name="LocalAIAssistant.app"
    local target="/Applications/$app_name"
    
    echo ""
    echo "安装 GUI 版本..."
    echo "  目标路径: $target"
    
    if [ -d "$target" ]; then
        echo "  发现已存在的安装，正在替换..."
        rm -rf "$target"
    fi
    
    cp -R "build/$app_name" "/Applications/"
    
    echo "  ✓ GUI 版本安装成功"
    echo ""
    echo "现在可以通过以下方式运行:"
    echo "  - 在应用程序文件夹中找到 LocalAIAssistant"
    echo "  - 或使用命令: open /Applications/LocalAIAssistant.app"
}

uninstall_cli() {
    local target="$INSTALL_PREFIX/bin/$EXECUTABLE_NAME"
    
    echo ""
    echo "卸载 CLI 版本..."
    
    if [ ! -f "$target" ]; then
        echo "  ⚠️  CLI 版本未安装"
        return 0
    fi
    
    rm -f "$target"
    echo "  ✓ CLI 版本已卸载"
}

uninstall_gui() {
    local target="/Applications/LocalAIAssistant.app"
    
    echo ""
    echo "卸载 GUI 版本..."
    
    if [ ! -d "$target" ]; then
        echo "  ⚠️  GUI 版本未安装"
        return 0
    fi
    
    rm -rf "$target"
    echo "  ✓ GUI 版本已卸载"
}

print_banner() {
    echo ""
    echo "==================================="
    echo "  本地AI助手 - 安装脚本"
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
    
    if [ "$UNINSTALL" = true ]; then
        local action="将卸载以下组件:"
        [ "$INSTALL_CLI" = true ] && action+="\n  - CLI 版本 ($INSTALL_PREFIX/bin/$EXECUTABLE_NAME)"
        [ "$INSTALL_GUI" = true ] && action+="\n  - GUI 版本 (/Applications/LocalAIAssistant.app)"
        
        if ! confirm_action "$action"; then
            exit 0
        fi
        
        if ! check_permissions; then
            exit 1
        fi
        
        [ "$INSTALL_CLI" = true ] && uninstall_cli
        [ "$INSTALL_GUI" = true ] && uninstall_gui
        
        echo ""
        echo "==================================="
        echo "  ✓ 卸载完成"
        echo "==================================="
    else
        if ! check_executables; then
            exit 1
        fi
        
        local action="将安装以下组件:"
        [ "$INSTALL_CLI" = true ] && action+="\n  - CLI 版本 ($INSTALL_PREFIX/bin/$EXECUTABLE_NAME)"
        [ "$INSTALL_GUI" = true ] && action+="\n  - GUI 版本 (/Applications/LocalAIAssistant.app)"
        
        if ! confirm_action "$action"; then
            exit 0
        fi
        
        if ! check_permissions; then
            exit 1
        fi
        
        [ "$INSTALL_CLI" = true ] && install_cli
        [ "$INSTALL_GUI" = true ] && install_gui
        
        echo ""
        echo "==================================="
        echo "  ✓ 安装完成"
        echo "==================================="
    fi
}

main "$@"
