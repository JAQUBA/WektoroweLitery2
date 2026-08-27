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
- simple block-based VL2 layout format with legacy TXT import

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
- confirmation before closing when the layout has unsaved changes

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
2. Choose a tool preset; this sets the cutter diameter, stepover, and feed rates.
3. Choose a font.
4. Adjust material thickness, engraving depth, and safe Z for the current operation.
5. Inspect the preview.
6. Export G-Code.

When closing the application after editing a layout, choose `Save`, `Don't Save`,
or `Cancel` in the confirmation dialog. The same check is used by `File > Exit`
and the window close button.

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

New layouts use the readable block-based VL2 format. It uses sections and
`key=value` properties, so repeated plate parameters are written once in a
template. The file is UTF-8 and does not depend on indentation or an external
parser library. A compact row writes one complete plate on one line:

```text
[row]
pump: POMPA 1
pump: POMPA 2
control: STEROWANIE | ZDALNE
```

The `|` separator creates multiple text lines. Per-plate overrides can be
written in the template selector, for example
`pump(size=200x40,text=12,condensation=150): FALOWNIK 1`.
Set `frame=false` in a template or selector for text without a visible frame.
The `offset=x,y` value moves the text relative to the plate center and works
for both framed and unframed plates. In a template, the short key `text=4`
means text height; the row text remains after the colon.
In VL2, `thickness` is the real stroke width in **mm** (unlike the legacy TXT
format below, where `thickness` is an abstract stroke-width unit); the parser
converts the mm value internally (`units = mm * 100 / text_height_mm`).

Layouts without named templates can define shared parameters in a `[default]`
section and omit the selector:

```text
[default]
type=plate
size=100x30
text_height=8
thickness=0.4
frame=true

[row]
: POMPA 1
: POMPA 2
```

A single plate can define its parameters inline with `plate(...)`:

```text
[row]
plate(size=80x20,text=6,frame=true): STEROWANIE
```

Inline values inherit from `[default]` when that section exists. A selector
such as `pump: POMPA 1` still refers to the named `[template pump]`. Without
`[default]`, an empty selector is reported as a parse error.

Example:

```text
version=1

[template pump]
type=plate
width=100
height=30
offset_x=0
offset_y=4
text_height=8
condensation=100
thickness=0.4
frame=true

[row]
use=pump
text=POMPA 1
use=pump
text=POMPA 2

[row]
use=pump
text=POMPA 3
```

Use `type=multiline` with `line1=...`, `line2=...` and so on for a plate
that contains multiple text lines. A template can define line-specific
properties such as `line1_offset_y` or `line2_condensation`. Use
`type=empty` for a frame without text.

The older semicolon-separated TXT format remains supported for existing
files and can be converted manually to VL2.

Commands:

| Command | Description |
|---------|-------------|
| `l` | start a new row |
| `t` | text-only nameplate |
| `tw` | text with frame |
| `w` | frame only |

Legacy TXT text rows use:

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
- redundant rapid moves are skipped by tracking the current machine position
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
- spindle speed (RPM)

Tool selection does not change the operation's engraving depth or safe Z. These
values depend on the material and machining operation, so they are set in the
toolbar and remain unchanged when switching tools. Legacy `cutDepth` and
`safeHeight` entries in `tools.ini` are retained for compatibility but are not
applied automatically.

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
