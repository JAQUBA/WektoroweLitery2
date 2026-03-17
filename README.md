# Wektorowe Litery 2

Native Windows desktop application (C++) for generating vector font nameplates for CNC milling and laser engraving.

Port of the original C# WPF application [WektoroweLitery](../WektoroweLitery/) to C++17 using [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib).

## Features

- Reads layout definition files (.TXT) with semicolon-separated commands
- Parses vector letter data from CSV files (one file per character)
- Computes tool envelope offsets for CNC milling compensation
- Real-time GDI preview with zoom/pan navigation
- Exports G-Code (.NC) files for CNC milling and laser engraving
- Dark themed UI (Catppuccin Mocha)
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

1. Set the CSV directory containing vector letter files (named by ASCII code, e.g. `65.csv` for 'A')
2. Open a layout file (.TXT) via File → Run document
3. Preview the result in the canvas (scroll to zoom, drag to pan)
4. Export G-Code via File → Export G-Code

## Layout File Format

Semicolon-separated commands:
- `p;diameter;stepover` — laser mode
- `f;diameter;stepover;idleZ;workZ;cutZ` — milling mode
- `l` — new row
- `t;W;H;dx;dy;?;textH;condensation;thickness;text` — text nameplate
- `tw;W;H;dx;dy;?;textH;condensation;thickness;text` — nameplate with frame
- `w;W;H` — frame only

## License

See [LICENSE](LICENSE).
