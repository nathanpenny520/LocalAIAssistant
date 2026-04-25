#!/usr/bin/env pwsh
# 本地AI助手 - Windows PowerShell 构建脚本

param(
    [switch]$Help,
    [switch]$Clean,
    [switch]$Debug,
    [switch]$Release,
    [switch]$Verbose,
    [int]$Jobs = 0,
    [string]$QtPath = "",
    [string]$Target = "all",
    [switch]$ListTargets
)

$BuildDir = "build"
$BuildType = if ($Debug) { "Debug" } else { "Release" }
$ErrorActionPreference = "Stop"

function Show-Help {
    Write-Host ""
    Write-Host "本地AI助手 - 构建脚本"
    Write-Host ""
    Write-Host "用法: $PSCommandPath [选项]"
    Write-Host ""
    Write-Host "选项:"
    Write-Host "  -Help              显示此帮助信息"
    Write-Host "  -Clean             清理构建目录后重新构建"
    Write-Host "  -Debug             构建调试版本"
    Write-Host "  -Release           构建发布版本（默认）"
    Write-Host "  -Verbose           显示详细输出"
    Write-Host "  -Jobs <n>          指定并行编译任务数"
    Write-Host "  -QtPath <path>     指定 Qt 安装路径"
    Write-Host "  -Target <name>     构建目标: all | LocalAIAssistant | LocalAIAssistant-CLI"
    Write-Host "  -ListTargets       列出所有可用目标"
    Write-Host ""
    Write-Host "示例:"
    Write-Host "  $PSCommandPath                     # 默认构建（Release 版本）"
    Write-Host "  $PSCommandPath -Clean              # 清理后重新构建"
    Write-Host "  $PSCommandPath -Debug              # 构建调试版本"
    Write-Host "  $PSCommandPath -Jobs 8             # 使用 8 个并行任务"
    Write-Host "  $PSCommandPath -Target LocalAIAssistant-CLI  # 仅构建 CLI"
    Write-Host "  $PSCommandPath -QtPath 'C:\Qt\6.10.2\msvc2019_64'  # 指定 Qt 路径"
    Write-Host ""
    Write-Host "环境变量:"
    Write-Host "  QT_PATH            Qt 安装路径（可通过 -QtPath 选项覆盖）"
    Write-Host "  Qt6_DIR            Qt6 cmake 目录"
    Write-Host ""
}

function List-Targets {
    Write-Host ""
    Write-Host "可用目标:"
    Write-Host "  - all                   构建所有目标"
    Write-Host "  - LocalAIAssistant      GUI 版本"
    Write-Host "  - LocalAIAssistant-CLI  CLI 版本"
    Write-Host ""
}

function Detect-QtPath {
    # Check environment variables first
    if ($env:QT_PATH) {
        $path = $env:QT_PATH
        if (Test-Path "$path\lib\Qt6Widgets.lib") {
            Write-Host "OK 检测到环境变量 QT_PATH: $path" -ForegroundColor Green
            return $path
        }
    }

    if ($env:Qt6_DIR) {
        $path = Split-Path (Split-Path $env:Qt6_DIR -Parent) -Parent
        if (Test-Path "$path\lib\Qt6Widgets.lib") {
            Write-Host "OK 检测到环境变量 Qt6_DIR: $path" -ForegroundColor Green
            return $path
        }
    }

    # Check common Qt installation paths
    $qtVersions = @("6.10.2", "6.10.1", "6.10.0", "6.9.2", "6.9.1", "6.9.0", "6.8.2", "6.8.1", "6.8.0", "6.7.2", "6.7.1", "6.7.0")
    $compilers = @("msvc2019_64", "msvc2022_64", "mingw1310_64")
    $drives = @("C:", "D:")

    foreach ($drive in $drives) {
        foreach ($version in $qtVersions) {
            foreach ($compiler in $compilers) {
                $path = "$drive\Qt\$version\$compiler"
                $libFile = if ($compiler -like "*mingw*") { "Qt6Widgets.a" } else { "Qt6Widgets.lib" }
                if (Test-Path "$path\lib\$libFile") {
                    Write-Host "OK 检测到 Qt 安装: $path" -ForegroundColor Green
                    return $path
                }
            }
        }
    }

    return $null
}

function Print-Banner {
    Write-Host ""
    Write-Host "===================================" -ForegroundColor Cyan
    Write-Host "  本地AI助手 - 构建脚本" -ForegroundColor Cyan
    Write-Host "===================================" -ForegroundColor Cyan
    Write-Host ""
}

# Main logic
if ($Help) {
    Show-Help
    exit 0
}

if ($ListTargets) {
    List-Targets
    exit 0
}

Print-Banner

# Check if in project root
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Host "错误: 未找到项目根目录" -ForegroundColor Red
    Write-Host "请在项目根目录运行此脚本"
    exit 1
}

# Check dependencies
Write-Host "检查依赖..."

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Host "  错误: 未找到 cmake" -ForegroundColor Red
    Write-Host "  请安装 CMake 或在 Visual Studio Developer Command Prompt 中运行"
    exit 1
}

$cmakeVersion = (cmake --version | Select-Object -First 1)
Write-Host "  OK CMake: $cmakeVersion" -ForegroundColor Green

# Detect Qt path
$DetectedQtPath = if ($QtPath) {
    Write-Host "OK 使用指定的 Qt 路径: $QtPath" -ForegroundColor Green
    $QtPath
} else {
    Detect-QtPath
}

if (-not $DetectedQtPath) {
    Write-Host ""
    Write-Host "错误: 未找到 Qt6 安装" -ForegroundColor Red
    Write-Host ""
    Write-Host "请通过以下方式之一指定 Qt 路径:"
    Write-Host "  1. 使用 -QtPath 选项: $PSCommandPath -QtPath 'C:\Qt\6.x.x\msvc2019_64'"
    Write-Host "  2. 设置环境变量: `$env:QT_PATH='C:\Qt\6.x.x\msvc2019_64'; $PSCommandPath"
    Write-Host ""
    Write-Host "安装 Qt6:"
    Write-Host "  Windows: 从 https://www.qt.io/download 下载安装"
    Write-Host "          选择 MSVC 版本 (如 Qt 6.x.x for MSVC 2019 64-bit)"
    exit 1
}

# Clean build if requested
if ($Clean) {
    Write-Host ""
    Write-Host "清理旧的构建文件..."
    if (Test-Path $BuildDir) {
        Remove-Item $BuildDir -Recurse -Force
    }
}

# Create build directory
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# Configure project
Write-Host ""
Write-Host "配置项目..."
Write-Host "  构建类型: $BuildType"
Write-Host "  目标: $Target"

$CmakeArgs = @(
    "..",
    "-DCMAKE_PREFIX_PATH=$DetectedQtPath",
    "-DCMAKE_BUILD_TYPE=$BuildType"
)

if ($Verbose) {
    $CmakeArgs += "-DCMAKE_VERBOSE_MAKEFILE=ON"
}

Push-Location $BuildDir

try {
    cmake $CmakeArgs
    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        Write-Host ""
        Write-Host "错误: CMake 配置失败" -ForegroundColor Red
        exit 1
    }

    # Build project
    Write-Host ""
    Write-Host "编译项目..."

    $BuildArgs = @(
        "--build", ".",
        "--config", $BuildType
    )

    if ($Jobs -gt 0) {
        $BuildArgs += "-j$Jobs"
    } else {
        $BuildArgs += "-j$env:NUMBER_OF_PROCESSORS"
    }

    if ($Target -ne "all") {
        $BuildArgs += "--target"
        $BuildArgs += $Target
    }

    if ($Verbose) {
        $BuildArgs += "-v"
    }

    $StartTime = Get-Date

    cmake $BuildArgs
    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        Write-Host ""
        Write-Host "错误: 编译失败" -ForegroundColor Red
        exit 1
    }

    $EndTime = Get-Date
    $Duration = ($EndTime - $StartTime).TotalSeconds

    Pop-Location

    # Success
    Write-Host ""
    Write-Host "===================================" -ForegroundColor Green
    Write-Host "  OK 构建成功! (耗时: $([int]$Duration)s)" -ForegroundColor Green
    Write-Host "===================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "可执行文件位置:"
    Write-Host "  GUI版本: $BuildDir\LocalAIAssistant.exe"
    Write-Host "  CLI版本: $BuildDir\LocalAIAssistant-CLI.exe"
    Write-Host ""
    Write-Host "运行命令:"
    Write-Host "  GUI版本: $BuildDir\LocalAIAssistant.exe"
    Write-Host "  CLI版本: $BuildDir\LocalAIAssistant-CLI.exe --help"
    Write-Host ""

    if ($BuildType -eq "Debug") {
        Write-Host "提示: 这是调试版本，包含调试符号但运行较慢"
        Write-Host "      如需发布版本，请使用 -Release 选项"
        Write-Host ""
    }

} catch {
    Pop-Location
    Write-Host ""
    Write-Host "错误: $_" -ForegroundColor Red
    exit 1
}