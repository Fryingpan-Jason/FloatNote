# FloatNote

English · [简体中文](README.md)

FloatNote is a lightweight, portable desktop memo for Windows. It is written in native C++/Win32 and has no network access, account, installer, or third-party runtime dependency.

![FloatNote glass interface on Windows 11](docs/images/floatnote.png)

## Features

- One persistent desktop note with automatic local saving beside the executable.
- Native rounded glass on Windows 11; standard background opacity from 0–100%.
- Glass and standard transparency are separate modes. Glass uses zero app tint, while standard mode remembers its previous opacity.
- Five presets, custom RGB colors, and automatic high-contrast text.
- Optional always-on-top, mouse click-through, tray menu, and startup shortcut.
- English and Simplified Chinese UI. Follow Windows by default or choose a language in settings or the tray menu.
- Per-monitor DPI, display hot-plug recovery, High Contrast, Energy Saver, and Remote Desktop fallbacks.

## Download and use

Download the matching archive from [Releases](https://github.com/Fryingpan-Jason/FloatNote/releases), extract it to a writable folder, and run `FloatNote.exe`. Avoid `Program Files`: notes and settings are stored beside the executable by default.

| Archive | Devices |
| --- | --- |
| `windows-x64` | Most 64-bit Intel and AMD Windows PCs |
| `windows-x86` | 32-bit Windows 10 or environments that require a 32-bit app |
| `windows-arm64` | Windows 11 ARM devices, including Snapdragon X PCs |

Main controls:

- Click text to edit, drag text to select, and drag blank editor space or the top pill to move the note.
- Resize from the bottom-right grip.
- Use `Ctrl + wheel`, `Ctrl + +`, or `Ctrl + -` to change text size; `Ctrl+0` resets it to 13 pt.
- `Ctrl+Alt+E` shows the note and restores editing, `Ctrl+Alt+H` hides it, and `Ctrl+Alt+P` toggles mouse click-through.
- Double-click the tray icon or launch FloatNote again to restore editing.

## Windows version differences

| Environment | Behavior |
| --- | --- |
| Windows 11 | Native rounded glass and standard transparency. Glass requires Windows transparency effects. |
| Windows 10 1809–22H2 | Standard transparency is supported. Glass falls back; native rounded glass is not promised. |
| Earlier than Windows 10 | Unsupported. |
| High Contrast | Uses opaque system colors and system text colors. |
| Energy Saver / Remote Desktop | Pauses glass and uses the standard background to reduce cost or avoid compositor differences. |

See [Compatibility](docs/COMPATIBILITY.md) for tested boundaries and caveats.

## Data and privacy

FloatNote has no network access or telemetry. `data/note.txt` is UTF-8 text, and `data/settings.ini` stores window and appearance preferences. Writes use a temporary file followed by an atomic replacement. Read and save failures preserve the original note instead of replacing it with an error message.

## Build

Visual Studio C++ tools and the Windows 10/11 SDK are required:

```powershell
.\build.ps1 -Architecture x64
.\build.ps1 -Architecture x86 -OutputDirectory build\x86
.\build.ps1 -Test -OutputDirectory build\tests
```

ARM64 can be built with Visual Studio ARM64 C++ tools or on GitHub's Windows 11 ARM runner. See [Development](docs/DEVELOPMENT.md) for the build, test, and release flow.

## Contributing

Issues and pull requests are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md) first. FloatNote is available under the [MIT License](LICENSE).
