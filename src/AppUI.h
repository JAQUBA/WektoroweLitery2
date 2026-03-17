// ============================================================================
// AppUI.h — User interface creation
// ============================================================================
#ifndef APP_UI_H
#define APP_UI_H

#include <windows.h>
#include <string>

class SimpleWindow;

void createUI(SimpleWindow* window);

// Wrapper to set editor text without triggering auto-render
void setEditorTextUI(const std::string& text);

// Splitter handle (managed by AppUI, repositioned by doRelayout)
extern HWND hSplitter;

#endif // APP_UI_H
