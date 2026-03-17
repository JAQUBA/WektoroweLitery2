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
#include <commdlg.h>

static void styleBtn(SimpleWindow* win, Button* btn,
                     COLORREF bg, COLORREF text, COLORREF hover) {
    btn->setBackColor(bg);
    btn->setTextColor(text);
    btn->setHoverColor(hover);
    win->add(btn);
}

// --- File dialog helpers ---
static std::string openFileDialog(HWND owner, const wchar_t* filter, const wchar_t* title) {
    wchar_t filePath[MAX_PATH] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&ofn))
        return StringUtils::wideToUtf8(filePath);
    return "";
}

static std::string saveFileDialog(HWND owner, const wchar_t* filter, const wchar_t* title,
                                   const wchar_t* defaultExt) {
    wchar_t filePath[MAX_PATH] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    if (GetSaveFileNameW(&ofn))
        return StringUtils::wideToUtf8(filePath);
    return "";
}

void createUI(SimpleWindow* win) {
    int y = 6;
    int m = 10;
    int w = 870;

    // --- Status label ---
    lblStatus = new Label(m, y, w, 18, L"Vector Letters 2 — Ready");
    win->add(lblStatus);
    lblStatus->setFont(L"Segoe UI", 11, false);
    lblStatus->setTextColor(CLR_LABEL_TEXT);
    lblStatus->setBackColor(CLR_WIN_BG);
    y += 24;

    // --- Input file ---
    auto* lblInput = new Label(m, y + 3, 70, 20, L"Input file:");
    win->add(lblInput);
    lblInput->setFont(L"Segoe UI", 10, false);
    lblInput->setTextColor(CLR_LABEL_TEXT);
    lblInput->setBackColor(CLR_WIN_BG);

    auto* inputField = new InputField(m + 75, y, 370, 24, lastInputFile.c_str(),
        [](InputField* f, const char* text) {
            lastInputFile = text;
        });
    inputField->setMaxLength(512);
    win->add(inputField);

    styleBtn(win, new Button(m + 450, y, 30, 26, "...",
        [inputField](Button*) {
            std::string path = openFileDialog(window->getHandle(),
                L"Layout files (*.txt)\0*.txt\0All files (*.*)\0*.*\0",
                L"Select layout file");
            if (!path.empty()) {
                lastInputFile = path;
                inputField->setText(path.c_str());
            }
        }),
        CLR_TOOL_BG, CLR_TOOL_TEXT, CLR_TOOL_HOVER);

    styleBtn(win, new Button(m + 490, y, 90, 26, "Run",
        [](Button*) { doRunDocument(); }),
        CLR_ACTION_BG, CLR_ACTION_TEXT, CLR_ACTION_HOVER);

    styleBtn(win, new Button(m + 590, y, 90, 26, "Export GCode",
        [](Button*) { doExportGCode(); }),
        CLR_EXPORT_BG, CLR_EXPORT_TEXT, CLR_EXPORT_HOVER);

    styleBtn(win, new Button(m + 690, y, 90, 26, "Reset View",
        [](Button*) {
            extern CanvasWindow* canvas;
            if (canvas) canvas->resetView();
        }),
        CLR_TOOL_BG, CLR_TOOL_TEXT, CLR_TOOL_HOVER);

    y += 30;

    // --- Output file ---
    auto* lblOutput = new Label(m, y + 3, 55, 20, L"Output:");
    win->add(lblOutput);
    lblOutput->setFont(L"Segoe UI", 10, false);
    lblOutput->setTextColor(CLR_LABEL_TEXT);
    lblOutput->setBackColor(CLR_WIN_BG);

    auto* outputField = new InputField(m + 75, y, 670, 24, lastOutputFile.c_str(),
        [](InputField* f, const char* text) {
            lastOutputFile = text;
        });
    outputField->setMaxLength(512);
    win->add(outputField);

    styleBtn(win, new Button(m + 750, y, 30, 26, "...",
        [outputField](Button*) {
            std::string path = saveFileDialog(window->getHandle(),
                L"G-Code files (*.gcode)\0*.gcode\0All files (*.*)\0*.*\0",
                L"Select output G-Code file", L"gcode");
            if (!path.empty()) {
                lastOutputFile = path;
                outputField->setText(path.c_str());
            }
        }),
        CLR_TOOL_BG, CLR_TOOL_TEXT, CLR_TOOL_HOVER);

    y += 30;

    // --- Export depth parameters ---
    auto* lblIdle = new Label(m, y + 3, 65, 20, L"Idle Z:");
    win->add(lblIdle);
    lblIdle->setFont(L"Segoe UI", 10, false);
    lblIdle->setTextColor(CLR_LABEL_TEXT);
    lblIdle->setBackColor(CLR_WIN_BG);

    auto* idleField = new InputField(m + 65, y, 80, 24, exportIdleDepth.c_str(),
        [](InputField*, const char* text) {
            exportIdleDepth = text;
        });
    idleField->setMaxLength(32);
    win->add(idleField);

    auto* lblWork = new Label(m + 170, y + 3, 65, 20, L"Work Z:");
    win->add(lblWork);
    lblWork->setFont(L"Segoe UI", 10, false);
    lblWork->setTextColor(CLR_LABEL_TEXT);
    lblWork->setBackColor(CLR_WIN_BG);

    auto* workField = new InputField(m + 235, y, 80, 24, exportWorkDepth.c_str(),
        [](InputField*, const char* text) {
            exportWorkDepth = text;
        });
    workField->setMaxLength(32);
    win->add(workField);

    auto* lblCut = new Label(m + 340, y + 3, 65, 20, L"Cut Z:");
    win->add(lblCut);
    lblCut->setFont(L"Segoe UI", 10, false);
    lblCut->setTextColor(CLR_LABEL_TEXT);
    lblCut->setBackColor(CLR_WIN_BG);

    auto* cutField = new InputField(m + 405, y, 80, 24, exportCutDepth.c_str(),
        [](InputField*, const char* text) {
            exportCutDepth = text;
        });
    cutField->setMaxLength(32);
    win->add(cutField);

    auto* lblDepthHint = new Label(m + 510, y + 3, 330, 20, L"Used only for G-Code export");
    win->add(lblDepthHint);
    lblDepthHint->setFont(L"Segoe UI", 10, false);
    lblDepthHint->setTextColor(CLR_INFO_TEXT);
    lblDepthHint->setBackColor(CLR_WIN_BG);

    y += 30;

    // --- Info label ---
    lblInfo = new Label(m, y, w, 18, L"");
    win->add(lblInfo);
    lblInfo->setFont(L"Segoe UI", 10, false);
    lblInfo->setTextColor(CLR_INFO_TEXT);
    lblInfo->setBackColor(CLR_WIN_BG);
}
