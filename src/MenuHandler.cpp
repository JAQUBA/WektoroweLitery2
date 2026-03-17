// ============================================================================
// MenuHandler.cpp — Menu bar creation and command routing
// ============================================================================
#include "MenuHandler.h"
#include "AppState.h"
#include "CanvasWindow.h"

#include <Core.h>
#include <UI/SimpleWindow/SimpleWindow.h>
#include <UI/MenuBar/MenuBar.h>

extern VectorCanvas* canvas;

void createAppMenu(SimpleWindow* win) {
    MenuBar* menuBar = new MenuBar();

    // --- File ---
    menuBar->addMenu(L"File", [](PopupMenu& m) {
        m.addItem(L"New", []() { doNewFile(); });
        m.addItem(L"Open...", []() { doOpenFile(); });
        m.addItem(L"Save", []() { doSaveFile(); });
        m.addItem(L"Save As...", []() { doSaveFileAs(); });
        m.addSeparator();
        m.addItem(L"Export G-Code", []() { doExportGCode(); });
        m.addSeparator();
        m.addItem(L"Exit", []() {
            saveSettings();
            PostMessageW(window->getHandle(), WM_CLOSE, 0, 0);
        });
    });

    // --- View ---
    menuBar->addMenu(L"View", [](PopupMenu& m) {
        m.addCheckItem(L"Show grid", gridVisible, [](bool) { doToggleGrid(); });
        m.addItem(L"Reset view", []() {
            if (canvas) canvas->resetView();
        });
        m.addSeparator();
        m.addItem(L"Log window", []() { doToggleLogWindow(); });
    });

    // --- Settings ---
    menuBar->addMenu(L"Settings", [](PopupMenu& m) {
        m.addItem(L"Machine workspace size...", []() { doShowWorkspaceSettings(); });
    });

    // --- Help ---
    menuBar->addMenu(L"Help", [](PopupMenu& m) {
        m.addItem(L"About...", []() {
            MessageBoxW(window->getHandle(),
                L"Vector Letters 2\n"
                L"C++ port using JQB_WindowsLib\n\n"
                L"CNC / Laser engraving nameplate generator\n"
                L"with vector font rendering.",
                L"About", MB_OK | MB_ICONINFORMATION);
        });
    });

    menuBar->attachTo(win);
}
