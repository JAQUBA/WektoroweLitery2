# Wektorowe Litery 2

Native Windows desktop application for generating **vector font nameplates** for CNC milling and laser engraving. Reads layout definition files, renders real-time preview, and exports optimized **G-Code with G2/G3 arc fitting**.

Port of the original C# WPF application [WektoroweLitery](https://github.com/JAQUBA/WektoroweLitery) to C++17 using [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib).

---

## Features

### Font & Text

- **LibreCAD Font Format (LFF)** — 26 vector font files included (standard, cursive, roman, gothic, greek, cyrillic, etc.)
- **Font selector** — switch fonts from toolbar popup
- **Tool envelope algorithm** — angle computation, offset generation, serif endcaps, multi-pass thickness
- **Unicode support** — UTF-8 input with Windows-1250 fallback, Polish diacritics

### Layout & Editing

- **Built-in layout editor** — RichEdit with syntax highlighting and auto-render on changes (300 ms debounce)
- **Layout file format** — semicolon-separated commands: text (`t`), text+frame (`tw`), frame (`w`), row break (`l`)
- **Resizable splitter** — drag to adjust editor / canvas proportions
- **Error highlighting** — red underline on invalid layout lines

### CAM Processing

- **G2/G3 arc fitting** — greedy arc optimization reduces G-Code size and improves CNC motion smoothness
- **Collinear reduction** — merges co-linear G01 segments
- **Milling mode** — Z-based depth control with feed rates from tool preset
- **Laser mode** — M03/M05 spindle control, no Z moves

### G-Code Output

- **GRBL-compatible** — `G21 G90 G17 G94 G54 G91.1`, `G00`/`G01`/`G02`/`G03` with `I`,`J` center offsets
- **Dynamic feed rates** — `F{feedZ}` on Z plunge, `F{feedXY}` on first XY move per segment
- **Z convention** — bottom of material = Z0, surface = Z(materialThickness), text = Z(surface − textDepth), frame = Z0, rapid = Z(surface + safeHeight)

### User Interface

- **Real-time GDI preview** — pan (left-drag), zoom (scroll), reset (double-click)
- **Tool presets** — popup selector + management dialog, persisted in `tools.ini`
- **Configurable workspace** — machine dimensions affect grid extent
- **Dark theme** — Tokyo Night color palette
- **Log window** — operation feedback and optimization statistics

---

## Build

### Requirements

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- [JQB_MinGW](https://github.com/JAQUBA/JQB_MinGW) platform (downloaded automatically)
- [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) library (downloaded automatically)
- [JQB_CAMCommon](https://github.com/JAQUBA/JQB_CAMCommon) library (downloaded automatically)
- Git (used by pre-build script to auto-download Clipper2 if missing)

No manual compiler or library installation needed — PlatformIO resolves everything.

### Compile

```bash
pio run -e windows_x86
```

Output binary: `.pio/build/windows_x86/WektoroweLitery2.exe`

Font files are copied to the output directory automatically by `scripts/copy_fonts.py`.

Clipper2 is downloaded automatically on build (if missing) by JQB_CAMCommon library manifest (`library.json` → `build.extraScript`).

The checked-in `platformio.ini` uses GitHub dependencies for both shared libraries:

```ini
lib_deps =
    https://github.com/JAQUBA/JQB_WindowsLib.git
    https://github.com/JAQUBA/JQB_CAMCommon.git
```

### `platformio.ini`

```ini
[env:windows_x86]
platform = https://github.com/JAQUBA/JQB_MinGW.git
extra_scripts =
    post:scripts/copy_fonts.py
lib_deps =
    https://github.com/JAQUBA/JQB_WindowsLib.git
    https://github.com/JAQUBA/JQB_CAMCommon.git
lib_extra_dirs =
    lib/Clipper2/CPP
```

For local multi-repo development, these Git dependencies can be replaced with `../JQB_WindowsLib` and `../JQB_CAMCommon`.

C++17, UNICODE, static linking, and library flags are added automatically by `compile_resources.py`.

---

## Usage

1. **Create or open a layout file** — `File → Open...` or type directly in the built-in editor
2. **Select tool preset** — click the tool selector in toolbar or `Settings → Tool presets...`
3. **Select font** — click the font selector in toolbar
4. **Set export parameters** — Material thickness, Text depth, Safe Z (auto-populated from tool preset)
5. **Preview** — layout renders automatically on each edit; canvas shows vectors + frames + workspace bounds
6. **Choose output file** — enter path in the Output field or click `...` to browse
7. **Export** — click `Export GCode` button or `File → Export G-Code`

### Menu Bar

```
File      → New | Open... | Save | Save As... | Export G-Code | Exit
View      → Show grid | Reset view | Log window
Settings  → Tool presets... | Machine workspace size...
Help      → About...
```

The `Help → About...` dialog inside the binary lists the bundled/open-source libraries together with their licenses.

### Layout File Format

Semicolon-separated commands. Lines starting with `#` are comments.

| Command | Parameters | Description |
|---------|-----------|-------------|
| `l` | — | New row (line break) |
| `t` | `width;height;dx;dy;textH;condensation;thickness;text` | Text-only nameplate |
| `tw` | `width;height;dx;dy;textH;condensation;thickness;text` | Text with frame |
| `w` | `width;height` | Frame only |

**Parameters**: width/height in mm, dx/dy = text offset, textH = font size in mm, condensation = horizontal scale (100 = normal), thickness = stroke width units.

**Example** (`examples/example.txt`):
```
tw; 100; 30; 0; 4; 8; 100; 12;POMPA 1;
tw; 100; 30; 0; 4; 8; 100; 12;POMPA 2;
tw; 100; 30; 0; 4; 8; 100; 12;POMPA 3;
tw; 100; 30; 0; 4; 8; 100; 12;POMPA 4;
l
tw; 100; 30; 0; 4; 8; 100; 12;WENTYLATOR 1;
tw; 100; 30; 0; 4; 8; 100; 12;WENTYLATOR 2;
```

---

## Architecture

```
WektoroweLitery2/
├── src/
│   ├── main.cpp              # Entry point (setup/loop), window + canvas init
│   ├── AppState.h / .cpp     # Global state, settings, tool presets, shared actions
│   ├── AppUI.h / .cpp        # Toolbar, editor, splitter, font/tool popups
│   ├── MenuHandler.h / .cpp  # Menu bar creation and command routing
│   ├── CanvasWindow.h / .cpp # VectorCanvas — GDI rendering of documents
│   ├── theme.h               # Dark theme color palette (extends ThemeTokyoNight)
│   ├── Document/             # Document model and parsing
│   │   ├── Document.h            # Document model (settings + rows)
│   │   ├── TableRow.h            # Row of nameplates
│   │   ├── Nameplate.h / .cpp    # Single nameplate — text layout within frame
│   │   └── DocumentParser.h / .cpp # Layout file (.TXT) parser
│   ├── Font/                 # Vector font engine
│   │   ├── VectorPoint.h        # Point struct with envelope metadata
│   │   ├── VectorLetterEngine.h / .cpp # Envelope generation + toolpath
│   │   └── LffFont.h / .cpp     # LibreCAD Font Format parser
│   └── GCode/                # G-Code generation
│       └── GCodeEngine.h / .cpp  # GRBL-compatible export with G2/G3 arc fitting
├── resources/
│   ├── app.manifest          # Windows Common Controls v6 manifest
│   ├── resources.rc          # Windows resource file (icon + manifest + version)
│   ├── icon.ico              # Application icon
│   └── fonts/                # LFF vector font files (26 fonts)
├── examples/                 # Example layout files (.TXT)
├── scripts/
│   └── copy_fonts.py         # Post-build: copies fonts to output directory
└── platformio.ini            # PlatformIO build config
```

### Module Responsibilities

| Module | Responsibility |
|--------|---------------|
| **main.cpp** | `setup()` / `loop()` entry points, window creation, canvas init, close handler |
| **AppState** | Global state, `ToolPreset` struct, settings load/save (config.ini + tools.ini), file dialogs, shared actions (`doRenderPreview`, `doExportGCode`, `doShowToolPresets`, `doShowWorkspaceSettings`, `doRelayout`), font loading |
| **AppUI** | Toolbar creation (Export button, tool/font selectors, parameter fields), RichEdit editor with auto-render, resizable splitter, error highlighting |
| **MenuHandler** | Menu bar (File, View, Settings, Help) with lambda routing |
| **VectorCanvas** | Subclass of JQB_WindowsLib `CanvasWindow` — renders workspace bounds, nameplates (frames + letter vectors), coordinate transforms |
| **Document/Document** | Document-level parameters (diameter, stepover, materialThickness, textDepth, safeHeight, feedXY, feedZ, laserMode) + collection of TableRows |
| **Document/Nameplate** | Text layout: loads glyphs from LFF font, applies scaling/condensation, centers within frame, generates toolpaths |
| **Document/DocumentParser** | Parses semicolon-separated layout files into Document model with error reporting |
| **Font/LffFont** | LFF font file parser: glyph loading, reference resolution, arc tessellation, font enumeration |
| **Font/VectorLetterEngine** | Core vector engine: angle computation, envelope offset generation, serif handling, multi-pass thickness |
| **GCode/GCodeEngine** | G-Code output: GRBL preamble, G00/G01/G02/G03, greedy arc fitting, collinear reduction, feed rate management, milling/laser modes |

---

## G-Code Format

### Preamble

```gcode
G21 G90 G17 G94 G54
G91.1
G00 Z6.50
G00 X0.000 Y0.000
```

### Z Coordinate Convention

| State | Z value |
|-------|---------|
| Bottom of material | 0.0 |
| Surface | materialThickness |
| Text engraving | materialThickness − textDepth |
| Frame cutting | 0.0 |
| Rapid travel | materialThickness + safeHeight |

### Optimization Pipeline

1. Remove duplicate points (< 0.003 mm)
2. Compute turn angles at interior points
3. Split at sharp corners (> ~25°)
4. Greedy arc fit — extend while within 0.01 mm tolerance
5. Collinear reduction — merge segments within 0.005 mm
6. Emit `G01` for lines, `G02`/`G03` for arcs with `I`,`J` center offsets

### Epilog

```gcode
G00 Z6.50
G00 X0.000 Y0.000
M30
```

---

## Configuration

### Application Settings (`config.ini`)

| Key | Description | Default |
|-----|-------------|---------|
| `last_input_file` | Last opened layout file | (empty) |
| `last_output_file` | Last G-Code output path | (empty) |
| `last_input_dir` | Last input file directory | (empty) |
| `last_output_dir` | Last output file directory | (empty) |
| `export_material_thickness` | Material thickness [mm] | `1,50` |
| `export_text_depth` | Text engraving depth [mm] | `0,20` |
| `export_safe_height` | Safe travel height [mm] | `5,00` |
| `font_name` | Active LFF font name | `standard` |
| `grid_visible` | Show grid in canvas | `1` |
| `workspace_w` / `workspace_h` | Machine workspace dimensions [mm] | `300` / `200` |
| `editor_width` | Editor panel width [px] | `345` |
| `logwin_x/y/w/h` | Log window position/size | (auto) |

### Tool Presets (`tools.ini`)

Shared format with [gbr2gcode](https://github.com/JAQUBA/gbr2gcode).

| Key | Description | Example |
|-----|-------------|---------|
| `tool_count` | Number of presets | `9` |
| `tool_N_name` | Tool name | `V-bit 60deg 0.2mm` |
| `tool_N_diameter` | Diameter [mm] | `0.200` |
| `tool_N_stepover` | Stepover [mm] | `0.10` |
| `tool_N_cutDepth` | Cut depth [mm] (negative) | `-0.100` |
| `tool_N_safeHeight` | Safe Z [mm] | `1.00` |
| `tool_N_feedXY` | XY feed [mm/min] | `300.0` |
| `tool_N_feedZ` | Z feed [mm/min] | `100.0` |
| `active_tool` | Active preset index | `0` |

Default presets created on first run: V-bits (60°/30°), end mills (0.8/1.0/2.0 mm), drills (0.8/1.0 mm), laser (0.1 mm). Selecting a tool auto-populates diameter, stepover, text depth and safe Z fields.

---

## Examples

The `examples/` directory contains sample layout files for Polish industrial nameplates:
- **example.txt** — control panel labels (pumps, fans) with frames
- **firstGraw.txt** — multi-row board with mixed text + frames
- **G1.txt** — small framed labels (start/stop/alarm)
- **Graw.txt** — transformer/switchboard labels

---

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Language | C++17 |
| Build system | PlatformIO (native via [JQB_MinGW](https://github.com/JAQUBA/JQB_MinGW)) |
| UI framework | [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) |
| CAM library | [JQB_CAMCommon](https://github.com/JAQUBA/JQB_CAMCommon) |
| Geometry backend | [Clipper2](https://github.com/AngusJohnson/Clipper2) |
| Rendering | WinAPI GDI (via CanvasWindow — zoom/pan/grid) |
| Font format | LibreCAD Font Format (.lff) |
| Target | Windows 10+ (x64) |

---

## Contributing

Contributions are welcome. For substantial changes, keep `README.md` and `.github/copilot-instructions.md` aligned with the implementation, especially when changing build flow, data structures, or UI behavior.

---

## License

The source code in this repository is licensed under the [MIT License](LICENSE).

This repository also depends on third-party components with separate licenses. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for details.

Important: while this repository's own source files are MIT-licensed, distributed binaries must also comply with the licenses of linked dependencies (currently LGPL for JQB_WindowsLib and JQB_CAMCommon, plus BSL-1.0 for Clipper2).

Because the current build uses static linking, a compliant distribution should also include the required third-party license texts and a practical LGPL relinking path for JQB_WindowsLib and JQB_CAMCommon (for example relinkable object files or an equivalent mechanism).

## Acknowledgments

- [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) — native Win32 UI framework
- [JQB_CAMCommon](https://github.com/JAQUBA/JQB_CAMCommon) — shared CAM utilities
- [Clipper2](https://github.com/AngusJohnson/Clipper2) — polygon operations and offsets
