# Compatibility

FloatNote targets Windows 10 version 1809 (build 17763) and later.

| System | CPU builds | Standard transparency | Rounded glass | Notes |
| --- | --- | --- | --- | --- |
| Windows 11 21H2 and later | x64, x86, ARM64 | Yes | Yes, when supported by the compositor | Enable **Settings → Personalization → Colors → Transparency effects**. |
| Windows 10 1809–22H2 | x64, x86 | Yes | Fallback only | The Glass preference remains saved for use if the data folder later moves to Windows 11. |
| Windows 10 ARM64 | ARM64 | Not release-tested | Not promised | The binary may run, but the published ARM64 package is tested by CI on Windows 11 ARM. |
| Windows 8.1 / 8 / 7 | — | No | No | Unsupported. |

## Automatic fallbacks

- High Contrast replaces transparency and custom colors with opaque system colors.
- Energy Saver pauses glass.
- Remote Desktop uses the standard background.
- Disabling Windows transparency effects disables glass.
- If the Windows Composition APIs required for rounded glass are unavailable, FloatNote keeps editing and uses standard transparency.

## Display and input

- FloatNote is per-monitor DPI aware and rescales when moved between displays.
- Stored window bounds are constrained to the nearest current work area after startup or a display change.
- Native EDIT behavior provides IME composition, Unicode text, selection, and undo. Microsoft Pinyin and other specific IMEs have not all been tested individually.
- Global hotkeys can be unavailable when another app has registered the same combination. The tray icon and launching FloatNote again remain recovery paths.

## Tested evidence for v1.0.0

- Windows 11 build 26200, 200% display scaling, x64 binary.
- x64 and x86 native regression suites on an x64 host.
- Four-corner compositor check for rounded glass, real backdrop color sampling, blur, click-through, and return to standard transparency.
- CI builds x64 and x86 on Windows Server x64 and ARM64 on the native Windows 11 ARM runner.

The CI build proves compilation and headless behavior on its runner images. It does not certify every GPU driver, OEM shell customization, multi-monitor topology, or remote graphics stack.
