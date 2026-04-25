#!/usr/bin/env pwsh
# 本地AI助手 - Windows PowerShell 演示脚本

param(
    [switch]$Help
)

$SCRIPT_DIR = $PSScriptRoot
$PROJECT_ROOT = Split-Path $SCRIPT_DIR -Parent
$CLI_PATH = Join-Path $PROJECT_ROOT "build\LocalAIAssistant-CLI.exe"
$GUI_PATH = Join-Path $PROJECT_ROOT "build\LocalAIAssistant.exe"

function Show-Help {
    Write-Host ""
    Write-Host "本地AI助手 - 演示脚本"
    Write-Host ""
    Write-Host "用法: $PSCommandPath [选项]"
    Write-Host ""
    Write-Host "选项:"
    Write-Host "  -Help    显示此帮助信息"
    Write-Host ""
    Write-Host "此脚本将演示 CLI 版本的主要功能。"
    Write-Host ""
}

function Print-Header {
    param([string]$Message)
    Write-Host ""
    Write-Host "===================================" -ForegroundColor Cyan
    Write-Host "  $Message" -ForegroundColor Cyan
    Write-Host "===================================" -ForegroundColor Cyan
    Write-Host ""
}

function Print-Section {
    param([string]$Message)
    Write-Host ""
    Write-Host ">>> $Message" -ForegroundColor Yellow
    Write-Host "-----------------------------------"
}

function Check-Build {
    if (-not (Test-Path $CLI_PATH)) {
        Write-Host "错误: 未找到编译好的可执行文件" -ForegroundColor Red
        Write-Host ""
        Write-Host "请先运行以下命令进行编译:"
        Write-Host "  .\build.ps1"
        exit 1
    }
}

# Main
if ($Help) {
    Show-Help
    exit 0
}

Print-Header "本地AI助手 - 功能演示"

Check-Build

Write-Host "本脚本将演示 CLI 版本的主要功能"
Write-Host ""

Print-Section "1. 查看帮助信息"
& $CLI_PATH --help

Print-Section "2. 查看当前配置"
& $CLI_PATH config --show-config

Print-Section "3. 会话管理演示"
Write-Host "创建新会话..."
& $CLI_PATH sessions -n
Write-Host ""
Write-Host "列出所有会话..."
& $CLI_PATH sessions -l

Print-Section "4. 单次查询演示"
Write-Host "查询: 什么是人工智能？"
Write-Host ""
& $CLI_PATH ask "什么是人工智能？"

Print-Header "演示完成！"

Write-Host "✓ CLI 版本演示已完成" -ForegroundColor Green
Write-Host ""

if (Test-Path $GUI_PATH) {
    Write-Host "✓ GUI 版本可用，运行以下命令启动:" -ForegroundColor Green
    Write-Host "  $GUI_PATH"
    Write-Host ""
}

Write-Host "📖 更多功能:"
Write-Host "  $CLI_PATH chat              # 交互式对话模式"
Write-Host "  $CLI_PATH ask `"你的问题`"   # 单次查询"
Write-Host "  $CLI_PATH sessions -l       # 列出所有会话"
Write-Host "  $CLI_PATH config --help     # 查看配置选项"
Write-Host ""

Write-Host "💡 提示:"
Write-Host "  - 使用 .\build.ps1 构建项目"
Write-Host "  - 使用 .\install.ps1 安装到系统"
Write-Host "  - 查看 README.md 了解更多功能"
Write-Host ""