// ============================================================================
// AppState.h — Global application state, shared variables, helpers
// ============================================================================
#ifndef APP_STATE_H
#define APP_STATE_H

#include <string>
#include <vector>

// Forward declarations — JQB_WindowsLib classes
class SimpleWindow;
class Label;
class Button;
class ConfigManager;
class LogWindow;

// Forward declarations — application classes
class Document;

// ============================================================================
// UI Components (global — accessible from multiple modules)
// ============================================================================
extern SimpleWindow*   window;
extern Label*          lblStatus;
extern Label*          lblInfo;
extern LogWindow*      logWindow;

// ============================================================================
// Application objects
// ============================================================================
extern ConfigManager   config;
extern Document*       currentDocument;

// ============================================================================
// Application state
// ============================================================================
extern std::string     lastInputFile;
extern std::string     lastOutputFile;
extern bool            gridVisible;

// ============================================================================
// Helper functions
// ============================================================================
void logMsg(const wchar_t* msg);
void logMsg(const std::wstring& msg);
void loadSettings();
void saveSettings();

// Shared actions (used by UI buttons and menu)
void doRunDocument();
void doExportGCode();
void doToggleLogWindow();
void doToggleGrid();

#endif // APP_STATE_H
