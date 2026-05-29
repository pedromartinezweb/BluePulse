param(
    [string]$OutputDir = 'dist',
    [switch]$Clean,
    [switch]$Deep
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$scriptPath = Join-Path $repoRoot 'scripts\build.ps1'

if (-not (Test-Path $scriptPath)) {
    Write-Error "Build entrypoint not found: $scriptPath"
    exit 1
}

& $scriptPath -OutputDir $OutputDir -Clean:$Clean -Deep:$Deep
