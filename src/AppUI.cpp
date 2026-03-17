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
#include <Util/StringUtils.h>
#include <commctrl.h>

static void styleBtn(SimpleWindow* win, Button* btn,
                     COLORREF bg, COLORREF text, COLORREF hover) {
    btn->setBackColor(bg);
    btn->setTextColor(text);
    btn->setHoverColor(hover);
    win->add(btn);
}

// --- Dark theme subclass for editor EDIT control ---
static HBRUSH hEditorBgBrush = NULL;

static LRESULT CALLBACK EditorParentSubclassProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR, DWORD_PTR) {
    if (msg == WM_CTLCOLOREDIT) {
        HWND hCtrl = (HWND)lParam;
        if (hCtrl == hEditor) {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, CLR_EDITOR_TEXT);
            SetBkColor(hdc, CLR_EDITOR_BG);
            if (!hEditorBgBrush)
                hEditorBgBrush = CreateSolidBrush(CLR_EDITOR_BG);
            return (LRESULT)hEditorBgBrush;
        }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void createUI(SimpleWindow* win) {
    int y = 4;
    int m = 10;

    // --- Row 1: Action buttons + Tool parameters ---
    styleBtn(win, new Button(m, y, 80, 26, "Run",
        [](Button*) { doRunDocument(); }),
        CLR_ACTION_BG, CLR_ACTION_TEXT, CLR_ACTION_HOVER);

    styleBtn(win, new Button(m + 85, y, 110, 26, "Export GCode",
        [](Button*) { doExportGCode(); }),
        CLR_EXPORT_BG, CLR_EXPORT_TEXT, CLR_EXPORT_HOVER);

    styleBtn(win, new Button(m + 200, y, 90, 26, "Reset View",
        [](Button*) {
            extern VectorCanvas* canvas;
            if (canvas) canvas->resetView();
        }),
        CLR_TOOL_BG, CLR_TOOL_TEXT, CLR_TOOL_HOVER);

    // Tool parameters (compact, same row)
    int px = m + 310;
    auto addParam = [&](const wchar_t* label, int lw, int fw, std::string* varPtr) {
        auto* lbl = new Label(px, y + 3, lw, 20, label);
        win->add(lbl);
        lbl->setFont(L"Segoe UI", 10, false);
        lbl->setTextColor(CLR_LABEL_TEXT);
        lbl->setBackColor(CLR_WIN_BG);
        px += lw;

        auto* field = new InputField(px, y, fw, 24, varPtr->c_str(),
            [varPtr](InputField*, const char* text) { *varPtr = text; });
        field->setMaxLength(32);
        win->add(field);
        px += fw + 5;
    };

    addParam(L"Dia:", 28, 52, &exportDiameter);
    addParam(L"Step:", 32, 52, &exportStepover);
    addParam(L"Mat:", 28, 52, &exportMaterialThickness);
    addParam(L"Dep:", 28, 52, &exportTextDepth);
    addParam(L"Safe:", 32, 52, &exportSafeHeight);

    y += 30;

    // --- Row 2: Output file + Info ---
    auto* lblOutput = new Label(m, y + 3, 50, 20, L"Output:");
    win->add(lblOutput);
    lblOutput->setFont(L"Segoe UI", 10, false);
    lblOutput->setTextColor(CLR_LABEL_TEXT);
    lblOutput->setBackColor(CLR_WIN_BG);

    auto* outputField = new InputField(m + 55, y, 650, 24, lastOutputFile.c_str(),
        [](InputField* f, const char* text) {
            lastOutputFile = text;
        });
    outputField->setMaxLength(512);
    win->add(outputField);

    styleBtn(win, new Button(m + 710, y, 30, 26, "...",
        [outputField](Button*) {
            std::string path = saveFileDialog(window->getHandle(),
                L"G-Code files (*.gcode)\0*.gcode\0All files (*.*)\0*.*\0",
                L"Select output G-Code file", L"gcode", lastOutputDir);
            if (!path.empty()) {
                lastOutputFile = path;
                lastOutputDir = extractDir(path);
                outputField->setText(path.c_str());
            }
        }),
        CLR_TOOL_BG, CLR_TOOL_TEXT, CLR_TOOL_HOVER);

    lblInfo = new Label(m + 760, y + 3, 400, 20, L"");
    win->add(lblInfo);
    lblInfo->setFont(L"Segoe UI", 10, false);
    lblInfo->setTextColor(CLR_INFO_TEXT);
    lblInfo->setBackColor(CLR_WIN_BG);

    y += 30;

    // --- Layout editor (multiline EDIT control) ---
    hEditor = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT", L"",
        WS_CHILD | WS_VISIBLE |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN |
        WS_VSCROLL | WS_HSCROLL,
        m, y, 345, 525,
        win->getHandle(),
        NULL,
        _core.hInstance, NULL);

    // Monospace font for the editor
    HFONT hEditorFont = CreateFontW(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    SendMessageW(hEditor, WM_SETFONT, (WPARAM)hEditorFont, TRUE);

    // Set tab stops (16 dialog units ≈ 4 chars in monospace font)
    DWORD tabStop = 16;
    SendMessageW(hEditor, EM_SETTABSTOPS, 1, (LPARAM)&tabStop);

    // Dark theme for the editor via parent subclass
    SetWindowSubclass(win->getHandle(), EditorParentSubclassProc, 1, 0);
}
