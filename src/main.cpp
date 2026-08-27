// ============================================================================
// Vector Letters 2 — CNC/Laser vector font nameplate application
// Based on: https://github.com/JAQUBA/JQB_WindowsLib
// ============================================================================

#include <Core.h>
#include <UI/SimpleWindow/SimpleWindow.h>
#include <Util/ConfigManager.h>

#include "AppState.h"
#include "AppUI.h"
#include "MenuHandler.h"
#include "CanvasWindow.h"
#include "theme.h"

#include <UI/LogWindow/LogWindow.h>
#include <Util/StringUtils.h>
#include <fstream>

// ============================================================================
// Canvas window (global — accessed from AppState and AppUI)
// ============================================================================
VectorCanvas* canvas = nullptr;

// ============================================================================
// setup() — Application initialization
// ============================================================================
void setup() {
    loadSettings();

    // --- Main window (wider for side-by-side editor + canvas) ---
    window = new SimpleWindow(1200, 700, "Vector Letters 2", 101);
    window->init();
    window->setBackgroundColor(CLR_WIN_BG);
    window->setTextColor(CLR_TEXT);

    // --- Menu bar ---
    createAppMenu(window);

    // --- Restore window position/size and maximized state (default: maximized) ---
    // Must run after setMenu(), which resizes the window back to its construction size.
    {
        HWND hwnd = window->getHandle();
        if (windowW > 0 && windowH > 0) {
            MoveWindow(hwnd, windowX, windowY, windowW, windowH, FALSE);
        }
        ShowWindow(hwnd, windowMaximized ? SW_MAXIMIZE : SW_SHOWNORMAL);
    }

    // --- UI components (toolbar + editor + splitter) ---
    createUI(window);

    // --- Canvas (positioned by doRelayout) ---
    canvas = new VectorCanvas();
    canvas->create(window->getHandle(), 0, 0, 100, 100);
    canvas->setBackgroundColor(CLR_CANVAS_BG);
    canvas->setGridColor(CLR_GRID_LINE);
    canvas->setGridVisible(gridVisible);
    canvas->setGridExtent(workspaceWidth, workspaceHeight);
    canvas->setRapidMovesVisible(rapidMovesVisible);
    canvas->setHUDVisible(hudVisible);
    canvas->setVectorArrowsVisible(vectorArrowsVisible);

    // Position editor + splitter + canvas
    doRelayout();

    // --- Load last file into editor ---
    if (!currentFilePath.empty()) {
        std::ifstream f(currentFilePath, std::ios::binary);
        if (f.is_open()) {
            std::string content((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
            f.close();
            setEditorTextUI(content);
            savedEditorContent = getEditorText();
        }
    }
    updateWindowTitle();

    // Initial render
    doRenderPreview();
    if (canvas) canvas->fitToContent();

    // --- Save settings on close ---
    window->onClose([]() {
        saveSettings();
        if (logWindow) logWindow->close();
        if (currentDocument) {
            delete currentDocument;
            currentDocument = nullptr;
        }
        if (canvas) {
            delete canvas;
            canvas = nullptr;
        }
    });

    logMsg(L"Vector Letters 2 ready");
}

// ============================================================================
// loop() — Main loop (empty — event-driven via callbacks)
// ============================================================================
void loop() {
}
