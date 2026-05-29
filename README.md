# BluePulse

**Keep your Windows PC awake without admin permissions.**

[Download from releases](https://github.com/pedromartinezweb/BluePulse/releases) · [Source code](src/BluePulse/BluePulse.c) · [MIT License](LICENSE)

BluePulse is a small native Windows utility that helps keep the current session active and prevents the screen from going idle during inactivity.

It is built for a narrow purpose: keep the display awake while you are working around idle timeouts, without installing services, changing system policies, or simulating user input.

## Quick start

1. Download `BluePulse.exe` from the latest signed release.
2. Run the executable.
3. Enable the utility.
4. Choose the idle threshold in minutes.
5. Minimize it to the system tray when you want it out of the way.

No installer is required.

## Safety and transparency

BluePulse is intentionally simple:

- No administrator permissions.
- No background service installation.
- No keyboard or mouse input simulation.
- No network access.
- No analytics or user data collection.
- No corporate policy bypass.

If a company policy forces idle locking, BluePulse does not modify or disable that policy.

## Code signing

BluePulse uses the SignPath Foundation for code signing of public release builds.

The GitHub Actions workflow builds the executable on a GitHub-hosted Windows runner, submits the build artifact to SignPath when signing is configured, verifies the Authenticode signature, and publishes only the signed executable to GitHub Releases.

Until SignPath approval is complete, the workflow does not publish new unsigned release binaries.

Setup details are documented in [docs/SIGNPATH.md](docs/SIGNPATH.md).

## Build from source

Requirements:

- Windows x64.
- `gcc` and `windres` available in `PATH`, or a WinLibs/MinGW ZIP inside `tools/`.

Build from PowerShell:

```powershell
.\build.ps1
```

The generated executable is written to:

```text
dist\BluePulse.exe
```

Clean generated artifacts:

```powershell
.\build.ps1 -Clean
```

Run a deeper cleanup:

```powershell
.\build.ps1 -Clean -Deep
```

## Project status

BluePulse is early-stage open source software. The source code, build workflow, Windows resource metadata, and release process are public for review.

## License

BluePulse is open source software distributed under the [MIT License](LICENSE).
