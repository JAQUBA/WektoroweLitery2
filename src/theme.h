// ============================================================================
// theme.h — Dark theme color palette for WektoroweLitery2
// ============================================================================
#ifndef THEME_H
#define THEME_H

#include <UI/Theme/ThemeTokyoNight.h>

// ============================================================================
// Application-specific colors (derived from theme)
// ============================================================================

/* Window background */
#define CLR_WIN_BG          CLR_BASE

/* Action buttons (Run, Draw) */
#define CLR_ACTION_BG       CLR_ACCENT
#define CLR_ACTION_HOVER    CLR_ACCENT_H
#define CLR_ACTION_TEXT     CLR_BG

/* Tool buttons */
#define CLR_TOOL_BG         CLR_BTN_BG
#define CLR_TOOL_HOVER      CLR_BTN_HOVER
#define CLR_TOOL_TEXT       CLR_TEXT

/* Export / Save */
#define CLR_EXPORT_BG       CLR_SURFACE3
#define CLR_EXPORT_HOVER    CLR_TEAL
#define CLR_EXPORT_TEXT     CLR_GREEN

/* Canvas drawing colors (GDI) */
#define CLR_CANVAS_BG       CLR_BG
#define CLR_GRID_LINE       CLR_SURFACE2
#define CLR_VECTOR_LINE     CLR_GREEN
#define CLR_FRAME_LINE      CLR_RED

/* Status / info */
#define CLR_INFO_TEXT       CLR_SUBTEXT

/* Label text */
#define CLR_LABEL_TEXT      CLR_TEXT

/* Editor (layout file inline editor) */
#define CLR_EDITOR_BG       CLR_BG
#define CLR_EDITOR_TEXT     CLR_TEXT
#define CLR_ERROR_TEXT      CLR_RED

/* Splitter */
#define CLR_SPLITTER        CLR_SURFACE3
#define CLR_SPLITTER_HOVER  CLR_ACCENT

/* Workspace boundary */
#define CLR_WORKSPACE_LINE  CLR_SURFACE3

/* CAM / CAD visualization colors */
#define CLR_RAPID_LINE      RGB(249, 226, 175)  /* Peach / Amber G0 rapid moves */
#define CLR_ORIGIN_X        RGB(243, 139, 168)  /* Red for WCS X0 axis */
#define CLR_ORIGIN_Y        RGB(166, 227, 161)  /* Green for WCS Y0 axis */
#define CLR_TOOL_KERF       RGB(116, 199, 236)  /* Faint Sapphire/Blue for tool diameter */
#define CLR_HUD_TEXT        RGB(186, 194, 222)  /* Text overlay HUD color */
#define CLR_HUD_BG          RGB(24, 24, 37)     /* Dark background for HUD overlay */

#endif /* THEME_H */
