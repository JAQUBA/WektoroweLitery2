# Wektorowe Litery 2

Native Windows desktop application (C++) for generating vector font nameplates for CNC milling and laser engraving.

Port of the original C# WPF application [WektoroweLitery](../WektoroweLitery/) to C++17 using [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib).

## Features

- Reads layout definition files (.TXT) with semicolon-separated commands
- Parses vector letter data from CSV files (one file per Unicode character)
- Computes tool envelope offsets for CNC milling compensation
- Real-time GDI preview with zoom/pan navigation
- Exports G-Code (`.gcode`) files for CNC milling and laser engraving
- G-Code output without `Nxxxx` line numbering
- G-Code prolog includes `G90`, `F1000`, `G21`
- Decimal separator in output G-Code is `.`
- Built-in dark theme (Tokyo Night)
- Configurable settings saved to INI file

## Build

Requires [PlatformIO](https://platformio.org/).

```bash
pio run              # Build
pio run -t upload    # Build and run
```

Dependencies are fetched automatically:
- [JQB_MinGW](https://github.com/JAQUBA/JQB_MinGW) — MinGW-w64 toolchain platform
- [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) — Win32 UI library

## Usage

1. Place glyph CSV files in `resources/fonts/` (already included in this repository).
2. Open a layout file (`.TXT`) with the Input file picker.
3. Click `Run` to parse and preview the layout (scroll to zoom, drag to pan, double-click to reset view).
4. Set export parameters in UI (all positive values in mm):
	- `Material` — material thickness (cutting depth = full thickness)
	- `Text depth` — engraving depth below surface
	- `Safe Z` — safe travel height above material
5. Choose output `.gcode` file and click `Export GCode`.

**Z coordinate convention:** Surface of material = Z0.0. Text engraving goes to Z = -(text depth). Frame cutting goes to Z = -(material thickness). Rapid travel at Z = +(safe height).

## Layout File Format

Semicolon-separated commands (lines starting with `#` are ignored as comments):
- `l` — new row
- `t;W;H;dx;dy;?;textH;condensation;thickness;text` — text nameplate
- `tw;W;H;dx;dy;?;textH;condensation;thickness;text` — nameplate with frame
- `w;W;H` — frame only

## Glyph CSV Files

- Location: `resources/fonts/`
- File naming: Unicode code point (e.g. `65.csv` = `A`, `321.csv` = `Ł`, `46.csv` = `.`)
- Each CSV line stores vector points in format:
	- `x;y`
	- `x;y;options` where options can contain flags (`h`, `z`, `k`)

## Exported G-Code Format

- File extension: `.gcode`
- No `Nxxxx` line numbering
- Prolog:
	- `G90`
	- `F1000`
	- `G21`
- Epilog ends with `M30`

Example:

```gcode
G90
F1000
G21
G00 Z5.00
G00 X0.000 Y0.000
G00 Z5.00
G00 X0.000 Y0.000
G01 Z-1.50
G01 X0.000 Y30.000
...
G00 Z5.00
G00 X0.000 Y0.000
M30
```

## Notes

- The application expects glyph CSV files to be available in `resources/fonts/`.
- If glyph files are missing, preview/export for those characters can be incomplete.
- Export parameters entered in UI are used at export time.
- All depth/height values are positive numbers. The engine computes correct Z signs automatically.

## License

See [LICENSE](LICENSE).
