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

#endif /* THEME_H */
