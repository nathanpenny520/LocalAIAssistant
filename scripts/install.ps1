#!/usr/bin/env pwsh
# 本地AI助手 - Windows PowerShell 安装/卸载脚本

param(
    [switch]$Help,
    [string]$Prefix = "",
    [switch]$Cli,
    [switch]$Gui,
    [switch]$All,
    [switch]$Uninstall,
    [switch]$Force
)

# Default values
if (-not $Cli -and -not $Gui -and -not $All) {
    $Cli = $true
}
if ($All) {
    $Cli = $true
    $Gui = $true
}

# Default installation prefix
if ([string]::IsNullOrEmpty($Prefix)) {
    if ($Uninstall) {
        # Try to detect previous installation location
        $envPrefix = [Environment]::GetEnvironmentVariable("LocalAIAssistant_Path", "User")
        if ($envPrefix) {
            $InstallPrefix = $envPrefix
        } else {
            $InstallPrefix = "$env:LOCALAPPDATA\LocalAIAssistant"
        }
    } else {
        # Default to user directory to avoid needing admin rights
        $InstallPrefix = "$env:LOCALAPPDATA\LocalAIAssistant"
    }
} else {
    $InstallPrefix = $Prefix
}

$ExecutableName = "ai.exe"
$ErrorActionPreference = "Stop"

function Show-Help {
    Write-Host ""
    Write-Host "本地AI助手 - 安装/卸载脚本"
    Write-Host ""
    Write-Host "用法: $PSCommandPath [选项]"
    Write-Host ""
    Write-Host "选项:"
    Write-Host "  -Help              显示此帮助信息"
    Write-Host "  -Prefix <path>     安装路径前缀 (默认: `$env:LOCALAPPDATA\LocalAIAssistant)"
    Write-Host "  -Cli               安装 CLI 版本 (默认)"
    Write-Host "  -Gui               安装 GUI 版本"
    Write-Host "  -All               安装 CLI 和 GUI 版本"
    Write-Host "  -Uninstall         卸载已安装的程序"
    Write-Host "  -Force             强制安装/卸载，不询问确认"
    Write-Host ""
    Write-Host "示例:"
    Write-Host "  $PSCommandPath                     # 安装 CLI 版本到用户目录"
    Write-Host "  $PSCommandPath -Cli                # 同上"
    Write-Host "  $PSCommandPath -Gui                # 安装 GUI 版本"
    Write-Host "  $PSCommandPath -All                # 安装 CLI 和 GUI 版本"
    Write-Host "  $PSCommandPath -Prefix 'C:\Program Files\LocalAIAssistant'  # 安装到系统目录"
    Write-Host "  $PSCommandPath -Uninstall          # 卸载 CLI 版本"
    Write-Host "  $PSCommandPath -Uninstall -All    # 卸载所有版本"
    Write-Host ""
    Write-Host "注意:"
    Write-Host "  - 安装到 Program Files 需要管理员权限"
    Write-Host "  - 安装到用户目录 (`$env:LOCALAPPDATA) 不需要管理员权限"
    Write-Host ""
}

function Test-AdminPrivileges {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Test-Executables {
    $missing = @()

    if ($Cli) {
        if (-not (Test-Path "build\LocalAIAssistant-CLI.exe")) {
            $missing += "CLI版本: build\LocalAIAssistant-CLI.exe"
        }
    }

    if ($Gui) {
        if (-not (Test-Path "build\LocalAIAssistant.exe")) {
            $missing += "GUI版本: build\LocalAIAssistant.exe"
        }
    }

    if ($missing.Count -gt 0) {
        Write-Host "错误: 未找到以下可执行文件:" -ForegroundColor Red
        foreach ($file in $missing) {
            Write-Host "  - $file"
        }
        Write-Host ""
        Write-Host "请先运行 .\build.ps1 进行编译"
        return $false
    }

    return $true
}

function Test-SystemDirectory {
    param([string]$Path)

    $systemDirs = @(
        "$env:SystemDrive\Program Files",
        "$env:SystemDrive\Program Files (x86)",
        "$env:SystemDrive\ProgramData"
    )

    foreach ($dir in $systemDirs) {
        if ($Path -like "$dir*") {
            return $true
        }
    }

    return $false
}

function Confirm-Action {
    param([string]$Message)

    if ($Force) {
        return $true
    }

    Write-Host ""
    Write-Host $Message
    $response = Read-Host "确认继续? [y/N]"

    if ($response -notmatch "^[Yy]$") {
        Write-Host "操作已取消"
        return $false
    }

    return $true
}

function Install-Cli {
    $binDir = $InstallPrefix
    $target = Join-Path $binDir $ExecutableName

    Write-Host ""
    Write-Host "安装 CLI 版本..."
    Write-Host "  目标路径: $target"

    if (-not (Test-Path $binDir)) {
        New-Item -ItemType Directory -Path $binDir -Force | Out-Null
    }

    if (Test-Path $target) {
        Write-Host "  发现已存在的安装，正在替换..."
        Remove-Item $target -Force
    }

    Copy-Item "build\LocalAIAssistant-CLI.exe" $target -Force

    # Save installation path for future uninstall
    [Environment]::SetEnvironmentVariable("LocalAIAssistant_Path", $InstallPrefix, "User")

    Write-Host "  ✓ CLI 版本安装成功" -ForegroundColor Green
    Write-Host ""
    Write-Host "现在可以使用以下命令:"
    Write-Host "  $target --help"
    Write-Host "  $target chat"
    Write-Host "  $target ask `"你的问题`""

    # Add to PATH
    $currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if ($currentPath -notlike "*$binDir*") {
        [Environment]::SetEnvironmentVariable("Path", "$currentPath;$binDir", "User")
        Write-Host ""
        Write-Host "  提示: 已将 $binDir 添加到 PATH 环境变量" -ForegroundColor Yellow
        Write-Host "        请重新打开 PowerShell 或命令提示符以生效" -ForegroundColor Yellow
    }
}

function Install-Gui {
    $target = Join-Path $InstallPrefix "LocalAIAssistant.exe"

    Write-Host ""
    Write-Host "安装 GUI 版本..."
    Write-Host "  目标路径: $target"

    if (-not (Test-Path $InstallPrefix)) {
        New-Item -ItemType Directory -Path $InstallPrefix -Force | Out-Null
    }

    if (Test-Path $target) {
        Write-Host "  发现已存在的安装，正在替换..."
        Remove-Item $target -Force
    }

    Copy-Item "build\LocalAIAssistant.exe" $target -Force

    Write-Host "  ✓ GUI 版本安装成功" -ForegroundColor Green
    Write-Host ""
    Write-Host "现在可以通过以下方式运行:"
    Write-Host "  - 在文件资源管理器中找到 $target"
    Write-Host "  - 或运行: $target"

    # Create desktop shortcut (optional)
    $desktop = [Environment]::GetFolderPath("Desktop")
    $shortcutPath = Join-Path $desktop "LocalAIAssistant.lnk"

    if (-not (Test-Path $shortcutPath)) {
        try {
            $WScriptShell = New-Object -ComObject WScript.Shell
            $shortcut = $WScriptShell.CreateShortcut($shortcutPath)
            $shortcut.TargetPath = $target
            $shortcut.Description = "本地AI助手"
            $shortcut.Save()
            Write-Host ""
            Write-Host "  提示: 已创建桌面快捷方式" -ForegroundColor Yellow
        } catch {
            # Ignore shortcut creation errors
        }
    }
}

function Uninstall-Cli {
    $target = Join-Path $InstallPrefix $ExecutableName

    Write-Host ""
    Write-Host "卸载 CLI 版本..."

    if (-not (Test-Path $target)) {
        Write-Host "  ⚠️ CLI 版本未安装" -ForegroundColor Yellow
        return
    }

    Remove-Item $target -Force
    Write-Host "  ✓ CLI 版本已卸载" -ForegroundColor Green

    # Remove from PATH
    $currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if ($currentPath -like "*$InstallPrefix*") {
        $newPath = ($currentPath -split ';' | Where-Object { $_ -ne $InstallPrefix }) -join ';'
        [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
        Write-Host "  已从 PATH 移除 $InstallPrefix" -ForegroundColor Yellow
    }
}

function Uninstall-Gui {
    $target = Join-Path $InstallPrefix "LocalAIAssistant.exe"

    Write-Host ""
    Write-Host "卸载 GUI 版本..."

    if (-not (Test-Path $target)) {
        Write-Host "  ⚠️ GUI 版本未安装" -ForegroundColor Yellow
        return
    }

    Remove-Item $target -Force
    Write-Host "  ✓ GUI 版本已卸载" -ForegroundColor Green

    # Remove desktop shortcut
    $desktop = [Environment]::GetFolderPath("Desktop")
    $shortcutPath = Join-Path $desktop "LocalAIAssistant.lnk"

    if (Test-Path $shortcutPath) {
        Remove-Item $shortcutPath -Force -ErrorAction SilentlyContinue
    }
}

function Remove-InstallDirectory {
    # Remove directory if empty
    if ((Test-Path $InstallPrefix) -and -not (Get-ChildItem $InstallPrefix -ErrorAction SilentlyContinue)) {
        Remove-Item $InstallPrefix -Force
        Write-Host "  已删除安装目录: $InstallPrefix" -ForegroundColor Yellow
    }

    # Clean up environment variable
    [Environment]::SetEnvironmentVariable("LocalAIAssistant_Path", $null, "User")
}

function Print-Banner {
    Write-Host ""
    Write-Host "===================================" -ForegroundColor Cyan
    Write-Host "  本地AI助手 - 安装脚本" -ForegroundColor Cyan
    Write-Host "===================================" -ForegroundColor Cyan
    Write-Host ""
}

# Main logic
if ($Help) {
    Show-Help
    exit 0
}

Print-Banner

# Check if in project root
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Host "错误: 未找到项目根目录" -ForegroundColor Red
    Write-Host "请在项目根目录运行此脚本"
    exit 1
}

# Check admin privileges for system directories
if (Test-SystemDirectory $InstallPrefix) {
    if (-not (Test-AdminPrivileges)) {
        Write-Host "错误: 安装到系统目录需要管理员权限" -ForegroundColor Red
        Write-Host ""
        Write-Host "请使用以下方式之一:"
        Write-Host "  1. 以管理员身份运行 PowerShell"
        Write-Host "  2. 安装到用户目录: $PSCommandPath -Prefix `"`$env:LOCALAPPDATA\LocalAIAssistant`""
        exit 1
    }
}

if ($Uninstall) {
    $action = "将卸载以下组件:"
    if ($Cli) { $action += "`n  - CLI 版本 ($InstallPrefix\$ExecutableName)" }
    if ($Gui) { $action += "`n  - GUI 版本 ($InstallPrefix\LocalAIAssistant.exe)" }

    if (-not (Confirm-Action $action)) {
        exit 0
    }

    if ($Cli) { Uninstall-Cli }
    if ($Gui) { Uninstall-Gui }
    Remove-InstallDirectory

    Write-Host ""
    Write-Host "===================================" -ForegroundColor Green
    Write-Host "  ✓ 卸载完成" -ForegroundColor Green
    Write-Host "==================================="
} else {
    if (-not (Test-Executables)) {
        exit 1
    }

    $action = "将安装以下组件:"
    if ($Cli) { $action += "`n  - CLI 版本 ($InstallPrefix\$ExecutableName)" }
    if ($Gui) { $action += "`n  - GUI 版本 ($InstallPrefix\LocalAIAssistant.exe)" }

    if (-not (Confirm-Action $action)) {
        exit 0
    }

    if ($Cli) { Install-Cli }
    if ($Gui) { Install-Gui }

    Write-Host ""
    Write-Host "===================================" -ForegroundColor Green
    Write-Host "  ✓ 安装完成" -ForegroundColor Green
    Write-Host "==================================="
}