# Microsoft Store submission

This document contains copy-ready Microsoft Store listing information for BluePulse.

## Recommended publishing path

Use an MSIX package for Microsoft Store distribution.

Microsoft Store distribution signs MSIX packages during submission, so the Store package does not require the SignPath certificate used for GitHub releases.

## Partner Center basics

- Product name: `BluePulse`
- App type: `Windows app`
- Package type: `MSIX`
- Price: `Free`
- Category: `Utilities & tools`
- Subcategory: `Utilities`
- Markets: `All available markets`
- Visibility: `Public`
- Target devices: `PC`
- Minimum OS: `Windows 10 version 1809`
- Recommended OS: `Windows 11`
- Architecture: `x64`

## Store listing

### Short description

```text
Keep your Windows PC awake without admin permissions.
```

### Description

```text
BluePulse is a small open source Windows utility that helps keep the current session active and prevents the screen from going idle during inactivity.

It is designed for a narrow and transparent purpose: keeping the display awake while you are working around idle timeouts. BluePulse does not install background services, change system policies, collect user data, access the network, or simulate keyboard or mouse input.

Use it when you need a simple tray utility that can keep the screen awake without administrator permissions.
```

### Key features

```text
Keep the screen awake during idle periods
Configurable idle threshold in minutes
Runs without administrator permissions
Minimizes to the Windows system tray
No installer service, analytics, or network access
Open source under the MIT License
```

### Search terms

```text
keep awake
prevent sleep
screen awake
idle
system tray
utility
windows
```

### Support URL

```text
https://github.com/pedromartinezweb/BluePulse/issues
```

### Website URL

```text
https://github.com/pedromartinezweb/BluePulse
```

### Privacy policy URL

Use this if you want to keep your current website privacy policy:

```text
https://pedromartinezweb.com/es/privacy
```

Alternative project-specific option:

```text
https://github.com/pedromartinezweb/BluePulse/blob/main/PRIVACY.md
```

## Privacy answers

Use these answers for Partner Center privacy and data questions:

- Collects personal information: `No`
- Uses analytics: `No`
- Uses advertising ID: `No`
- Sends telemetry: `No`
- Requires account sign-in: `No`
- Requires internet access: `No`
- Shares data with third parties: `No`
- Uses location: `No`
- Uses camera, microphone, contacts, calendar, documents, pictures, or videos: `No`

## Age rating notes

Recommended answers:

- Violence: `None`
- Fear: `None`
- Sexual content: `None`
- Language: `None`
- Controlled substances: `None`
- Gambling: `None`
- User-generated content: `No`
- Online interaction: `No`
- Location sharing: `No`
- Personal data sharing: `No`

Expected rating: suitable for all ages.

## Certification notes

Paste this in any certification notes field:

```text
BluePulse is a small native Windows utility that keeps the display awake using standard Windows power availability APIs. It does not simulate keyboard or mouse input, install background services, change system policies, bypass corporate lock policies, collect user data, or access the network.
```

## Screenshots to prepare

Required screenshots should be captured on Windows after launching the app:

1. Main BluePulse window with the utility disabled.
2. Main BluePulse window with the utility enabled.
3. Idle threshold control visible.
4. System tray menu open.

Recommended screenshot format:

- PNG
- 16:9 aspect ratio
- 1920x1080 or 1366x768
- No personal data visible

## Store image assets

Initial logo assets are stored in `assets/store/`:

- `StoreLogo.png`
- `Square44x44Logo.png`
- `Square150x150Logo.png`
- `Square300x300Logo.png`
- `Square310x310Logo.png`

These were generated from `assets/windows/BluePulse.ico` and should be reviewed before final submission.

## Required technical work before submission

The repo still needs an MSIX packaging step before the Microsoft Store submission can be completed.

Recommended output:

```text
dist/store/BluePulse.msix
```

Recommended future workflow:

1. Build `dist\BluePulse.exe`.
2. Package it into `dist\store\BluePulse.msix`.
3. Upload the MSIX to Partner Center.
4. Let Microsoft Store sign the submitted package.

## Account-dependent fields

These must match your Partner Center account:

- Publisher display name
- Seller contact email
- Tax profile
- Payout profile, if you ever publish paid apps
- Account verification details
