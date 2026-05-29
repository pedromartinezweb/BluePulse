param(
    [string]$OutputDir = 'dist',
    [switch]$Clean,
    [switch]$Deep
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

$sourceFile = Join-Path $repoRoot 'src\BluePulse\BluePulse.c'
$resourceDir = Join-Path $repoRoot 'assets\windows'
$resourceFile = Join-Path $resourceDir 'BluePulse.rc'
$intermediateDir = Join-Path $repoRoot 'build\windows'
$resourceObject = Join-Path $intermediateDir 'BluePulse.res.o'
$tempExe = Join-Path $intermediateDir 'BluePulse.exe'
$finalOutputDir = if ([System.IO.Path]::IsPathRooted($OutputDir)) { $OutputDir } else { Join-Path $repoRoot $OutputDir }
$finalExe = Join-Path $finalOutputDir 'BluePulse.exe'

function Invoke-Cleanup {
    param(
        [string]$RepoRoot,
        [switch]$Deep
    )

    Get-Process -Name BluePulse -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

    $pathsToRemove = @(
        (Join-Path $RepoRoot 'build'),
        (Join-Path $RepoRoot 'dist'),
        (Join-Path $RepoRoot 'BluePulse.exe'),
        (Join-Path $RepoRoot 'BluePulseV1.exe'),
        (Join-Path $RepoRoot 'win-install-mingw.log')
    )

    foreach ($path in $pathsToRemove) {
        if (Test-Path $path) {
            Remove-Item -LiteralPath $path -Recurse -Force -ErrorAction SilentlyContinue
        }
    }

    Get-ChildItem -Path $RepoRoot -Filter '*.log' -File -ErrorAction SilentlyContinue |
        Remove-Item -Force -ErrorAction SilentlyContinue

    if ($Deep) {
        foreach ($path in @(
            (Join-Path $RepoRoot '.local'),
            (Join-Path $RepoRoot 'mingw'),
            (Join-Path $RepoRoot 'native')
        )) {
            if (Test-Path $path) {
                Remove-Item -LiteralPath $path -Recurse -Force -ErrorAction SilentlyContinue
            }
        }
    }

    Write-Host "Cleaned build artifacts in $RepoRoot"
}

function Resolve-ToolExecutable {
    param(
        [string[]]$Candidates,
        [string[]]$FallbackNames
    )

    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path $candidate)) {
            return $candidate
        }
    }

    foreach ($name in $FallbackNames) {
        $command = Get-Command $name -ErrorAction SilentlyContinue
        if ($command) {
            return $command.Source
        }
    }

    return $null
}

function Test-ToolchainRoot {
    param(
        [string]$Root
    )

    if (-not $Root -or -not (Test-Path $Root)) {
        return $false
    }

    $requiredPaths = @(
        (Join-Path $Root 'bin\x86_64-w64-mingw32-gcc.exe'),
        (Join-Path $Root 'x86_64-w64-mingw32\include\windows.h')
    )

    foreach ($path in $requiredPaths) {
        if (-not (Test-Path $path)) {
            return $false
        }
    }

    return $true
}

function Expand-LocalToolchain {
    param(
        [string]$ToolsDir,
        [string]$DestinationRoot
    )

    $archive = Get-ChildItem -Path $ToolsDir -Filter '*.zip' -File -ErrorAction SilentlyContinue |
        Sort-Object Length -Descending |
        Select-Object -First 1

    if (-not $archive) {
        return $null
    }

    $destination = Join-Path $DestinationRoot '.local\toolchains\mingw'
    New-Item -ItemType Directory -Path $destination -Force | Out-Null

    $toolchainRoot = Join-Path $destination 'mingw64'
    if (Test-Path $toolchainRoot) {
        Remove-Item -LiteralPath $toolchainRoot -Recurse -Force -ErrorAction SilentlyContinue
    }

    $tar = Resolve-ToolExecutable -Candidates @(
        (Join-Path $env:SystemRoot 'System32\tar.exe')
    ) -FallbackNames @('tar')

    Write-Host "Extracting bundled toolchain: $($archive.Name) -> $destination"

    if ($tar) {
        & $tar -xf $archive.FullName -C $destination
    } else {
        Expand-Archive -Path $archive.FullName -DestinationPath $destination -Force
    }

    return $destination
}

$cachedToolchainRoot = Join-Path $repoRoot '.local\toolchains\mingw\mingw64'

if ((Test-Path $cachedToolchainRoot) -and -not (Test-ToolchainRoot -Root $cachedToolchainRoot)) {
    Write-Host "Cached local toolchain is incomplete; re-extracting bundled toolchain."
    $null = Expand-LocalToolchain -ToolsDir (Join-Path $repoRoot 'tools') -DestinationRoot $repoRoot
}

$gcc = Resolve-ToolExecutable -Candidates @(
    (Join-Path $repoRoot '.local\toolchains\mingw\mingw64\bin\x86_64-w64-mingw32-gcc.exe'),
    (Join-Path $repoRoot '.local\toolchains\mingw\mingw64\bin\gcc.exe'),
    (Join-Path $repoRoot 'mingw\mingw64\bin\x86_64-w64-mingw32-gcc.exe'),
    (Join-Path $repoRoot 'mingw\mingw64\bin\gcc.exe')
) -FallbackNames @('x86_64-w64-mingw32-gcc', 'gcc')

$windres = Resolve-ToolExecutable -Candidates @(
    (Join-Path $repoRoot '.local\toolchains\mingw\mingw64\bin\windres.exe'),
    (Join-Path $repoRoot 'mingw\mingw64\bin\windres.exe')
) -FallbackNames @('windres', 'x86_64-w64-mingw32-windres')

if (-not $gcc -or -not $windres) {
    $expandedRoot = Expand-LocalToolchain -ToolsDir (Join-Path $repoRoot 'tools') -DestinationRoot $repoRoot
    if ($expandedRoot) {
        if (-not $gcc) {
            $gcc = Resolve-ToolExecutable -Candidates @(
                (Join-Path $expandedRoot 'mingw64\bin\x86_64-w64-mingw32-gcc.exe'),
                (Join-Path $expandedRoot 'mingw64\bin\gcc.exe')
            ) -FallbackNames @('x86_64-w64-mingw32-gcc', 'gcc')
        }

        if (-not $windres) {
            $windres = Resolve-ToolExecutable -Candidates @(
                (Join-Path $expandedRoot 'mingw64\bin\windres.exe')
            ) -FallbackNames @('windres', 'x86_64-w64-mingw32-windres')
        }
    }
}

if ((Test-Path $cachedToolchainRoot) -and -not (Test-ToolchainRoot -Root $cachedToolchainRoot)) {
    Write-Error 'Bundled MinGW toolchain is incomplete after extraction. Expected x86_64-w64-mingw32/include/windows.h under .local/toolchains/mingw/mingw64.'
    exit 2
}

if (-not $gcc) {
    Write-Error 'Unable to locate gcc. Provide it in PATH or place a WinLibs/MinGW ZIP under tools/.'
    exit 2
}

if (-not $windres) {
    Write-Error 'Unable to locate windres. Provide it in PATH or place a WinLibs/MinGW ZIP under tools/.'
    exit 2
}

if (-not (Test-Path $sourceFile)) {
    Write-Error "Source not found: $sourceFile"
    exit 2
}

if ($Clean) {
    Invoke-Cleanup -RepoRoot $repoRoot -Deep:$Deep
    exit 0
}

Invoke-Cleanup -RepoRoot $repoRoot

$toolBinDir = Split-Path -Parent $gcc
if ($toolBinDir -and -not (($env:PATH -split ';') -contains $toolBinDir)) {
    $env:PATH = "$toolBinDir;$env:PATH"
}

New-Item -ItemType Directory -Path $intermediateDir -Force | Out-Null
New-Item -ItemType Directory -Path $finalOutputDir -Force | Out-Null

Write-Host "Using windres: $windres"
Write-Host "Using gcc: $gcc"

if (Test-Path $resourceFile) {
    Write-Host "Compiling resources: $resourceFile -> $resourceObject"
    Push-Location $resourceDir
    try {
        & $windres 'BluePulse.rc' -O coff -o $resourceObject
    } finally {
        Pop-Location
    }
} else {
    $resourceObject = $null
    Write-Host 'No Windows resources found; compiling without resource object.'
}

Write-Host "Compiling C source: $sourceFile"
$gccArgs = @('-O2', '-s', '-o', $tempExe, $sourceFile)
if ($resourceObject) {
    $gccArgs += $resourceObject
}
$gccArgs += @('-mwindows', '-luser32', '-lshell32', '-ladvapi32', '-lshlwapi', '-lcomctl32', '-lgdi32')

& $gcc @gccArgs

if (-not (Test-Path $tempExe)) {
    Write-Error 'Build failed: output executable not found.'
    exit 1
}

Move-Item -Force $tempExe $finalExe

if ($resourceObject -and (Test-Path $resourceObject)) {
    Remove-Item -LiteralPath $resourceObject -Force -ErrorAction SilentlyContinue
}

Get-ChildItem -Path $intermediateDir -File -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -like '*.o' } |
    Remove-Item -Force -ErrorAction SilentlyContinue

if (((Get-ChildItem -Path $intermediateDir -Force -ErrorAction SilentlyContinue | Measure-Object).Count) -eq 0) {
    Remove-Item -LiteralPath $intermediateDir -Force -ErrorAction SilentlyContinue
}

$info = Get-Item $finalExe
Write-Host "Built: $($info.FullName) ($($info.Length) bytes)"