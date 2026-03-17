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
static std::string extractDir(const std::string& filePath) {
    size_t pos = filePath.find_last_of("\\/");
    if (pos != std::string::npos)
        return filePath.substr(0, pos);
    return "";
}

static std::string openFileDialog(HWND owner, const wchar_t* filter, const wchar_t* title,
                                   const std::string& initialDir) {
    wchar_t filePath[MAX_PATH] = {};
    std::wstring wInitDir = StringUtils::utf8ToWide(initialDir);
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!wInitDir.empty())
        ofn.lpstrInitialDir = wInitDir.c_str();
    if (GetOpenFileNameW(&ofn))
        return StringUtils::wideToUtf8(filePath);
    return "";
}

static std::string saveFileDialog(HWND owner, const wchar_t* filter, const wchar_t* title,
                                   const wchar_t* defaultExt, const std::string& initialDir) {
    wchar_t filePath[MAX_PATH] = {};
    std::wstring wInitDir = StringUtils::utf8ToWide(initialDir);
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    if (!wInitDir.empty())
        ofn.lpstrInitialDir = wInitDir.c_str();
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
                L"Select layout file", lastInputDir);
            if (!path.empty()) {
                lastInputFile = path;
                lastInputDir = extractDir(path);
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
                L"Select output G-Code file", L"gcode", lastOutputDir);
            if (!path.empty()) {
                lastOutputFile = path;
                lastOutputDir = extractDir(path);
                outputField->setText(path.c_str());
            }
        }),
        CLR_TOOL_BG, CLR_TOOL_TEXT, CLR_TOOL_HOVER);

    y += 30;

    // --- Tool parameters ---
    auto* lblDia = new Label(m, y + 3, 65, 20, L"Dia:");
    win->add(lblDia);
    lblDia->setFont(L"Segoe UI", 10, false);
    lblDia->setTextColor(CLR_LABEL_TEXT);
    lblDia->setBackColor(CLR_WIN_BG);

    auto* diaField = new InputField(m + 45, y, 80, 24, exportDiameter.c_str(),
        [](InputField*, const char* text) {
            exportDiameter = text;
        });
    diaField->setMaxLength(32);
    win->add(diaField);

    auto* lblStep = new Label(m + 145, y + 3, 70, 20, L"Stepover:");
    win->add(lblStep);
    lblStep->setFont(L"Segoe UI", 10, false);
    lblStep->setTextColor(CLR_LABEL_TEXT);
    lblStep->setBackColor(CLR_WIN_BG);

    auto* stepField = new InputField(m + 215, y, 80, 24, exportStepover.c_str(),
        [](InputField*, const char* text) {
            exportStepover = text;
        });
    stepField->setMaxLength(32);
    win->add(stepField);

    auto* lblMat = new Label(m + 315, y + 3, 75, 20, L"Material:");
    win->add(lblMat);
    lblMat->setFont(L"Segoe UI", 10, false);
    lblMat->setTextColor(CLR_LABEL_TEXT);
    lblMat->setBackColor(CLR_WIN_BG);

    auto* matField = new InputField(m + 390, y, 80, 24, exportMaterialThickness.c_str(),
        [](InputField*, const char* text) {
            exportMaterialThickness = text;
        });
    matField->setMaxLength(32);
    win->add(matField);

    auto* lblTopHint = new Label(m + 490, y + 3, 320, 20, L"Dia/step/material [mm]");
    win->add(lblTopHint);
    lblTopHint->setFont(L"Segoe UI", 10, false);
    lblTopHint->setTextColor(CLR_INFO_TEXT);
    lblTopHint->setBackColor(CLR_WIN_BG);

    y += 30;

    // --- Z parameters ---
    auto* lblTextD = new Label(m, y + 3, 80, 20, L"Text depth:");
    win->add(lblTextD);
    lblTextD->setFont(L"Segoe UI", 10, false);
    lblTextD->setTextColor(CLR_LABEL_TEXT);
    lblTextD->setBackColor(CLR_WIN_BG);

    auto* textDField = new InputField(m + 80, y, 80, 24, exportTextDepth.c_str(),
        [](InputField*, const char* text) {
            exportTextDepth = text;
        });
    textDField->setMaxLength(32);
    win->add(textDField);

    auto* lblSafe = new Label(m + 180, y + 3, 60, 20, L"Safe Z:");
    win->add(lblSafe);
    lblSafe->setFont(L"Segoe UI", 10, false);
    lblSafe->setTextColor(CLR_LABEL_TEXT);
    lblSafe->setBackColor(CLR_WIN_BG);

    auto* safeField = new InputField(m + 240, y, 80, 24, exportSafeHeight.c_str(),
        [](InputField*, const char* text) {
            exportSafeHeight = text;
        });
    safeField->setMaxLength(32);
    win->add(safeField);

    auto* lblDepthHint = new Label(m + 340, y + 3, 500, 20,
        L"All values positive [mm] | Z0.0 = bottom of material");
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
