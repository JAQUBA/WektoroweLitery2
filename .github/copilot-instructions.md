# Copilot Instructions — Vector Letters 2

## Project Description

Native Windows desktop application (C++) for generating vector font nameplates for CNC milling and laser engraving. Reads layout definition files (.TXT), parses vector letter data from CSV files, computes tool envelope offsets, renders a real-time GDI preview, and exports G-Code (.gcode) files. Port of the original C# WPF application "WektoroweLitery" to C++17 using JQB_WindowsLib.

## Architecture

```
WektoroweLitery2/
├── src/
│   ├── main.cpp                # Entry point: setup(), loop() — module orchestration
│   ├── AppState.h / .cpp       # Global state, variables, helpers (logMsg, loadSettings, doRunDocument)
│   ├── AppUI.h / .cpp          # UI component creation (createUI) with dark theme
│   ├── MenuHandler.h / .cpp    # Menu bar creation and command routing
│   ├── CanvasWindow.h / .cpp   # VectorCanvas — subclass of library CanvasWindow for document rendering
│   ├── VectorPoint.h           # VectorPoint struct — point with angles and options
│   ├── VectorLetterEngine.h / .cpp # VectorLetterEngine — vector letter engine with tool envelope
│   ├── Document.h              # Document model (collection of table rows)
│   ├── TableRow.h              # Row of nameplates within a document
│   ├── Nameplate.h / .cpp      # Single nameplate — text layout within a frame
│   ├── DocumentParser.h / .cpp # Layout file (.TXT) parser into Document model
│   ├── GCodeEngine.h / .cpp    # G-Code generator for CNC / laser
│   └── theme.h                 # Dark theme color palette
├── resources/
│   ├── app.manifest            # Windows Common Controls v6 manifest
│   ├── resources.rc            # Windows resource file (icon + manifest + version)
│   ├── icon.ico                # Application icon
│   └── fonts/                  # Glyph CSV files (Unicode codepoint filenames)
└── platformio.ini              # PlatformIO config (platform: native via JQB_MinGW)
```

### Module Responsibilities

| Module | Responsibility |
|--------|---------------|
| **main.cpp** | `setup()` / `loop()` — window, menu, UI, canvas init; onClose cleanup |
| **AppState** | Global variable definitions, settings load/save (INI), shared actions (`doRunDocument()`, `doExportGCode()`, `doToggleLogWindow()`, `doToggleGrid()`) |
| **AppUI** | `createUI(SimpleWindow*)` — creates toolbar with input fields and buttons |
| **MenuHandler** | `createAppMenu()` — MenuBar with File, View, Help menus |
| **CanvasWindow** (VectorCanvas) | Subclass of JQB_WindowsLib CanvasWindow — renders documents, nameplates, and vector toolpaths on the canvas |
| **VectorPoint** (VectorPoint.h) | Point struct with coordinates, angles (alphaPrimary, alphaMean), serif flag, terminator flag |
| **VectorLetterEngine** (VectorLetterEngine.h/.cpp) | Core vector engine: CSV import, angle computation, envelope generation, toolpath calculation |
| **Document** | Document-level parameters (materialThickness, textDepth, safeHeight, diameter, laser mode) + collection of TableRows |
| **TableRow** | Row of Nameplates |
| **Nameplate** | Text layout engine: loads letter CSVs, positions, centers within frame, generates toolpaths |
| **DocumentParser** | Parses semicolon-separated layout files into Document model |
| **GCodeEngine** | Generates G-Code (G00/G01/M03/M05/M30) without line numbering, with milling/laser mode support |

## Tech Stack

- **Language**: C++17
- **Build system**: PlatformIO (`platform = native` via JQB_MinGW)
- **UI framework**: [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) — lightweight Win32 UI library
- **Rendering**: WinAPI GDI (via reusable CanvasWindow from JQB_WindowsLib — zoom/pan/grid inherited)
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
- `setup()` → load settings → window + menu → UI → canvas
- `loop()` → main loop (empty — event-driven via callbacks)
- UI components created via `new` and added to `SimpleWindow` via `window->add()`
- **Do not change** the `setup()` and `loop()` signatures — framework entry points

### Dark Theme

Uses Tokyo Night base from JQB_WindowsLib, extended with app-specific colors in `theme.h`.

### Menu Bar

```
File → Run document | Export G-Code | Exit
View → Show grid | Reset view | Log window
Help → About...
```

### Layout File Format (.TXT)

Semicolon-separated commands (lines starting with `#` are ignored as comments):
- `l` — new row (line break)
- `t;width;height;dx;dy;?;textH;condensation;thickness;text` — text-only nameplate
- `tw;width;height;dx;dy;?;textH;condensation;thickness;text` — nameplate with frame
- `w;width;height` — frame-only element

**Z coordinate convention:** Bottom of material = Z0.0 (lowest point). Surface = Z(materialThickness). Text engraving → Z = materialThickness − textDepth. Frame cutting → Z = 0.0. Rapid travel → Z = materialThickness + safeHeight.

### CSV Letter Format

Each letter is a semicolon-separated CSV file in `resources/fonts/` (e.g., `65.csv` for 'A', `321.csv` for 'Ł'):
- `x;y` — point coordinates
- `x;y;options` — point with flags: `h` = serif, `z` = terminator, `k` = new segment

### Vector Envelope Algorithm

1. Import letter points from CSV
2. Compute primary angles (`alphaPrimary`) between consecutive points
3. Compute mean angles (`alphaMean`) for smooth envelope joints
4. Draw segment axis (center line)
5. Generate envelope offsets at computed angles (tool compensation)
6. Handle serif endpoints and perpendicular endcaps
7. Multiple passes with increasing offset for thick strokes

### G-Code Format

Plain lines (no `Nxxxx` numbering) with:
- `G90` — absolute positioning
- `F1000` — feed rate command inserted after `G90`
- `G21` — millimeter units
- `G00` — rapid move (idle)
- `G01` — linear interpolation (working)
- `M03` / `M05` — laser on/off (laser mode)
- `M30` — program end
- Decimal separator in output is `.`

### Configuration (config.ini)

| Key | Description | Default |
|-----|-------------|---------|
| `last_input_file` | Last opened layout file | (empty) |
| `last_output_file` | Last G-Code output path | (empty) |
| `last_input_dir` | Last input file directory | (empty) |
| `last_output_dir` | Last output file directory | (empty) |
| `export_diameter` | Tool diameter [mm] | `0,30` |
| `export_stepover` | Tool stepover [mm] | `0,15` |
| `export_material_thickness` | Material thickness [mm] | `1,50` |
| `export_text_depth` | Text engraving depth [mm] | `0,20` |
| `export_safe_height` | Safe travel height [mm] | `5,00` |
| `grid_visible` | Show grid in canvas | `1` |
| `logwin_x/y/w/h` | Log window position/size | (auto) |

## Copilot Guidelines

### Adding New UI Components
Create in `AppUI.cpp` → `createUI()`. Use `styleBtn()` helper for button styling. If accessed from other modules, declare `extern` in `AppState.h`.

### Adding New Menu Commands
Add via `menuBar->addMenu()` / `m.addItem()` in `MenuHandler.cpp`. Shared actions go in `AppState.h/.cpp` as `doXxx()` functions.

### Adding New Settings
1. Add `extern` variable in `AppState.h`, define in `AppState.cpp`
2. Load in `loadSettings()`, save in `saveSettings()`

### Extending Canvas Rendering
Add drawing methods to `VectorCanvas` in `CanvasWindow.cpp`. Base canvas features (zoom/pan/grid/double-buffer) are inherited from JQB_WindowsLib `CanvasWindow`. World coordinates are in mm; use `toScreenX()`/`toScreenY()` for transforms.

### Adding New Letter Formats
Extend `VectorLetterEngine::importFromCSV()` in `VectorLetterEngine.cpp`.

### Extending G-Code Output
Add methods to `GCodeEngine`. Keep plain commands without `N` line numbering.

### Font Files Path
- Glyph CSV files are loaded from `resources/fonts/` resolved relative to executable location.
- Do not reintroduce manual folder selection in UI unless explicitly requested.

### SimpleWindow is a Singleton
Do not create a second one. For additional windows use `OverlayWindow` or raw WinAPI with `GWLP_USERDATA`.

### Logging
Use `logMsg(const wchar_t*)` or `logMsg(const std::wstring&)` from `AppState.h`. Logs displayed in `LogWindow`.
