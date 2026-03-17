// ============================================================================
// theme.h — Dark theme color palette for WektoroweLitery2
// ============================================================================
#ifndef THEME_H
#define THEME_H

#include <UI/Theme/ThemeCatppuccinMocha.h>

// ============================================================================
// Application-specific colors
// ============================================================================

/* Window background */
#define CLR_WIN_BG          RGB(30, 30, 38)

/* Action buttons (Run, Draw) */
#define CLR_ACTION_BG       RGB(40, 130, 200)
#define CLR_ACTION_HOVER    RGB(55, 150, 220)
#define CLR_ACTION_TEXT     RGB(240, 245, 255)

/* Tool buttons (Add X, Multiply X, etc.) */
#define CLR_TOOL_BG         RGB(50, 52, 62)
#define CLR_TOOL_HOVER      RGB(65, 68, 80)
#define CLR_TOOL_TEXT       RGB(200, 210, 225)

/* Export / Save */
#define CLR_EXPORT_BG       RGB(45, 90, 90)
#define CLR_EXPORT_HOVER    RGB(55, 110, 110)
#define CLR_EXPORT_TEXT     RGB(150, 240, 220)

/* Canvas drawing colors (GDI) */
#define CLR_CANVAS_BG       RGB(22, 22, 28)
#define CLR_GRID_LINE       RGB(50, 50, 60)
#define CLR_VECTOR_LINE     RGB(0, 200, 80)
#define CLR_FRAME_LINE      RGB(200, 50, 50)

/* Status / info */
#define CLR_INFO_TEXT       RGB(180, 190, 200)

#endif /* THEME_H */
