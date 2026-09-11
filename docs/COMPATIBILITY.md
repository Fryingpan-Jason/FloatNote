# Compatibility — FloatNote 2.0

The application targets Windows 10 1809 and later APIs; the liquid-glass desktop experience is primarily validated on Windows 11 x64.

| Environment | Expected behavior | Verification scope |
| --- | --- | --- |
| Supported Windows 11 capture/composition environment | Liquid glass, system frosted glass or solid mode | Interactive visual acceptance on Windows 11 x64; headless checks are separate |
| Windows 10 | Basic editing and standard-background fallback when host backdrop is unavailable | A complete 2.0 visual matrix on Windows 10 has not been run |
| Windows 11 ARM | Native ARM64 release package | Native ARM CI compilation/tests, not a full GPU visual certification |
| Windows before 10 1809 | Unsupported | No compatibility claim |

## Fallback and capture

High Contrast uses system colors. Energy Saver, Remote Desktop, disabled transparency, unavailable composition or capture APIs can disable advanced material rendering. The native editor and recovery controls remain the intended fallback path.

The liquid renderer captures the monitor locally and excludes the note itself to prevent feedback. This exclusion can also hide the note from screenshots and third-party remote streams. Switch to system frosted glass when capture visibility is needed. The top control/settings windows are independent and not excluded, but no guarantee is made for every remote product. Screens are not uploaded or recorded by the app.

## Input and language

Per-monitor DPI, native Unicode editing, selection, IME and undo remain supported. Specific IMEs and all multi-monitor layouts have not been individually certified. Global shortcuts may conflict with another application; tray recovery and a second launch remain alternatives.

The new control panel, close dialog and advanced material controls currently use Chinese. The original tray/basic interface retains its language selector. Do not interpret the bilingual documentation as a claim that every 2.0 control is translated.

## Release checks

Local x64/x86 compilation and the existing native regression are complemented by geometry, motion, pointer-area and material-math checks. GitHub CI builds the three architectures, runs native/glass tests, then validates portable archive version, CPU architecture and the fixed file list. These checks do not certify all GPU drivers, screen-capture policies or remote graphics stacks.
