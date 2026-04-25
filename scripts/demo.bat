@echo off
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..
set CLI_PATH=%PROJECT_ROOT%\build\LocalAIAssistant-CLI.exe
set GUI_PATH=%PROJECT_ROOT%\build\LocalAIAssistant.exe

call :print_header "本地AI助手 - 功能演示"

call :check_build

echo 本脚本将演示 CLI 版本的主要功能
echo.

call :print_section "1. 查看帮助信息"
"%CLI_PATH%" --help

call :print_section "2. 查看当前配置"
"%CLI_PATH%" config --show-config

call :print_section "3. 会话管理演示"
echo 创建新会话...
"%CLI_PATH%" sessions -n
echo.
echo 列出所有会话...
"%CLI_PATH%" sessions -l

call :print_section "4. 单次查询演示"
echo 查询: 什么是人工智能？
echo.
"%CLI_PATH%" ask "什么是人工智能？"

call :print_header "演示完成！"

echo √ CLI 版本演示已完成
echo.

if exist "%GUI_PATH%" (
    echo √ GUI 版本可用，运行以下命令启动:
    echo   "%GUI_PATH%"
    echo.
)

echo 更多功能:
echo   "%CLI_PATH%" chat              # 交互式对话模式
echo   "%CLI_PATH%" ask "你的问题"    # 单次查询
echo   "%CLI_PATH%" sessions -l       # 列出所有会话
echo   "%CLI_PATH%" config --help     # 查看配置选项
echo.

echo 提示:
echo   - 使用 build.bat 构建项目
echo   - 使用 install.bat 安装到系统
echo   - 查看 README.md 了解更多功能
echo.

exit /b 0

:: ============================================
:: Functions
:: ============================================

:check_build
if not exist "%CLI_PATH%" (
    echo 错误: 未找到编译好的可执行文件
    echo.
    echo 请先运行以下命令进行编译:
    echo   build.bat
    exit /b 1
)
exit /b 0

:print_header
echo.
echo ===================================
echo   %~1
echo ===================================
echo.
exit /b 0

:print_section
echo.
echo ^>^>^> %~1
echo -----------------------------------
exit /b 0