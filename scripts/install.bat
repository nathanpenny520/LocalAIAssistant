@echo off
setlocal EnableDelayedExpansion

set INSTALL_PREFIX=C:\Program Files\LocalAIAssistant
set EXECUTABLE_NAME=ai.exe
set INSTALL_CLI=1
set INSTALL_GUI=0
set UNINSTALL=0
set FORCE=0

:: Parse arguments
:parse_args
if "%~1"=="" goto :done_args
if /I "%~1"=="/?" goto :show_help
if /I "%~1"=="/H" goto :show_help
if /I "%~1"=="/HELP" goto :show_help
if /I "%~1"=="/CLI" set INSTALL_CLI=1& shift & goto :parse_args
if /I "%~1"=="/GUI" set INSTALL_GUI=1& shift & goto :parse_args
if /I "%~1"=="/ALL" set INSTALL_CLI=1& set INSTALL_GUI=1& shift & goto :parse_args
if /I "%~1"=="/UNINSTALL" set UNINSTALL=1& shift & goto :parse_args
if /I "%~1"=="/U" set UNINSTALL=1& shift & goto :parse_args
if /I "%~1"=="/F" set FORCE=1& shift & goto :parse_args
if /I "%~1"=="/FORCE" set FORCE=1& shift & goto :parse_args
:: Handle /PREFIX:path format
set "ARG1=%~1"
if /I "%ARG1:~0,8%"=="/PREFIX:" (
    set INSTALL_PREFIX=%ARG1:~8%
    shift & goto :parse_args
)
if /I "%~1"=="/PREFIX" (
    set INSTALL_PREFIX=%~2
    shift & shift & goto :parse_args
)
echo 错误: 未知选项 %~1
echo 使用 /? 查看帮助信息
exit /b 1

:done_args

:: Print banner
echo.
echo ===================================
echo   本地AI助手 - 安装脚本
echo ===================================
echo.

:: Check if in project root
if not exist CMakeLists.txt (
    echo 错误: 未找到项目根目录
    echo 请在项目根目录运行此脚本
    exit /b 1
)

:: Handle uninstall
if %UNINSTALL%==1 (
    call :uninstall
    exit /b 0
)

:: Check executables
call :check_executables
if errorlevel 1 exit /b 1

:: Confirm installation
call :confirm_install
if errorlevel 1 exit /b 0

:: Install
if %INSTALL_CLI%==1 call :install_cli
if %INSTALL_GUI%==1 call :install_gui

echo.
echo ===================================
echo   安装完成
echo ===================================
echo.
exit /b 0

:: ============================================
:: Functions
:: ============================================

:check_executables
set MISSING=
if %INSTALL_CLI%==1 (
    if not exist "build\LocalAIAssistant-CLI.exe" (
        set MISSING=1
        echo   错误: 未找到 CLI 版本: build\LocalAIAssistant-CLI.exe
    )
)
if %INSTALL_GUI%==1 (
    if not exist "build\LocalAIAssistant.exe" (
        set MISSING=1
        echo   错误: 未找到 GUI 版本: build\LocalAIAssistant.exe
    )
)
if "%MISSING%"=="1" (
    echo.
    echo 请先运行 build.bat 进行编译
    exit /b 1
)
exit /b 0

:confirm_install
if %FORCE%==1 exit /b 0
echo.
echo 将安装以下组件:
if %INSTALL_CLI%==1 echo   - CLI 版本 (%INSTALL_PREFIX%\%EXECUTABLE_NAME%)
if %INSTALL_GUI%==1 echo   - GUI 版本 (%INSTALL_PREFIX%\LocalAIAssistant.exe)
echo.
set /p CONFIRM="确认继续? [y/N] "
if /I "%CONFIRM%"=="y" exit /b 0
echo 操作已取消
exit /b 1

:install_cli
echo.
echo 安装 CLI 版本...
echo   目标路径: %INSTALL_PREFIX%\%EXECUTABLE_NAME%

if not exist "%INSTALL_PREFIX%" mkdir "%INSTALL_PREFIX%"

if exist "%INSTALL_PREFIX%\%EXECUTABLE_NAME%" (
    echo   发现已存在的安装，正在替换...
    del /f /q "%INSTALL_PREFIX%\%EXECUTABLE_NAME%" >nul 2>&1
)

copy /y "build\LocalAIAssistant-CLI.exe" "%INSTALL_PREFIX%\%EXECUTABLE_NAME%" >nul
if errorlevel 1 (
    echo   错误: 复制文件失败，可能需要管理员权限
    echo   请以管理员身份运行此脚本
    exit /b 1
)

echo   √ CLI 版本安装成功
echo.
echo 现在可以使用以下命令:
echo   "%INSTALL_PREFIX%\%EXECUTABLE_NAME%" --help
echo   "%INSTALL_PREFIX%\%EXECUTABLE_NAME%" chat
echo   "%INSTALL_PREFIX%\%EXECUTABLE_NAME%" ask "你的问题"
echo.

:: 提示用户手动添加 PATH（避免 setx 截断问题）
echo   提示: 请将 %INSTALL_PREFIX% 添加到 PATH 环境变量
echo         方法: 右键"此电脑" -> 属性 -> 高级系统设置 -> 环境变量
echo         或使用 install.ps1 进行自动配置
exit /b 0

:install_gui
echo.
echo 安装 GUI 版本...
echo   目标路径: %INSTALL_PREFIX%\LocalAIAssistant.exe

if not exist "%INSTALL_PREFIX%" mkdir "%INSTALL_PREFIX%"

if exist "%INSTALL_PREFIX%\LocalAIAssistant.exe" (
    echo   发现已存在的安装，正在替换...
    del /f /q "%INSTALL_PREFIX%\LocalAIAssistant.exe" >nul 2>&1
)

copy /y "build\LocalAIAssistant.exe" "%INSTALL_PREFIX%\LocalAIAssistant.exe" >nul
if errorlevel 1 (
    echo   错误: 复制文件失败，可能需要管理员权限
    echo   请以管理员身份运行此脚本
    exit /b 1
)

echo   √ GUI 版本安装成功
echo.
echo 现在可以通过以下方式运行:
echo   - 在文件资源管理器中找到 %INSTALL_PREFIX%\LocalAIAssistant.exe
echo   - 或运行: "%INSTALL_PREFIX%\LocalAIAssistant.exe"
exit /b 0

:uninstall
echo.
echo 将卸载以下组件:
echo   - CLI 版本 (%INSTALL_PREFIX%\%EXECUTABLE_NAME%)
if %INSTALL_GUI%==1 echo   - GUI 版本 (%INSTALL_PREFIX%\LocalAIAssistant.exe)
echo.

if %FORCE%==0 (
    set /p CONFIRM="确认继续? [y/N] "
    if /I not "!CONFIRM!"=="y" (
        echo 操作已取消
        exit /b 0
    )
)

echo.
echo 卸载 CLI 版本...
if exist "%INSTALL_PREFIX%\%EXECUTABLE_NAME%" (
    del /f /q "%INSTALL_PREFIX%\%EXECUTABLE_NAME%" >nul 2>&1
    echo   √ CLI 版本已卸载
) else (
    echo   CLI 版本未安装
)

echo.
echo 卸载 GUI 版本...
if exist "%INSTALL_PREFIX%\LocalAIAssistant.exe" (
    del /f /q "%INSTALL_PREFIX%\LocalAIAssistant.exe" >nul 2>&1
    echo   √ GUI 版本已卸载
) else (
    echo   GUI 版本未安装
)

:: Remove directory if empty
dir /b "%INSTALL_PREFIX%" 2>nul | findstr "^" >nul || rd "%INSTALL_PREFIX%" 2>nul

echo.
echo ===================================
echo   卸载完成
echo ===================================
exit /b 0

:show_help
echo.
echo 本地AI助手 - 安装/卸载脚本
echo.
echo 用法: %~nx0 [选项]
echo.
echo 选项:
echo   /?              显示此帮助信息
echo   /CLI            安装 CLI 版本 (默认)
echo   /GUI            安装 GUI 版本
echo   /ALL            安装 CLI 和 GUI 版本
echo   /PREFIX:path    安装路径前缀 (默认: C:\Program Files\LocalAIAssistant)
echo   /UNINSTALL       卸载已安装的程序
echo   /F              强制安装/卸载，不询问确认
echo.
echo 示例:
echo   %~nx0                      # 安装 CLI 版本到默认位置
echo   %~nx0 /CLI                 # 同上
echo   %~nx0 /GUI                # 安装 GUI 版本
echo   %~nx0 /ALL                # 安装 CLI 和 GUI 版本
echo   %~nx0 /PREFIX "%LOCALAPPDATA%\LocalAIAssistant"  # 安装到用户目录
echo   %~nx0 /UNINSTALL          # 卸载 CLI 版本
echo   %~nx0 /UNINSTALL /ALL     # 卸载所有版本
echo.
echo 注意:
echo   - 安装到 Program Files 需要管理员权限
echo   - 安装到用户目录不需要管理员权限
echo.
exit /b 0