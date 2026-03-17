// ============================================================================
// AppUI.cpp — User interface components creation
// ============================================================================
#include "AppUI.h"
#include "AppState.h"
#include "CanvasWindow.h"
#include "theme.h"

#include <Core.h>
#include <UI/SimpleWindow/SimpleWindow.h>
#include <UI/Label/Label.h>
#include <UI/Button/Button.h>
#include <UI/InputField/InputField.h>

static void styleBtn(SimpleWindow* win, Button* btn,
                     COLORREF bg, COLORREF text, COLORREF hover) {
    btn->setBackColor(bg);
    btn->setTextColor(text);
    btn->setHoverColor(hover);
    win->add(btn);
}

void createUI(SimpleWindow* win) {
    win->setBackgroundColor(CLR_WIN_BG);

    int y = 6;
    int m = 10;
    int w = 780;

    // --- Status label ---
    lblStatus = new Label(m, y, w, 18, L"Vector Letters 2 — Ready");
    win->add(lblStatus);
    lblStatus->setFont(L"Segoe UI", 11, false);
    lblStatus->setTextColor(CLR_INFO_TEXT);
    y += 24;

    // --- File paths ---
    auto* lblInput = new Label(m, y + 3, 70, 20, L"Input file:");
    win->add(lblInput);
    lblInput->setFont(L"Segoe UI", 10, false);
    lblInput->setTextColor(CLR_INFO_TEXT);

    auto* inputField = new InputField(m + 75, y, 400, 24, "",
        [](InputField* f, const char* text) {
            lastInputFile = text;
        });
    inputField->setMaxLength(512);
    win->add(inputField);

    styleBtn(win, new Button(m + 485, y, 90, 26, "Run",
        [](Button*) { doRunDocument(); }),
        CLR_ACTION_BG, CLR_ACTION_TEXT, CLR_ACTION_HOVER);

    styleBtn(win, new Button(m + 585, y, 90, 26, "Export NC",
        [](Button*) { doExportGCode(); }),
        CLR_EXPORT_BG, CLR_EXPORT_TEXT, CLR_EXPORT_HOVER);

    styleBtn(win, new Button(m + 685, y, 90, 26, "Reset View",
        [](Button*) {
            extern CanvasWindow* canvas;
            if (canvas) canvas->resetView();
        }),
        CLR_TOOL_BG, CLR_TOOL_TEXT, CLR_TOOL_HOVER);

    y += 30;

    // --- CSV directory ---
    auto* lblCsv = new Label(m, y + 3, 70, 20, L"CSV dir:");
    win->add(lblCsv);
    lblCsv->setFont(L"Segoe UI", 10, false);
    lblCsv->setTextColor(CLR_INFO_TEXT);

    auto* csvField = new InputField(m + 75, y, 400, 24, "",
        [](InputField* f, const char* text) {
            csvDirectory = text;
        });
    csvField->setMaxLength(512);
    win->add(csvField);

    // --- Output file ---
    auto* lblOutput = new Label(m + 485, y + 3, 70, 20, L"Output:");
    win->add(lblOutput);
    lblOutput->setFont(L"Segoe UI", 10, false);
    lblOutput->setTextColor(CLR_INFO_TEXT);

    auto* outputField = new InputField(m + 555, y, 220, 24, "",
        [](InputField* f, const char* text) {
            lastOutputFile = text;
        });
    outputField->setMaxLength(512);
    win->add(outputField);

    y += 30;

    // --- Info label ---
    lblInfo = new Label(m, y, w, 18, L"");
    win->add(lblInfo);
    lblInfo->setFont(L"Segoe UI", 10, false);
    lblInfo->setTextColor(CLR_INFO_TEXT);
}
