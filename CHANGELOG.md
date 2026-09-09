# Changelog

All notable changes are documented here. FloatNote follows [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- Optional window shadow while retaining rounded glass.
- Custom text colors with a setting to return to automatic theme-based text color.

### Changed

- Glass and standard mode share adjustable 0–100% background opacity without fading text.

### Fixed

- Remove light fringes around text over dark backgrounds using independent grayscale coverage masks, preserving native selection colors.

## [1.0.0] - 2026-09-09

### Added

- Portable single-note Windows application with atomic UTF-8 saving.
- Native rounded glass on supported Windows 11 systems and 0–100% standard background opacity.
- Always-on-top, mouse click-through, tray controls, global shortcuts, and per-user startup shortcut.
- Five theme presets, custom RGB colors, and readable foreground selection.
- English and Simplified Chinese UI with automatic Windows-language detection and a manual selector.
- x64, x86, and ARM64 build targets.

### Reliability

- Protect unreadable, unsupported, oversized, and locked note files from accidental replacement.
- Recover off-screen windows after monitor changes and rescale for per-monitor DPI.
- Fall back from glass in High Contrast, Energy Saver, Remote Desktop, disabled transparency, or unsupported compositor environments.
- Avoid idle repaint loops and reuse the layered drawing buffer.
