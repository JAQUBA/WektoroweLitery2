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
        m.addItem(L"Fit to content (Nameplates)", []() { doFitToContent(); });
        m.addItem(L"Fit to workspace", []() { doFitToWorkspace(); });
        m.addItem(L"Reset view", []() { doResetView(); });
        m.addSeparator();
        m.addCheckItem(L"Show grid", gridVisible, [](bool) { doToggleGrid(); });
        m.addCheckItem(L"Show rapid moves (G0)", rapidMovesVisible, [](bool) { doToggleRapidMoves(); });
        m.addCheckItem(L"Show vector arrows", vectorArrowsVisible, [](bool) { doToggleVectorArrows(); });
        m.addCheckItem(L"Show HUD overlay", hudVisible, [](bool) { doToggleHUD(); });
        m.addSeparator();
        m.addItem(L"Log window", []() { doToggleLogWindow(); });
    });

    // --- Settings ---
    menuBar->addMenu(L"Settings", [](PopupMenu& m) {
        m.addCheckItem(L"Repeat frame cut", repeatFrameCut, [](bool) { doToggleRepeatFrameCut(); });
        m.addCheckItem(L"Rotate generation 90 deg (swap X/Y)", swapGenerationAxes,
                       [](bool) { doToggleGenerationAxes(); });
        m.addSeparator();
        m.addItem(L"Tool presets...", []() { doShowToolPresets(); });
        m.addItem(L"Machine workspace size...", []() { doShowWorkspaceSettings(); });
    });

    // --- Help ---
    menuBar->addMenu(L"Help", [](PopupMenu& m) {
        m.addItem(L"About...", []() {
            MessageBoxW(window->getHandle(),
                L"Vector Letters 2\n"
                L"C++ port using JQB_WindowsLib\n\n"
                L"CNC / Laser engraving nameplate generator\n"
                L"with vector font rendering.\n\n"
                L"Used libraries and licenses:\n"
                L"  JQB_WindowsLib — LGPL-3.0-or-later\n"
                L"  JQB_CAMCommon — LGPL-3.0-or-later\n"
                L"  Clipper2 — Boost Software License 1.0\n\n"
                L"Distribution details: see LICENSE and THIRD_PARTY_NOTICES.md",
                L"About", MB_OK | MB_ICONINFORMATION);
        });
    });

    menuBar->attachTo(win);
}
