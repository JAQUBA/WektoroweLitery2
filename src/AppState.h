// ============================================================================
// AppState.h — Global application state, shared variables, helpers
// ============================================================================
#ifndef APP_STATE_H
#define APP_STATE_H

#include <string>
#include <vector>
#include <windows.h>

// Forward declarations — JQB_WindowsLib classes
class SimpleWindow;
class Label;
class Button;
class ConfigManager;
class LogWindow;

// Forward declarations — application classes
class Document;
class LffFont;

// Tool preset
struct ToolPreset {
    std::string name;
    std::string diameter;
    std::string stepover;
};

// ============================================================================
// UI Components (global — accessible from multiple modules)
// ============================================================================
extern SimpleWindow*   window;
extern Label*          lblStatus;
extern Label*          lblInfo;
extern LogWindow*      logWindow;
extern HWND            hEditor;       // Layout file inline editor (multiline EDIT)

// Forward declarations — InputField
class InputField;

// Material parameter input fields (visible in toolbar)
extern InputField*     fldMaterial;
extern InputField*     fldDepth;
extern InputField*     fldSafeH;

// ============================================================================
// Application objects
// ============================================================================
extern ConfigManager   config;
extern Document*       currentDocument;
extern LffFont*        activeFont;
extern std::string     activeFontName;

// ============================================================================
// Application state
// ============================================================================
extern std::string     currentFilePath;   // Currently edited layout file (empty = untitled)
extern std::string     lastInputDir;
extern std::string     lastOutputFile;
extern std::string     lastOutputDir;
extern std::string     exportDiameter;
extern std::string     exportStepover;
extern std::string     exportMaterialThickness;
extern std::string     exportTextDepth;
extern std::string     exportSafeHeight;
extern bool            gridVisible;

// Tool presets
extern std::vector<ToolPreset> toolPresets;
extern int             activeToolIndex;

// Machine workspace size (mm)
extern double          workspaceWidth;
extern double          workspaceHeight;

// Editor splitter position (pixels from left edge of content area)
extern int             editorWidth;

// ============================================================================
// Helper functions
// ============================================================================
void logMsg(const wchar_t* msg);
void logMsg(const std::wstring& msg);
void loadSettings();
void saveSettings();

// Editor helpers
std::string getEditorText();
void setEditorText(const std::string& text);
void updateWindowTitle();

// File dialog helpers
std::string extractDir(const std::string& filePath);
std::string openFileDialog(HWND owner, const wchar_t* filter, const wchar_t* title,
                            const std::string& initialDir);
std::string saveFileDialog(HWND owner, const wchar_t* filter, const wchar_t* title,
                            const wchar_t* defaultExt, const std::string& initialDir);

// Shared actions (used by UI buttons and menu)
void doRenderPreview();
void doExportGCode();
void doToggleLogWindow();
void doToggleGrid();
void doNewFile();
void doOpenFile();
void doSaveFile();
void doSaveFileAs();
void doShowWorkspaceSettings();
void doRelayout();
void applyActiveToolPreset();
void doSelectTool(int index);
void doShowToolPresets();
bool loadFont(const std::string& fontName);

#endif // APP_STATE_H
