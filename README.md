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

GitHub Actions builds the Windows executable on `windows-latest`. Public release builds are prepared for SignPath Foundation code signing.

When SignPath approves the project, configure these GitHub repository variables:

- `SIGNPATH_ORGANIZATION_ID`
- `SIGNPATH_PROJECT_SLUG`
- `SIGNPATH_SIGNING_POLICY_SLUG`

Configure this GitHub repository secret:

- `SIGNPATH_API_TOKEN`

The workflow always uploads the unsigned executable as a short-lived workflow artifact. When SignPath is configured, it submits that artifact for signing, waits for the signed executable, verifies the Authenticode signature, and publishes the signed file to GitHub Releases.

If SignPath is not configured yet, releases are not published. This avoids distributing unsigned release binaries.

See `docs/SIGNPATH.md` for the full setup checklist.

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
