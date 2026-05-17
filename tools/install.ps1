<#
.SYNOPSIS
    Install MobuFaceMotion's device_faceMotion.dll into every detected
    MotionBuilder installation, picking the matching per-version build.

.DESCRIPTION
    Default behaviour (no arguments): scan `C:\Program Files\Autodesk\` for
    every `MotionBuilder <ver>` directory, and for each one copy the
    matching `build/<ver>/src/device_faceMotion/Release/device_faceMotion.dll`
    into `<ver>/bin/x64/plugins/`. Self-elevates via UAC if not already
    running as administrator.

    Versions for which no built .dll exists are skipped with a warning so
    the script is safe to run after partial builds.

    Pass -MobuVersion to install to a single MoBu version, or -DllPath to
    point at a .dll outside the build/<ver> convention.

.EXAMPLE
    .\tools\install.ps1
    Installs each available .dll into the matching MotionBuilder install.

.EXAMPLE
    .\tools\install.ps1 -MobuVersion 2027
    Installs only into MotionBuilder 2027.

.EXAMPLE
    .\tools\install.ps1 -DllPath D:\drop\device_faceMotion.dll -MobuVersion 2026
    Installs a single .dll from an arbitrary path into MotionBuilder 2026.
#>
[CmdletBinding()]
param(
    [string]$MobuVersion,
    [string]$DllPath,
    [string]$MobuRoot
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$autodeskRoot = 'C:\Program Files\Autodesk'

function Get-MobuInstalls {
    if (-not (Test-Path $autodeskRoot)) { return @() }
    Get-ChildItem $autodeskRoot -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^MotionBuilder (\d+)$' } |
        ForEach-Object {
            $ver = $Matches[1]
            [pscustomobject]@{
                Version    = $ver
                Root       = $_.FullName
                PluginsDir = Join-Path $_.FullName 'bin\x64\plugins'
            }
        } | Sort-Object Version
}

function Resolve-DllForVersion {
    param([string]$ver)
    Join-Path $repoRoot "build\$ver\src\device_faceMotion\Release\device_faceMotion.dll"
}

# Build the targets list.
$targets = @()
if ($MobuVersion) {
    if (-not $MobuRoot) { $MobuRoot = Join-Path $autodeskRoot "MotionBuilder $MobuVersion" }
    if (-not $DllPath)  { $DllPath  = Resolve-DllForVersion $MobuVersion }
    $targets += [pscustomobject]@{
        Version    = $MobuVersion
        Root       = $MobuRoot
        PluginsDir = Join-Path $MobuRoot 'bin\x64\plugins'
        DllPath    = $DllPath
    }
} else {
    $installs = Get-MobuInstalls
    if (-not $installs) {
        throw "No MotionBuilder installations found under $autodeskRoot."
    }
    foreach ($i in $installs) {
        $targets += [pscustomobject]@{
            Version    = $i.Version
            Root       = $i.Root
            PluginsDir = $i.PluginsDir
            DllPath    = Resolve-DllForVersion $i.Version
        }
    }
}

# Filter to targets whose source .dll actually exists.
$valid   = @($targets | Where-Object { Test-Path $_.DllPath })
$missing = @($targets | Where-Object { -not (Test-Path $_.DllPath) })

Write-Host ''
Write-Host 'MobuFaceMotion installer' -ForegroundColor Cyan
Write-Host '------------------------'
foreach ($t in $valid) {
    Write-Host ('  [will install] MotionBuilder {0}  <-  {1}' -f $t.Version, $t.DllPath)
}
foreach ($t in $missing) {
    Write-Host ('  [   skipped  ] MotionBuilder {0}  (no .dll at {1})' -f $t.Version, $t.DllPath) -ForegroundColor Yellow
}
Write-Host ''

if (-not $valid) {
    Write-Host 'Nothing to install. Build first:' -ForegroundColor Yellow
    Write-Host '  cmake -S . -B build\<ver> -G "Visual Studio 18 2026" -A x64 -DMOBU_VERSION=<ver>'
    Write-Host '  cmake --build build\<ver> --config Release'
    exit 1
}

# Self-elevate if needed (admin required to write under C:\Program Files).
$id = [Security.Principal.WindowsIdentity]::GetCurrent()
$pr = New-Object Security.Principal.WindowsPrincipal($id)
if (-not $pr.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host 'Re-launching elevated (accept UAC)...'
    $boundArgs = @()
    foreach ($k in $PSBoundParameters.Keys) {
        $boundArgs += "-$k"; $boundArgs += "`"$($PSBoundParameters[$k])`""
    }
    $argList = @('-NoProfile','-ExecutionPolicy','Bypass','-File',"`"$PSCommandPath`"") + $boundArgs
    $p = Start-Process -FilePath powershell.exe -ArgumentList $argList -Verb RunAs -Wait -PassThru
    exit $p.ExitCode
}

# Refuse to overwrite a loaded .dll -- check MoBu isn't running.
$running = Get-Process -Name 'motionbuilder' -ErrorAction SilentlyContinue
if ($running) {
    throw ("MotionBuilder is running (PID $($running.Id)). Close it before installing -- " +
           'Windows will not overwrite a loaded plugin.')
}

$installedCount = 0
foreach ($t in $valid) {
    if (-not (Test-Path $t.PluginsDir)) {
        New-Item -ItemType Directory -Path $t.PluginsDir -Force | Out-Null
    }
    $dst = Join-Path $t.PluginsDir 'device_faceMotion.dll'
    Copy-Item -LiteralPath $t.DllPath -Destination $dst -Force
    Write-Host ('  installed -> {0}' -f $dst) -ForegroundColor Green
    $installedCount++
}

Write-Host ''
Write-Host ('Done. {0} install(s) updated.' -f $installedCount) -ForegroundColor Green
Write-Host 'Re-launch any MotionBuilder you had open to pick up the new plugin.'
