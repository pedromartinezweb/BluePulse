param(
    [string]$FilePath = 'dist\BluePulse.exe'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

$target = if ([System.IO.Path]::IsPathRooted($FilePath)) { $FilePath } else { Join-Path $repoRoot $FilePath }

if (-not (Test-Path $target)) {
    Write-Error "Executable not found: $target"
    exit 1
}

function Resolve-SignTool {
    $command = Get-Command signtool.exe -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $programFilesX86 = [Environment]::GetEnvironmentVariable('ProgramFiles(x86)')
    $windowsKitsRoot = if ($programFilesX86) { Join-Path $programFilesX86 'Windows Kits\10\bin' } else { $null }

    if ($windowsKitsRoot -and (Test-Path $windowsKitsRoot)) {
        $candidate = Get-ChildItem -Path $windowsKitsRoot -Filter signtool.exe -Recurse -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\x64\\signtool\.exe$' } |
            Sort-Object FullName -Descending |
            Select-Object -First 1

        if ($candidate) {
            return $candidate.FullName
        }
    }

    return $null
}

$signTool = Resolve-SignTool
if (-not $signTool) {
    Write-Error 'Unable to locate signtool.exe. Install Windows SDK or run on windows-latest in GitHub Actions.'
    exit 2
}

& $signTool verify /pa /v $target
