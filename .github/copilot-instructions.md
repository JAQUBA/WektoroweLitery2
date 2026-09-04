# Copilot Instructions — Vector Letters 2

> **IMPORTANT:** Keep this file, `README.md`, and licensing docs (`LICENSE`, `THIRD_PARTY_NOTICES.md`) up to date whenever you add, rename, or remove modules, change data structures, add UI components, modify G-Code output, change configuration keys, or change dependencies/licenses.

## Project Description

Native Windows desktop application (C++) for generating vector font nameplates for CNC milling and laser engraving. Reads layout definition files (.TXT), parses vector letter data from LFF font files (LibreCAD Font Format), computes tool envelope offsets, renders a real-time GDI preview, and exports G-Code (.gcode) files with G2/G3 arc fitting. Port of the original C# WPF application "WektoroweLitery" to C++17 using JQB_WindowsLib.

## Architecture

```
WektoroweLitery2/
├── src/
│   ├── main.cpp              # Entry point: setup(), loop() — window, canvas, close handler
│   ├── AppState.h / .cpp     # Global state, ToolPreset, settings, shared actions, font loading
│   ├── AppUI.h / .cpp        # Toolbar, editor, splitter, font/tool popups, error highlighting
│   ├── MenuHandler.h / .cpp  # Menu bar creation and command routing
│   ├── CanvasWindow.h / .cpp # VectorCanvas — subclass of library CanvasWindow for document rendering
│   ├── theme.h               # Dark theme color palette (extends ThemeTokyoNight)
│   ├── Document/             # Document model and parsing
│   │   ├── Document.h            # Document model (settings + collection of table rows)
│   │   ├── TableRow.h            # Row of nameplates within a document
│   │   ├── Nameplate.h / .cpp    # Single nameplate — text layout, glyph loading, centering
│   │   └── DocumentParser.h / .cpp # Layout file (.TXT) parser into Document model
│   ├── Font/                 # Vector font engine
│   │   ├── VectorPoint.h        # VectorPoint struct — point with angles, serif, terminator
│   │   ├── VectorLetterEngine.h / .cpp # Vector envelope: angle computation, offset generation, multi-pass
│   │   └── LffFont.h / .cpp     # LibreCAD Font Format (.lff) parser
│   └── GCode/                # G-Code generation
│       └── GCodeEngine.h / .cpp  # GRBL-compatible G-Code with G2/G3 arc fitting
├── resources/
│   ├── app.manifest          # Windows Common Controls v6 manifest
│   ├── resources.rc          # Windows resource file (icon + manifest + version)
│   ├── icon.ico              # Application icon
│   └── fonts/                # LFF vector font files (26 fonts)
├── examples/                 # Example layout files (.TXT)
├── scripts/
│   └── copy_fonts.py         # Post-build: copies fonts to output directory
└── platformio.ini            # PlatformIO config (platform: native via JQB_MinGW)
```

### Module Responsibilities

| Module | Responsibility |
|--------|---------------|
| **main.cpp** | `setup()` / `loop()` — window creation (1200×700), menu, UI, canvas init, `doRelayout()`, close handler saves settings |
| **AppState** | `ToolPreset` struct. Global state: `currentDocument`, `activeFont`, tool presets, export params. Functions: `loadSettings()` / `saveSettings()`, `doRenderPreview()`, `doExportGCode()`, `doNewFile()` / `doOpenFile()` / `doSaveFile()` / `doSaveFileAs()`, `doShowToolPresets()`, `doShowWorkspaceSettings()`, `doRelayout()`, `applyActiveToolPreset()`, `loadFont()`, file dialogs, `logMsg()` |
| **AppUI** | `createUI()` — Export button, tool/font selector popups, Material/Depth/Safe Z fields, output path field, RichEdit editor (Consolas 12pt, auto-render with 300 ms debounce), draggable splitter, `highlightEditorErrors()`, `showToolPopup()` / `showFontPopup()` |
| **MenuHandler** | `createAppMenu()` — File (New, Open, Save, Save As, Export, Exit), View (fit to content, fit to workspace, reset view, grid, rapid moves, vector arrows, HUD, log), Settings (tool presets, workspace), Help (About). The About dialog lists bundled libraries and their licenses. |
| **CanvasWindow** (VectorCanvas) | Subclass of JQB_WindowsLib `CanvasWindow` — renders workspace bounds, WCS origin indicator (X0, Y0), G0 rapid traverse paths, letter vectors + frames, start points & direction arrows, HUD overlay, `setDocument()`, `fitToContent()`, `fitToWorkspace()` |
| **Font/VectorPoint** | Point struct: `x`, `y`, `alphaPrimary`, `alphaMean`, `widthFactor`, `hasSerif`, `isTerminator`, `options` |
| **Font/LffFont** | LFF font parser: `load(path)` parses glyphs (header, polylines, arcs, references), `getGlyph(codePoint)`, `getLetterSpacing()`, `listFonts(dir)`. Structs: `LffGlyph` (codePoint, width, strokes), `LffPoint` (x, y, bulge) |
| **Font/VectorLetterEngine** | Core engine: constructs from `LffGlyph` with scale/offset/tool params, `generateFullPath()` → `PointCollection` output. Steps: `computeAlphaPrimary()` → `computeAlphaMean()` → `drawSegmentAxis()` → `drawSegmentEnvelope()` (forward pass, endcap, reverse, startcap, multi-pass). Transforms: `multiplyX()`, `addX()` |
| **Document/Document** | Fields: `millingDiameter_mm`, `stepover_mm`, `materialThickness_mm`, `textDepth_mm`, `safeHeight_mm`, `feedXY_mm`, `feedZ_mm`, `laserMode`. Methods: `addRow()`, `getRows()`, `swapAxes()` for optional X/Y generation transposition around X0/Y0 |
| **Document/TableRow** | Row of `Nameplate` objects: `addNameplate()`, `getNameplates()` |
| **Document/Nameplate** | Fields: frame geometry, `text`, `textHeight_mm`, `condensation`, `thickness`, `diameter`, `stepover`, `hasFrame`. Method: `appendText(text, font)` — UTF-8 → Unicode, loads glyphs, applies scaling/condensation, centers in frame, generates envelope via VectorLetterEngine |
| **Document/DocumentParser** | Parses semicolon-separated layout files: `t`/`tw`/`w`/`l` commands, decimal comma→dot conversion, UTF-8 BOM stripping, error line tracking |
| **GCode/GCodeEngine** | GRBL export: preamble (`G21 G90 G17 G94 G54 G91.1`), G00/G01/G02/G03, greedy arc fitting (tolerance 0.01 mm), collinear reduction (0.005 mm), feed rate management (`F{feedZ}` on Z plunge, `F{feedXY}` on first XY), milling/laser mode, `exportDocument()` / `exportSingleFrame()`, optimization stats |

## Tech Stack

- **Language**: C++17
- **Build system**: PlatformIO (`platform = native` via JQB_MinGW)
- **UI framework**: [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) — lightweight Win32 UI library
- **CAM library**: [JQB_CAMCommon](https://github.com/JAQUBA/JQB_CAMCommon) — reusable CAM utilities (G-code generation, arc math)
- **Rendering**: WinAPI GDI (via reusable CanvasWindow from JQB_WindowsLib — zoom/pan/grid inherited)
- **Font format**: LibreCAD Font Format (.lff)
- **Target platform**: Windows 10+ (x64)

## Coding Conventions

### Code Style

- All code, comments, class names, methods, and variables in **English**
- Use `std::string` for file paths and internal data
- Use `std::wstring` and `L"..."` for displayed UI text
- Use section comments `// --- ... ---` for code organization in .cpp files
- Indentation: 4 spaces (no tabs)
- Explicit Wide WinAPI versions: `CreateFontW()`, `CreateWindowExW()`, `MessageBoxW()` etc.
- Do not use `std::thread` (use `CreateThread`), `std::to_wstring` (use `jqb_compat::to_wstring`)

### Application Pattern

- JQB_WindowsLib defines `setup()` and `loop()` — Arduino-like
- `setup()` → load settings → window + menu → UI → canvas → relayout → load last file
- `loop()` → main loop (empty — event-driven via callbacks)
- UI components created via `new` and added to `SimpleWindow` via `window->add()`
- **Do not change** the `setup()` and `loop()` signatures — framework entry points

### Dark Theme

Uses Tokyo Night base from JQB_WindowsLib (`ThemeTokyoNight.h`), extended with app-specific colors in `theme.h`:
- `CLR_ACTION_*` — action buttons
- `CLR_TOOL_*` — tool selector
- `CLR_EXPORT_*` — export button (green)
- `CLR_CANVAS_BG`, `CLR_GRID_LINE` — canvas
- `CLR_VECTOR_LINE` (green), `CLR_FRAME_LINE` (red) — rendering
- `CLR_EDITOR_BG`, `CLR_EDITOR_TEXT`, `CLR_ERROR_TEXT` — editor
- `CLR_SPLITTER`, `CLR_SPLITTER_HOVER` — splitter
- `CLR_WORKSPACE_LINE` — workspace bounds

### Menu Bar

```
File      → New | Open... | Save | Save As... | Export G-Code | Exit
View      → Show grid | Reset view | Log window
Settings  → Rotate generation 90 deg (swap X/Y) | Tool presets... | Machine workspace size...
Help      → About...
```

### UI Layout

```
┌──────────────────────────────────────────────────────────┐
│ [Export GCode] [▼ Tool preset] [▼ Font] Material|Depth|Safe Z │  ← Row 1
│ Output: [path field] [...] │ Info label                       │  ← Row 2
├────────────────┬──┬──────────────────────────────────────┤
│ RichEdit       │░░│ Canvas (VectorCanvas)                │  ← Split
│ layout editor  │░░│ zoom/pan/grid                        │
│ (Consolas 12)  │░░│ workspace bounds + letters + frames  │
└────────────────┴──┴──────────────────────────────────────┘
                  ↑ draggable splitter
```

---

## Layout File Formats (.VL2 and legacy .TXT)

New layout files use the simple block-based `.vl2` format. It is UTF-8,
versioned, independent of indentation, and uses `[section]` headers with
`key=value` properties. Templates avoid repeating dimensions and support
single-line plates (`type=plate`), multi-line plates (`type=multiline` with
`line1`, `line2`, ...), and empty frames (`type=empty`). Example:

```
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
```

The parser expands VL2 into the existing `Document` model, so geometry and
G-Code generation remain format-independent. Legacy `.txt` files using `t`,
`tw`, `w`, and `l` remain supported and are used for compatibility tests.
For daily editing, VL2 also supports compact rows such as `pump: POMPA 1` and
`control: STEROWANIE | ZDALNE`; `|` separates multiline text. Local overrides
are written in the selector, for example
`pump(size=200x40,text=12): FALOWNIK 1`.
Set `frame=false` for text without a visible frame. `offset=x,y` moves text
relative to the plate center for both framed and unframed plates. In a
template, short `text=4` means text height; row content remains after `:`.
In `.vl2`, `thickness` is the **real stroke width in mm** (e.g. `0.4` for a
0.4 mm wide line) — unlike the legacy `.txt` format below, where `thickness`
is an abstract stroke-width unit. `DocumentParser` converts the VL2 mm value
to the legacy unit internally (`units = mm * 100 / text_height_mm`).

## Legacy Layout File Format (.TXT)

Semicolon-separated commands (lines starting with `#` are ignored as comments):
- `l` — new row (line break)
- `t;width;height;dx;dy;textH;condensation;thickness;text` — text-only nameplate
- `tw;width;height;dx;dy;textH;condensation;thickness;text` — nameplate with frame
- `w;width;height` — frame-only element

Parameters: width/height in mm, dx/dy = text offset, textH = font size in mm, condensation = horizontal scale (100 = normal, 55 = narrow), thickness = stroke width units.

---

## LFF Font Format

Vector fonts use LibreCAD Font Format (.lff) files stored in `resources/fonts/`. Each .lff file contains all glyphs for a font.

**File format:**
- `# Header` — comments with font name, letter spacing, word spacing
- `[XXXX] char` — glyph header (hex Unicode codepoint)
- `x1,y1;x2,y2;x3,y3,Abulge` — polyline with optional arc bulge
- `CXXXX` — reference to another glyph (inheritance, 2-level resolution)
- Coordinates are in ~0–9 range (9 = uppercase height), scaled internally via `LFF_SCALE = 3000.0 / 9.0`

**Available fonts** (26 files): standard (default), simplex, romans, romanc, romand, romanp, romant, romansi, cursive, scriptc, scripts, italicc, italiccs, italict, gothgbt, gothgrt, gothitt, iso, iso3098, iso3098_i, greekc, greeks, cyrillic_ii, kst32b, unicode.

---

## Vector Envelope Algorithm

1. Import letter points from LFF glyph (with arc tessellation from bulge values)
2. `computeAlphaPrimary()` — angle between consecutive points
3. `computeAlphaMean()` — averaged angles for smooth envelope joints
4. `drawSegmentAxis()` — draw center line (back to front)
5. `drawSegmentEnvelope()` — forward pass, end cap, reverse pass, start cap
   - Each point offset by `alphaMean` at distance = pen width
   - Serif endpoints use perpendicular endcaps
6. Multiple passes with increasing offset for thick strokes (stepover-based)
7. Output: `PointCollection` ready for G-Code generation

---

## G-Code Engine

### Preamble (GRBL Compatible)

```gcode
G21 G90 G17 G94 G54
G91.1
G00 Z6.50
G00 X0.000 Y0.000
```

- `G21` — mm units
- `G90` — absolute positioning
- `G17` — XY plane (required for G02/G03 arcs)
- `G94` — feed rate in units/min
- `G54` — work coordinate system
- `G91.1` — incremental arc center (I/J relative to start)

### Z Coordinate Convention

- Bottom of material = Z0.0 (lowest point)
- Surface = Z(materialThickness)
- Text engraving = Z(materialThickness − textDepth), clamped to min Z0
- Frame cutting = Z0.0
- Rapid travel = Z(materialThickness + safeHeight)

### Feed Rate Management

- `F{feedZ}` emitted on Z plunge moves (`G01 Z...`)
- `F{feedXY}` emitted on first XY working move after each Z plunge
- Feed rates sourced from active tool preset in `tools.ini`

### Path Optimization Pipeline

1. Scale to mm, remove duplicate points (< 0.003 mm)
2. Compute turn angles at each interior point
3. Split at sharp corners (> ~25°)
4. **Greedy arc fit** — `fitCircle3()` through 3 points, extend while within tolerance (0.01 mm)
5. Validate GRBL arc constraints (radius 0.05–100 mm, start/end radii within 0.1%)
6. **Collinear reduction** — merge co-linear segments within 0.005 mm
7. Emit `G01` for lines, `G02`/`G03` for arcs with `I`,`J` center offsets

### Laser Mode

When `laserMode = true`:
- `M03` replaces Z plunge (spindle/laser on)
- `M05` replaces Z retract (spindle/laser off)
- No Z moves emitted

### Epilog

```gcode
(Optimized: N moves (M arcs G2/G3, K lines G01) from P raw points (D duplicates removed))
G00 Z6.50
G00 X0.000 Y0.000
M30
```

---

## Configuration

### Application Settings (config.ini)

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
| `rapid_moves_visible` | Show G0 rapid moves in canvas | `0` |
| `vector_arrows_visible` | Show vector start points and direction arrows | `0` |
| `hud_visible` | Show canvas HUD overlay | `0` |
| `repeat_frame_cut` | Cut each frame contour twice when exporting G-Code | `0` |
| `swap_generation_axes` | Rotate generation 90 degrees around X0/Y0 while keeping labels readable | `0` |
| `workspace_w` / `workspace_h` | Machine workspace dimensions [mm] | `300` / `200` |
| `editor_width` | Editor panel width [px] | `345` |
| `window_maximized` | Main window starts maximized | `1` |
| `window_x` / `window_y` / `window_w` / `window_h` | Restored (non-maximized) window position/size [px] | `-1` (unset — use OS default) |
| `logwin_x/y/w/h` | Log window position/size | (auto) |

### Tool Presets (tools.ini)

Tool presets are stored in a separate `tools.ini` file (not `config.ini`). Format compatible with gbr2gcode — both projects use the same key schema:

| Key pattern | Description | Example |
|-------------|-------------|---------|
| `tool_count` | Number of tool presets | `10` |
| `tool_N_name` | Tool name | `V-bit 60deg 0.2mm` |
| `tool_N_diameter` | Tool diameter [mm] | `0.300` |
| `tool_N_stepover` | Tool stepover [mm] | `0.10` |
| `tool_N_cutDepth` | Legacy cut depth [mm] (not applied automatically) | `-0.100` |
| `tool_N_safeHeight` | Legacy safe travel height [mm] (not applied automatically) | `1.00` |
| `tool_N_feedXY` | XY feed rate [mm/min] | `300.0` |
| `tool_N_feedZ` | Z feed rate [mm/min] | `100.0` |
| `tool_N_spindleRPM` | Spindle speed [RPM] | `12000` |
| `active_tool` | Active tool index | `0` |

Default presets created automatically on first run: V-bits (60° 0.3/0.2/0.1 mm and 30° 0.1 mm), end mills (0.8/1.0/2.0 mm), drills (0.8/1.0 mm), laser (0.1 mm). The default font is `standard`. Selecting a tool auto-populates diameter, stepover, feed rates, and spindle RPM. Engraving depth and safe Z remain operation settings. For milling, G-Code emits `S<RPM> M03` and ends with `M05`; laser mode uses `M03`/`M05` without spindle RPM.

---

## Copilot Guidelines

### Adding New UI Components
Create in `AppUI.cpp` → `createUI()`. Use `styleBtn()` helper for button styling. If accessed from other modules, declare `extern` in `AppState.h`. Call `doRelayout()` if component affects layout.

### Adding New Menu Commands
Add via `menuBar->addMenu()` / `m.addItem()` in `MenuHandler.cpp`. Shared actions go in `AppState.h/.cpp` as `doXxx()` functions.

### Adding New Settings
1. Add `extern` variable in `AppState.h`, define in `AppState.cpp`
2. Load in `loadSettings()`, save in `saveSettings()`

### Extending Canvas Rendering
Add drawing methods to `VectorCanvas` in `CanvasWindow.cpp`. Base canvas features (zoom/pan/grid/double-buffer) are inherited from JQB_WindowsLib `CanvasWindow`. World coordinates are in mm; use `toScreenX()`/`toScreenY()` for transforms.

### Adding New Fonts
Add `.lff` font files to `resources/fonts/`. The `LffFont` parser handles them automatically. `showFontPopup()` enumerates available fonts.

### Extending G-Code Output
Add methods to `GCodeEngine` in `GCode/GCodeEngine.cpp`. Keep plain commands without `N` line numbering. Arc fitting constants are in `GCodeEngine.h`.

### Font Files Path
- LFF font files loaded from `resources/fonts/` resolved relative to executable location.
- Active font name stored in `config.ini` as `font_name`.
- Post-build script `scripts/copy_fonts.py` copies fonts to output directory.

### Editor Integration
- `getEditorText()` / `setEditorText()` in `AppState.cpp` handle RichEdit content
- `highlightEditorErrors(errorLines)` in `AppUI.cpp` applies red underline formatting
- Auto-render debounce timer (300 ms) triggers `doRenderPreview()` on EN_CHANGE

### SimpleWindow is a Singleton
Do not create a second one. For additional windows use `OverlayWindow` or raw WinAPI with `GWLP_USERDATA`.

### Logging
Use `logMsg(const wchar_t*)` or `logMsg(const std::wstring&)` from `AppState.h`. Logs displayed in `LogWindow`.

### Coordinate System
- LFF font coordinates: 0–9 range (9 = uppercase height)
- Internal units: scaled by `LFF_SCALE = 3000.0 / 9.0` (0–3000 range)
- World coordinates: mm (after scaling by `900000.0 / textHeight_mm`)
- Canvas: `toScreenX(worldX)` / `toScreenY(worldY)` — Y is flipped (0 at bottom)
- G-Code: mm, Z0 = bottom of material

---

## Keeping Documentation Current

When making changes to this project, **always update these files**:

- **`.github/copilot-instructions.md`** (this file) — when adding/removing modules, changing data structures, adding config keys, modifying G-Code output format, adding UI components, or changing the architecture
- **`README.md`** — when adding user-visible features, changing build instructions, or modifying the application workflow
- **`THIRD_PARTY_NOTICES.md`** — when adding/removing dependencies or changing licensing/distribution obligations
- **`LICENSE`** — when changing the project's own license

---

### Build Configuration Note

The checked-in `platformio.ini` uses GitHub dependencies for `JQB_WindowsLib` and `JQB_CAMCommon`. For local multi-repo development, these can be switched to `../JQB_WindowsLib` and `../JQB_CAMCommon`, where the standalone JQB_CAMCommon library lives at `d:\Programowanie\JQB_CAMCommon`.

Clipper2 is provided through local `lib/Clipper2/CPP` in this repository. The shared pre-build script is wired in the JQB_CAMCommon library manifest (`library.json` → `build.extraScript`) and automatically clones Clipper2 from GitHub when `lib/Clipper2` is missing, so builds work out-of-the-box after clone.
