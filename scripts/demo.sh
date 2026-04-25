#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

CLI_PATH="$PROJECT_ROOT/build/LocalAIAssistant-CLI"
GUI_PATH="$PROJECT_ROOT/build/LocalAIAssistant.app"

print_header() {
    echo ""
    echo "==================================="
    echo "  $1"
    echo "==================================="
    echo ""
}

print_section() {
    echo ""
    echo ">>> $1"
    echo "-----------------------------------"
}

check_build() {
    if [ ! -f "$CLI_PATH" ]; then
        echo "❌ 错误: 未找到编译好的可执行文件"
        echo ""
        echo "请先运行以下命令进行编译:"
        echo "  ./build.sh"
        exit 1
    fi
}

main() {
    print_header "本地AI助手 - 功能演示"
    
    check_build
    
    echo "本脚本将演示 CLI 版本的主要功能"
    echo ""
    
    print_section "1. 查看帮助信息"
    $CLI_PATH --help
    
    print_section "2. 查看当前配置"
    $CLI_PATH config --show-config
    
    print_section "3. 会话管理演示"
    echo "创建新会话..."
    $CLI_PATH sessions -n
    echo ""
    echo "列出所有会话..."
    $CLI_PATH sessions -l
    
    print_section "4. 单次查询演示"
    echo "查询: 什么是人工智能？"
    echo ""
    $CLI_PATH ask "什么是人工智能？"
    
    print_header "演示完成！"
    
    echo "✓ CLI 版本演示已完成"
    echo ""
    
    if [ -d "$GUI_PATH" ]; then
        echo "✓ GUI 版本可用，运行以下命令启动:"
        echo "  open $GUI_PATH"
        echo ""
    fi
    
    echo "📖 更多功能:"
    echo "  $CLI_PATH chat              # 交互式对话模式"
    echo "  $CLI_PATH ask \"你的问题\"   # 单次查询"
    echo "  $CLI_PATH sessions -l       # 列出所有会话"
    echo "  $CLI_PATH config --help     # 查看配置选项"
    echo ""
    
    echo "💡 提示:"
    echo "  - 使用 ./build.sh 构建项目"
    echo "  - 使用 ./install.sh 安装到系统"
    echo "  - 查看 README.md 了解更多功能"
    echo ""
}

main "$@"
