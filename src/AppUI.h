// ============================================================================
// AppUI.h — User interface creation
// ============================================================================
#ifndef APP_UI_H
#define APP_UI_H

#include <windows.h>
#include <string>
#include <vector>

class SimpleWindow;

void createUI(SimpleWindow* window);

// Wrapper to set editor text without triggering auto-render
void setEditorTextUI(const std::string& text);

// Highlight lines with parse errors (red underline) in the editor.
void highlightEditorErrors(const std::vector<int>& errorLines);

// Splitter handle (managed by AppUI, repositioned by doRelayout)
extern HWND hSplitter;

#endif // APP_UI_H
