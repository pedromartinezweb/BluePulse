param(
    [string]$FilePath = 'dist\BluePulse.exe',
    [string]$CertificateBase64 = $env:WINDOWS_SIGNING_CERTIFICATE_BASE64,
    [string]$CertificatePassword = $env:WINDOWS_SIGNING_CERTIFICATE_PASSWORD,
    [string]$TimestampServer = 'http://timestamp.digicert.com',
    [switch]$Optional
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

if (-not $CertificateBase64 -or -not $CertificatePassword) {
    if ($Optional) {
        Write-Host 'Signing skipped because certificate secrets are not configured.'
        exit 0
    }

    Write-Error 'Set WINDOWS_SIGNING_CERTIFICATE_BASE64 and WINDOWS_SIGNING_CERTIFICATE_PASSWORD to sign the executable.'
    exit 2
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

$certificatePath = Join-Path ([System.IO.Path]::GetTempPath()) 'bluepulse-signing-cert.pfx'

try {
    [System.IO.File]::WriteAllBytes($certificatePath, [System.Convert]::FromBase64String($CertificateBase64))

    & $signTool sign /fd SHA256 /f $certificatePath /p $CertificatePassword /tr $TimestampServer /td SHA256 $target
    & $signTool verify /pa /v $target
} finally {
    if (Test-Path $certificatePath) {
        Remove-Item -LiteralPath $certificatePath -Force -ErrorAction SilentlyContinue
    }
}
