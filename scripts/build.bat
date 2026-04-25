@echo off
setlocal EnableDelayedExpansion

set BUILD_DIR=build
set QT_PATH=
set BUILD_TYPE=Release
set CLEAN_BUILD=0
set VERBOSE=0
set JOBS=
set TARGET=all

:: Parse arguments
:parse_args
if "%~1"=="" goto :done_args
if /I "%~1"=="/?" goto :show_help
if /I "%~1"=="/H" goto :show_help
if /I "%~1"=="/C" set CLEAN_BUILD=1& shift & goto :parse_args
if /I "%~1"=="/D" set BUILD_TYPE=Debug& shift & goto :parse_args
if /I "%~1"=="/R" set BUILD_TYPE=Release& shift & goto :parse_args
if /I "%~1"=="/V" set VERBOSE=1& shift & goto :parse_args
if /I "%~1"=="/T" goto :list_targets
:: Handle /J:n format (e.g., /J:8)
set "ARG1=%~1"
if /I "%ARG1:~0,3%"=="/J:" (
    set JOBS=%ARG1:~3%
    shift & goto :parse_args
)
if /I "%~1"=="/J" (
    set JOBS=%~2
    shift & shift & goto :parse_args
)
:: Handle /Q:path format (e.g., /Q:C:\Qt\6.10.2)
if /I "%ARG1:~0,3%"=="/Q:" (
    set QT_PATH=%ARG1:~3%
    shift & goto :parse_args
)
if /I "%~1"=="/Q" (
    set QT_PATH=%~2
    shift & shift & goto :parse_args
)
if /I "%~1"=="LocalAIAssistant" set TARGET=%~1& shift & goto :parse_args
if /I "%~1"=="LocalAIAssistant-CLI" set TARGET=%~1& shift & goto :parse_args
if /I "%~1"=="all" set TARGET=all& shift & goto :parse_args
echo 错误: 未知选项 %~1
echo 使用 /? 查看帮助信息
exit /b 1

:done_args

:: Print banner
echo.
echo ===================================
echo   本地AI助手 - 构建脚本
echo ===================================
echo.

:: Check if in project root
if not exist CMakeLists.txt (
    echo 错误: 未找到项目根目录
    echo 请在项目根目录运行此脚本
    exit /b 1
)

:: Check dependencies
echo 检查依赖...
where cmake >nul 2>&1
if errorlevel 1 (
    echo   错误: 未找到 cmake
    echo   请安装 CMake 或在 Visual Studio Developer Command Prompt 中运行
    exit /b 1
) else (
    for /f "tokens=3" %%v in ('cmake --version 2^>^&1') do (
        echo   OK CMake: %%v
        goto :cmake_done
    )
)
:cmake_done

:: Detect Qt path if not specified
if "%QT_PATH%"=="" (
    call :detect_qt_path
    if errorlevel 1 (
        echo.
        echo 错误: 未找到 Qt6 安装
        echo.
        echo 请通过以下方式之一指定 Qt 路径:
        echo   1. 使用 /Q 选项: %~nx0 /Q "C:\Qt\6.x.x\msvc2019_64"
        echo   2. 设置环境变量: set QT_PATH=C:\Qt\6.x.x\msvc2019_64 ^& %~nx0
        echo.
        echo 安装 Qt6:
        echo   Windows: 从 https://www.qt.io/download 下载安装
        echo           选择 MSVC 版本 (如 Qt 6.x.x for MSVC 2019 64-bit)
        exit /b 1
    )
) else (
    echo OK 使用指定的 Qt 路径: %QT_PATH%
)

:: Clean build if requested
if %CLEAN_BUILD%==1 (
    echo.
    echo 清理旧的构建文件...
    if exist "%BUILD_DIR%" rd /s /q "%BUILD_DIR%"
)

:: Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: Configure project
echo.
echo 配置项目...
echo   构建类型: %BUILD_TYPE%
echo   目标: %TARGET%

set "CMAKE_ARGS=-DCMAKE_PREFIX_PATH="%QT_PATH%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%"

if %VERBOSE%==1 (
    set "CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_VERBOSE_MAKEFILE=ON"
)

cd /d "%BUILD_DIR%"

cmake .. %CMAKE_ARGS%
if errorlevel 1 (
    cd ..
    echo.
    echo 错误: CMake 配置失败
    exit /b 1
)

:: Build project
echo.
echo 编译项目...

set BUILD_ARGS=--build . --config %BUILD_TYPE%

if not "%JOBS%"=="" (
    set BUILD_ARGS=%BUILD_ARGS% -j %JOBS%
) else (
    :: Use number of processors
    set BUILD_ARGS=%BUILD_ARGS% -j %NUMBER_OF_PROCESSORS%
)

if "%TARGET%" neq "all" (
    set BUILD_ARGS=%BUILD_ARGS% --target %TARGET%
)

if %VERBOSE%==1 (
    set BUILD_ARGS=%BUILD_ARGS% -v
)

cmake %BUILD_ARGS%
if errorlevel 1 (
    cd ..
    echo.
    echo 错误: 编译失败
    exit /b 1
)

:: Success
cd ..
echo.
echo ===================================
echo   OK 构建成功!
echo ===================================
echo.
echo 可执行文件位置:
echo   GUI版本: %BUILD_DIR%\LocalAIAssistant.exe
echo   CLI版本: %BUILD_DIR%\LocalAIAssistant-CLI.exe
echo.
echo 运行命令:
echo   GUI版本: %BUILD_DIR%\LocalAIAssistant.exe
echo   CLI版本: %BUILD_DIR%\LocalAIAssistant-CLI.exe --help
echo.

if "%BUILD_TYPE%"=="Debug" (
    echo 提示: 这是调试版本，包含调试符号但运行较慢
    echo       如需发布版本，请使用 /R 选项
    echo.
)

exit /b 0

:: ============================================
:: Functions
:: ============================================

:detect_qt_path
:: Check QT_PATH environment variable
if not "%QT_PATH%"=="" (
    if exist "%QT_PATH%\lib\Qt6Widgets.lib" (
        echo OK 检测到环境变量 QT_PATH: %QT_PATH%
        exit /b 0
    )
)

:: Check Qt6_DIR environment variable
if not "%Qt6_DIR%"=="" (
    set QT_PATH=%Qt6_DIR%\..\..
    if exist "%QT_PATH%\lib\Qt6Widgets.lib" (
        echo OK 检测到环境变量 Qt6_DIR: %QT_PATH%
        exit /b 0
    )
)

:: Check common Qt installation paths
for %%v in (6.10.2 6.10.1 6.10.0 6.9.2 6.9.1 6.9.0 6.8.2 6.8.1 6.8.0 6.7.2 6.7.1 6.7.0) do (
    if exist "C:\Qt\%%v\msvc2019_64\lib\Qt6Widgets.lib" (
        set QT_PATH=C:\Qt\%%v\msvc2019_64
        echo OK 检测到 Qt 安装: %QT_PATH%
        exit /b 0
    )
    if exist "C:\Qt\%%v\msvc2022_64\lib\Qt6Widgets.lib" (
        set QT_PATH=C:\Qt\%%v\msvc2022_64
        echo OK 检测到 Qt 安装: %QT_PATH%
        exit /b 0
    )
    if exist "C:\Qt\%%v\mingw1310_64\lib\Qt6Widgets.a" (
        set QT_PATH=C:\Qt\%%v\mingw1310_64
        echo OK 检测到 Qt 安装: %QT_PATH%
        exit /b 0
    )
)

:: Check D:\Qt as alternative drive
for %%v in (6.10.2 6.10.1 6.10.0 6.9.2 6.9.1 6.9.0 6.8.2 6.8.1 6.8.0 6.7.2 6.7.1 6.7.0) do (
    if exist "D:\Qt\%%v\msvc2019_64\lib\Qt6Widgets.lib" (
        set QT_PATH=D:\Qt\%%v\msvc2019_64
        echo OK 检测到 Qt 安装: %QT_PATH%
        exit /b 0
    )
    if exist "D:\Qt\%%v\msvc2022_64\lib\Qt6Widgets.lib" (
        set QT_PATH=D:\Qt\%%v\msvc2022_64
        echo OK 检测到 Qt 安装: %QT_PATH%
        exit /b 0
    )
    if exist "D:\Qt\%%v\mingw1310_64\lib\Qt6Widgets.a" (
        set QT_PATH=D:\Qt\%%v\mingw1310_64
        echo OK 检测到 Qt 安装: %QT_PATH%
        exit /b 0
    )
)

exit /b 1

:show_help
echo.
echo 本地AI助手 - 构建脚本
echo.
echo 用法: %~nx0 [选项] [目标]
echo.
echo 选项:
echo   /?              显示此帮助信息
echo   /C              清理构建目录后重新构建
echo   /D              构建调试版本
echo   /R              构建发布版本（默认）
echo   /V              显示详细输出
echo   /J:n            指定并行编译任务数
echo   /Q:path         指定 Qt 安装路径
echo   /T              列出所有可用目标
echo.
echo 目标:
echo   all                     构建所有目标（默认）
echo   LocalAIAssistant        仅构建 GUI 版本
echo   LocalAIAssistant-CLI    仅构建 CLI 版本
echo.
echo 示例:
echo   %~nx0                      默认构建（Release 版本）
echo   %~nx0 /C                   清理后重新构建
echo   %~nx0 /D                   构建调试版本
echo   %~nx0 /J:8                 使用 8 个并行任务
echo   %~nx0 LocalAIAssistant-CLI 仅构建 CLI 版本
echo   %~nx0 /Q "C:\Qt\6.10.2\msvc2019_64" 指定 Qt 路径
echo.
echo 环境变量:
echo   QT_PATH                 Qt 安装路径（可通过 /Q 选项覆盖）
echo   Qt6_DIR                 Qt6 cmake 目录
echo.
exit /b 0

:list_targets
echo.
echo 可用目标:
echo   - all                   构建所有目标
echo   - LocalAIAssistant      GUI 版本
echo   - LocalAIAssistant-CLI  CLI 版本
echo.
exit /b 0