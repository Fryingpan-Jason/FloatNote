# FloatNote 2.0

English · [简体中文](README.md)

A portable native Windows desktop note with liquid glass, frosted glass and solid backgrounds. Text uses the native Windows editor, with selection, IME input and undo. Notes and preferences stay local.

## Download an app, not source code

**[Download the latest portable release](https://github.com/Fryingpan-Jason/FloatNote/releases/latest)**, extract the ZIP, and run `FloatNote.exe`. No compiler, development tools or additional runtime installation is required.

| File | Choose for |
| --- | --- |
| `FloatNote-2.0.0-windows-x64.zip` | Most Intel/AMD 64-bit PCs; recommended default |
| `FloatNote-2.0.0-windows-arm64.zip` | Windows on ARM, including Snapdragon X devices |
| `FloatNote-2.0.0-windows-x86.zip` | 32-bit environments or a required 32-bit process |

The **Source code** links on GitHub are for developers, not ready-to-run applications. Extract into a writable folder, rather than `Program Files`. Binaries are unsigned; Windows may show a SmartScreen prompt. Compare the downloaded ZIP with `SHA256SUMS.txt` on the release page if needed.

## What's new

- Liquid glass with continuous edge refraction, background-responsive reflections, edge softness and adjustable dispersion. Frosted and solid modes remain available.
- A compact top control expands into settings and close actions when approached, then retracts when you leave.
- Shrink the note vertically to tuck it into a small status bar; click to restore. Absorption, restoration and arrival bounce share continuous geometry.
- Pinning, mouse click-through, preset/custom background and text colors, font size and blur controls.
- Choose between hiding to the tray and exiting when closing, and optionally remember the choice.
- Single-instance behavior, startup integration and global recovery shortcuts.

## Use and upgrade

- Hover near the top hint to reveal settings; drag the bottom-right grip to resize.
- `Ctrl+Alt+E`: show and restore editing. `Ctrl+Alt+H`: toggle visibility. `Ctrl+Alt+P`: toggle mouse click-through.
- Double-click the tray icon or run the app again to recover your note.
- `Ctrl+wheel`, `Ctrl++` and `Ctrl+-` change font size.
- To upgrade from 1.x, exit the old app and replace `FloatNote.exe` in the same folder. **Keep the existing `data` folder**, and back it up first if you want an easy rollback.

Archives contain no note data or personal settings. The app creates `data/note.txt`, `settings.ini`, `material.ini` and `experience.ini` locally. It has no accounts, telemetry or network client. Screen pixels used for glass are processed locally through Windows APIs and the GPU, never uploaded.

## Platform and language limits

Liquid glass is intended for supported Windows 11 capture/composition environments. Accessibility, power, remote-session and API limitations can trigger fallback; see [compatibility](docs/COMPATIBILITY.md). The live liquid-glass note can be absent from screenshots or third-party remote streams; choose frosted glass when capture visibility matters. The top control is drawn independently as a recovery route.

**The new 2.0 settings panel, close prompt and material controls are currently in Chinese.** Existing tray and basic UI language options remain available. A complete English translation of the new panel is not claimed for this release.

## Build

With Visual Studio C++ tools and a Windows SDK, the default build produces the 2.0 desktop app:

```powershell
.\build.ps1 -Architecture x64 -OutputDirectory build\x64
.\build.ps1 -Test -OutputDirectory build\tests
.\scripts\package.ps1 -Architecture x64
```

See [development](docs/DEVELOPMENT.md). MIT licensed; adapted material code is credited in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
