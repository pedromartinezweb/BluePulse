# Contributing

## Development flow

1. Fork or branch from `main`.
2. Keep changes focused and reviewable.
3. Run `./build.ps1` before opening a pull request.
4. Update documentation when behavior, layout or build flow changes.

## Repository conventions

- Source code lives in `src/`.
- Windows resources live in `assets/windows/`.
- Automation lives in `scripts/`.
- Build output belongs in `dist/` and must not be committed.

## Pull request checklist

- Build passes locally.
- No generated binaries or toolchains are committed.
- README and docs remain accurate.
- The app still avoids synthetic user input and policy bypass techniques.