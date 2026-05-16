<#
.SYNOPSIS
    Install MobuFaceMotion's device_faceMotion.dll into MotionBuilder's plugins folder.

.DESCRIPTION
    Copies the built plugin into <MoBu>/bin/x64/plugins/. Self-elevates via UAC
    if not already running as administrator.

    Pass -MobuVersion to target a non-default version, or -DllPath to point at
    a .dll outside the conventional build/<ver>/ location.

.EXAMPLE
    .\tools\install.ps1
    Installs build/2027/src/device_faceMotion/Release/device_faceMotion.dll
    into "C:\Program Files\Autodesk\MotionBuilder 2027\bin\x64\plugins".

.EXAMPLE
    .\tools\install.ps1 -MobuVersion 2026
    Installs build/2026/... into MotionBuilder 2026.

.EXAMPLE
    .\tools\install.ps1 -DllPath D:\drop\device_faceMotion.dll -MobuVersion 2025
#>
[CmdletBinding()]
param(
    [string]$MobuVersion,
    [string]$DllPath,
    [string]$MobuRoot
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot

if (-not $MobuVersion) {
    $MobuVersion = (Get-Content (Join-Path $repoRoot 'PRODUCT_VERSION.txt') -Raw).Trim()
}

if (-not $MobuRoot) {
    $MobuRoot = "C:\Program Files\Autodesk\MotionBuilder $MobuVersion"
}

if (-not $DllPath) {
    $DllPath = Join-Path $repoRoot "build\$MobuVersion\src\device_faceMotion\Release\device_faceMotion.dll"
}

if (-not (Test-Path $DllPath)) {
    throw "Built .dll not found at: $DllPath`nBuild it first: cmake --build build\$MobuVersion --config Release"
}

if (-not (Test-Path $MobuRoot)) {
    throw "MotionBuilder $MobuVersion not found at: $MobuRoot`nPass -MobuRoot to point at a custom install."
}

$dstDir = Join-Path $MobuRoot 'bin\x64\plugins'
$dst    = Join-Path $dstDir 'device_faceMotion.dll'

Write-Host "Source     : $DllPath"
Write-Host "Destination: $dst"

# Self-elevate if needed.
$id = [Security.Principal.WindowsIdentity]::GetCurrent()
$pr = New-Object Security.Principal.WindowsPrincipal($id)
if (-not $pr.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "Re-launching elevated (accept UAC)..."
    $boundArgs = @()
    foreach ($k in $PSBoundParameters.Keys) {
        $boundArgs += "-$k"; $boundArgs += "`"$($PSBoundParameters[$k])`""
    }
    $argList = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"") + $boundArgs
    $p = Start-Process -FilePath powershell.exe -ArgumentList $argList -Verb RunAs -Wait -PassThru
    exit $p.ExitCode
}

if (-not (Test-Path $dstDir)) {
    New-Item -ItemType Directory -Path $dstDir -Force | Out-Null
}

Copy-Item -LiteralPath $DllPath -Destination $dst -Force
Write-Host "Installed OK." -ForegroundColor Green
Write-Host "Restart MotionBuilder to load the new plugin."
