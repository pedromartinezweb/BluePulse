# BluePulse

[Download BluePulse.exe from the latest release](https://github.com/pedromartinezweb/BluePulse/releases/latest/download/BluePulse.exe)

BluePulse is a Windows application that helps keep the current session active and prevents the screen from going idle because of inactivity.

BluePulse uses the SignPath Foundation for code signing of public release builds.

## What it does

BluePulse is designed to keep a Windows PC active during idle periods without administrator permissions.

It does not simulate user input, bypass corporate lock policies, install services, change system policies, or run hidden background behavior. It uses standard Windows power availability APIs from a small native executable.

## Build

Requirements:

- Windows x64.
- `gcc` and `windres` available in `PATH`, or a WinLibs/MinGW ZIP inside `tools/`.

Build from PowerShell:

```powershell
.\build.ps1
```

The generated executable is written to `dist\BluePulse.exe`.

## Windows signing

The executable includes product metadata in `assets\windows\BluePulse.rc`.

GitHub Actions builds the Windows executable on `windows-latest`. Releases from `main` require a valid Authenticode certificate and fail if signing is not configured. This avoids publishing unsigned release binaries.

Add these repository secrets before publishing a release:

- `WINDOWS_SIGNING_CERTIFICATE_BASE64`: Base64-encoded `.pfx` certificate contents.
- `WINDOWS_SIGNING_CERTIFICATE_PASSWORD`: certificate password.

Generate the Base64 value on Windows:

```powershell
[Convert]::ToBase64String([IO.File]::ReadAllBytes('certificate.pfx')) | Set-Clipboard
```

A real code-signing certificate is required to improve trust with Windows SmartScreen and antivirus vendors. Metadata alone is not a digital signature, and a self-signed certificate does not establish publisher reputation for public downloads.

## Clean

Remove generated build artifacts:

```powershell
.\build.ps1 -Clean
```

Run a deeper cleanup:

```powershell
.\build.ps1 -Clean -Deep
```

## License

BluePulse is open source software distributed under the MIT License.
See `LICENSE` for the full text.
