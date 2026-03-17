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

// ============================================================================
// Canvas window (global — accessed from AppState and AppUI)
// ============================================================================
VectorCanvas* canvas = nullptr;

// ============================================================================
// setup() — Application initialization
// ============================================================================
void setup() {
    loadSettings();

    // --- Main window ---
    window = new SimpleWindow(900, 650, "Vector Letters 2", 101);
    window->init();
    window->setBackgroundColor(CLR_WIN_BG);
    window->setTextColor(CLR_TEXT);

    // --- Menu bar ---
    createAppMenu(window);

    // --- UI components (toolbar) ---
    createUI(window);

    // --- Canvas (custom GDI child window) ---
    canvas = new VectorCanvas();
    canvas->create(window->getHandle(), 10, 170, 880, 440);
    canvas->setBackgroundColor(CLR_CANVAS_BG);
    canvas->setGridColor(CLR_GRID_LINE);
    canvas->setGridVisible(gridVisible);

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
