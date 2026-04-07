<div align="center">

# Wektorowe Litery 2

**Native Windows generator for CNC-milled and laser-engraved vector nameplates.**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform: Windows 10+](https://img.shields.io/badge/Platform-Windows%2010+-0078d4.svg)](https://www.microsoft.com/windows)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Build: PlatformIO](https://img.shields.io/badge/Build-PlatformIO-orange.svg)](https://platformio.org/)

Desktop tool for creating vector-font labels and front-panel markings from plain-text layout files, with real-time preview and G-Code export for CNC milling or laser workflows.

</div>

## What It Does

Wektorowe Litery 2 takes compact text layout definitions and turns them into production-friendly vector output:

- parse a simple row-based label layout format
- load LibreCAD LFF vector fonts
- generate stroke envelopes for selected tool width / stepover
- preview the result in a native Windows canvas
- export optimized GRBL-style G-Code with optional arc fitting

It is built for practical workshop use: quick editing, instant visual feedback, repeatable tool presets, and minimal setup cost.

## Highlights

### Layout and Editing

- built-in RichEdit layout editor
- auto-render after edits with debounce
- error highlighting for invalid lines
- draggable editor/canvas splitter
- simple semicolon-based layout format

### Font Engine

- LibreCAD Font Format (`.lff`) support
- 26 bundled vector fonts
- Unicode text handling with Polish-friendly workflows
- glyph scaling, condensation, centering, and multi-pass envelope generation

### CAM Output

- milling and laser modes
- G2/G3 arc fitting for reduced file size and smoother motion
- collinear segment reduction
- feed-rate handling for plunge and XY moves
- deterministic GRBL-oriented output

### Operator Experience

- dark themed native Windows UI
- real-time preview with zoom, pan, and reset
- tool preset popup and management dialog
- configurable machine workspace bounds
- detached log window

## Why It Is Useful

The project sits between full CAD/CAM suites and ad-hoc label scripts.

It is useful when you want:

- a repeatable workflow for industrial labels and front panels
- vector fonts instead of raster engraving
- editable text-file layouts under version control
- a small native Windows tool instead of a large design suite

## Build

### Requirements

- [PlatformIO](https://platformio.org/)
- Git
- Windows 10+

### Build Command

```bash
git clone https://github.com/JAQUBA/WektoroweLitery2.git
cd WektoroweLitery2
pio run -e windows_x86
```

Output binary:

```text
.pio/build/windows_x86/WektoroweLitery2.exe
```

### Dependencies

The checked-in `platformio.ini` uses GitHub dependencies for shared libraries:

```ini
lib_deps =
    https://github.com/JAQUBA/JQB_WindowsLib.git
    https://github.com/JAQUBA/JQB_CAMCommon.git
lib_extra_dirs =
    lib/Clipper2/CPP
```

Dependency roles:

| Dependency | Role |
|------------|------|
| [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) | native UI, canvas, widgets, dialogs |
| [JQB_CAMCommon](https://github.com/JAQUBA/JQB_CAMCommon) | shared CAM helpers, G-code formatting, geometry/math support |
| [Clipper2](https://github.com/AngusJohnson/Clipper2) | geometry backend used through shared CAM utilities |

Font files are copied into the output directory automatically by `scripts/copy_fonts.py`.

### Local Multi-Repo Development

For local sibling-repo work:

```ini
lib_deps =
    ../JQB_WindowsLib
    ../JQB_CAMCommon
```

## Quick Start

1. Open or create a layout file.
2. Choose a tool preset.
3. Choose a font.
4. Adjust material, depth, and safe Z.
5. Inspect the preview.
6. Export G-Code.

## User Workflow

### Typical Session

1. **Edit the layout** in the built-in editor.
2. **Select a preset** matching your cutter or laser workflow.
3. **Switch fonts** until the visual style fits the plate.
4. **Preview the geometry** in the right-side canvas.
5. **Adjust workspace and parameters** if needed.
6. **Export G-Code** to the target output file.

### Menu Structure

```text
File      → New | Open... | Save | Save As... | Export G-Code | Exit
View      → Show grid | Reset view | Log window
Settings  → Tool presets... | Machine workspace size...
Help      → About...
```

### In-App About Dialog

`Help → About...` lists the libraries used by the binary together with their licenses.

## Layout Format

The input format is intentionally compact and version-control friendly.

Commands:

| Command | Description |
|---------|-------------|
| `l` | start a new row |
| `t` | text-only nameplate |
| `tw` | text with frame |
| `w` | frame only |

Text rows use:

```text
t;width;height;dx;dy;textH;condensation;thickness;text
tw;width;height;dx;dy;textH;condensation;thickness;text
```

Example:

```text
tw; 100; 30; 0; 4; 8; 100; 12;POMPA 1;
tw; 100; 30; 0; 4; 8; 100; 12;POMPA 2;
l
tw; 100; 30; 0; 4; 8; 100; 12;WENTYLATOR 1;
```

## Fonts

The application uses LibreCAD Font Format (`.lff`) files bundled in `resources/fonts/`.

Included families cover:

- standard technical lettering
- roman and italic families
- gothic variants
- script/cursive styles
- greek and cyrillic variants

This makes the tool useful both for industrial control labels and more decorative engraving workflows.

## Architecture Overview

```text
src/
├── main.cpp
├── AppState.h / AppState.cpp
├── AppUI.h / AppUI.cpp
├── MenuHandler.h / MenuHandler.cpp
├── CanvasWindow.h / CanvasWindow.cpp
├── Document/
├── Font/
└── GCode/
```

Core flow:

1. parse text layout into document objects
2. load glyphs from LFF font files
3. generate vector envelopes and tool-width paths
4. preview vectors and frames in the canvas
5. export optimized G-Code for milling or laser mode

## G-Code Model

The exporter uses a GRBL-oriented motion model.

Key traits:

- `G21 G90 G17 G94 G54 G91.1` preamble
- arc fitting for eligible segments
- Z-based milling mode
- `M03` / `M05` switching for laser mode
- return-to-origin style epilog

Example:

```gcode
G21 G90 G17 G94 G54
G91.1
G00 Z6.50
G00 X0.000 Y0.000
G01 Z1.30 F100
G01 X25.000 Y0.000 F300
G02 X30.000 Y5.000 I0.000 J5.000
M30
```

## Configuration and Presets

Persistent configuration lives in:

- `config.ini` for application state
- `tools.ini` for tool presets

Presets bundle values such as:

- tool diameter
- stepover
- cut depth
- safe height
- XY feed
- Z feed

This keeps repeat jobs predictable and makes switching between CNC and laser setups much faster.

## Examples

The `examples/` folder contains ready-to-open sample layouts for industrial and workshop-style labels.

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Language | C++17 |
| Build system | PlatformIO native via [JQB_MinGW](https://github.com/JAQUBA/JQB_MinGW) |
| UI framework | [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) |
| Shared CAM code | [JQB_CAMCommon](https://github.com/JAQUBA/JQB_CAMCommon) |
| Geometry backend | [Clipper2](https://github.com/AngusJohnson/Clipper2) |
| Rendering | WinAPI GDI |
| Font format | LibreCAD Font Format |

## Open Source and Licensing

The source code in this repository is licensed under the [MIT License](LICENSE).

The binaries also depend on third-party libraries with separate licenses. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Current dependency licensing model:

- JQB_WindowsLib — LGPL-3.0-or-later
- JQB_CAMCommon — LGPL-3.0-or-later
- Clipper2 — Boost Software License 1.0

Because the current build uses static linking, compliant binary distribution should also include required third-party license texts and a practical relinking path for the LGPL libraries.

## Contributing

Contributions are welcome.

For substantial changes, keep the public project docs aligned:

- [README.md](README.md)
- [.github/copilot-instructions.md](.github/copilot-instructions.md)
- [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)

## Acknowledgments

- the original [WektoroweLitery](https://github.com/JAQUBA/WektoroweLitery) project
- [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib)
- [JQB_CAMCommon](https://github.com/JAQUBA/JQB_CAMCommon)
- [Clipper2](https://github.com/AngusJohnson/Clipper2)
