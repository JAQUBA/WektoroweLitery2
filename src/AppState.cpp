// ============================================================================
// AppState.cpp — Global state definitions, settings, shared actions
// ============================================================================
#include "AppState.h"
#include "AppUI.h"
#include "Document.h"
#include "DocumentParser.h"
#include "GCodeEngine.h"
#include "CanvasWindow.h"
#include "LffFont.h"

#include <Core.h>
#include <UI/SimpleWindow/SimpleWindow.h>
#include <UI/Label/Label.h>
#include <UI/InputField/InputField.h>
#include <UI/LogWindow/LogWindow.h>
#include <Util/ConfigManager.h>
#include <Util/StringUtils.h>
#include <commdlg.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include <stdexcept>

// ============================================================================
// Global variable definitions
// ============================================================================
SimpleWindow*  window       = nullptr;
Label*         lblStatus    = nullptr;
Label*         lblInfo      = nullptr;
LogWindow*     logWindow    = nullptr;
HWND           hEditor      = nullptr;

InputField*    fldMaterial  = nullptr;
InputField*    fldDepth     = nullptr;
InputField*    fldSafeH     = nullptr;

ConfigManager  config("config.ini");
Document*      currentDocument = nullptr;
LffFont*       activeFont = nullptr;
std::string    activeFontName = "standard";

std::string    fontsDirectory = "";
std::string    currentFilePath = "";
std::string    lastInputDir  = "";
std::string    lastOutputFile = "";
std::string    lastOutputDir = "";
std::string    exportDiameter = "0,30";
std::string    exportStepover = "0,15";
std::string    exportMaterialThickness = "1,50";
std::string    exportTextDepth = "0,20";
std::string    exportSafeHeight = "2,00";
bool           gridVisible   = true;

double         workspaceWidth  = 300.0;
double         workspaceHeight = 200.0;
int            editorWidth     = 345;

std::vector<ToolPreset> toolPresets;
int            activeToolIndex = 0;

static bool fileExists(const std::string& path) {
    DWORD attrs = GetFileAttributesA(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

static bool tryParseDouble(const std::string& src, double& out) {
    std::string tmp = src;
    tmp.erase(std::remove_if(tmp.begin(), tmp.end(), [](unsigned char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }), tmp.end());
    std::replace(tmp.begin(), tmp.end(), ',', '.');
    if (tmp.empty()) return false;
    try {
        out = std::stod(tmp);
        return true;
    } catch (...) {
        return false;
    }
}

static std::string getFontsDirectory() {
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    std::wstring dir(exePath);
    size_t pos = dir.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
        dir = dir.substr(0, pos);

    std::string exeDir = StringUtils::wideToUtf8(dir);
    std::vector<std::string> candidates = {
        exeDir + "\\resources\\fonts\\",
        exeDir + "\\..\\resources\\fonts\\",
        exeDir + "\\..\\..\\resources\\fonts\\",
        exeDir + "\\..\\..\\..\\resources\\fonts\\",
        "resources\\fonts\\"
    };

    for (const auto& candidate : candidates) {
        if (fileExists(candidate + "standard.lff")) {
            return candidate;
        }
    }

    // Keep deterministic fallback even when files are missing.
    return exeDir + "\\resources\\fonts\\";
}

// External canvas (defined in main.cpp)
extern VectorCanvas* canvas;

// ============================================================================
// Logging
// ============================================================================
void logMsg(const wchar_t* msg) {
    if (logWindow && logWindow->isOpen())
        logWindow->appendMessage(msg);
}

void logMsg(const std::wstring& msg) {
    logMsg(msg.c_str());
}

// ============================================================================
// Settings load/save
// ============================================================================
static std::string intToStr(int n) {
    char buf[16];
    _snprintf(buf, 16, "%d", n);
    return std::string(buf);
}

void loadSettings() {
    fontsDirectory = getFontsDirectory();
    activeFontName = config.getValue("font_name", "standard");
    loadFont(activeFontName);

    currentFilePath = config.getValue("last_input_file", "");
    lastInputDir  = config.getValue("last_input_dir", "");
    lastOutputFile = config.getValue("last_output_file", "");
    lastOutputDir = config.getValue("last_output_dir", "");
    gridVisible   = config.getValue("grid_visible", "1") == "1";

    // Load tool presets
    int toolCount = 0;
    try { toolCount = std::stoi(config.getValue("tool_count", "0")); } catch (...) {}
    toolPresets.clear();
    if (toolCount > 0) {
        for (int i = 0; i < toolCount; i++) {
            std::string p = "tool_" + intToStr(i) + "_";
            ToolPreset tp;
            tp.name = config.getValue(p + "name", "Tool " + intToStr(i));
            tp.diameter = config.getValue(p + "diameter", "0,30");
            tp.stepover = config.getValue(p + "stepover", "0,15");
            toolPresets.push_back(tp);
        }
    } else {
        // Default presets
        toolPresets.push_back({"V-bit 0.3mm", "0,30", "0,15"});
        toolPresets.push_back({"V-bit 0.2mm", "0,20", "0,10"});
        toolPresets.push_back({"Laser 0.1mm", "0,10", "0,05"});
    }
    try { activeToolIndex = std::stoi(config.getValue("active_tool", "0")); } catch (...) { activeToolIndex = 0; }
    if (activeToolIndex < 0 || activeToolIndex >= (int)toolPresets.size())
        activeToolIndex = 0;
    applyActiveToolPreset();

    // Load material parameters (independent of tool presets)
    exportMaterialThickness = config.getValue("export_material_thickness", "1,50");
    exportTextDepth = config.getValue("export_text_depth", "0,20");
    exportSafeHeight = config.getValue("export_safe_height", "2,00");

    double w = 300.0, h = 200.0;
    if (tryParseDouble(config.getValue("workspace_width", "300"), w) && w > 0)
        workspaceWidth = w;
    if (tryParseDouble(config.getValue("workspace_height", "200"), h) && h > 0)
        workspaceHeight = h;

    int ew = 345;
    try { ew = std::stoi(config.getValue("editor_width", "345")); } catch (...) {}
    if (ew < 100) ew = 100;
    if (ew > 800) ew = 800;
    editorWidth = ew;
}

void saveSettings() {
    config.setValue("last_input_file", currentFilePath);
    config.setValue("last_input_dir", lastInputDir);
    config.setValue("last_output_file", lastOutputFile);
    config.setValue("last_output_dir", lastOutputDir);
    config.setValue("grid_visible", gridVisible ? "1" : "0");
    config.setValue("font_name", activeFontName);

    // Save tool presets
    config.setValue("tool_count", intToStr((int)toolPresets.size()));
    for (int i = 0; i < (int)toolPresets.size(); i++) {
        std::string p = "tool_" + intToStr(i) + "_";
        config.setValue(p + "name", toolPresets[i].name);
        config.setValue(p + "diameter", toolPresets[i].diameter);
        config.setValue(p + "stepover", toolPresets[i].stepover);
    }
    config.setValue("active_tool", intToStr(activeToolIndex));

    // Save material parameters
    config.setValue("export_material_thickness", exportMaterialThickness);
    config.setValue("export_text_depth", exportTextDepth);
    config.setValue("export_safe_height", exportSafeHeight);

    {
        char buf[64];
        _snprintf(buf, 64, "%.1f", workspaceWidth);
        config.setValue("workspace_width", buf);
        _snprintf(buf, 64, "%.1f", workspaceHeight);
        config.setValue("workspace_height", buf);
    }
    {
        char buf[16];
        _snprintf(buf, 16, "%d", editorWidth);
        config.setValue("editor_width", buf);
    }
}

// ============================================================================
// File dialog helpers
// ============================================================================
std::string extractDir(const std::string& filePath) {
    size_t pos = filePath.find_last_of("\\/");
    if (pos != std::string::npos)
        return filePath.substr(0, pos);
    return "";
}

std::string openFileDialog(HWND owner, const wchar_t* filter, const wchar_t* title,
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

std::string saveFileDialog(HWND owner, const wchar_t* filter, const wchar_t* title,
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

// ============================================================================
// Editor helpers
// ============================================================================
std::string getEditorText() {
    if (!hEditor) return "";
    int len = GetWindowTextLengthW(hEditor);
    if (len <= 0) return "";
    std::wstring buf(len + 1, L'\0');
    GetWindowTextW(hEditor, &buf[0], len + 1);
    buf.resize(len);
    std::string result = StringUtils::wideToUtf8(buf);
    // Normalize line endings (RichEdit may use \r only)
    size_t pos = 0;
    while ((pos = result.find('\r', pos)) != std::string::npos) {
        if (pos + 1 < result.size() && result[pos + 1] == '\n') {
            result.erase(pos, 1);
        } else {
            result[pos] = '\n';
        }
    }
    return result;
}

void setEditorText(const std::string& text) {
    if (!hEditor) return;
    std::wstring wText = StringUtils::utf8ToWide(text);
    SetWindowTextW(hEditor, wText.c_str());
}

void updateWindowTitle() {
    std::wstring title = L"Vector Letters 2";
    if (!currentFilePath.empty()) {
        size_t pos = currentFilePath.find_last_of("\\/");
        std::string fname = (pos != std::string::npos)
            ? currentFilePath.substr(pos + 1) : currentFilePath;
        title = StringUtils::utf8ToWide(fname) + L" \u2014 " + title;
    } else {
        title = L"Untitled \u2014 " + title;
    }
    if (window) SetWindowTextW(window->getHandle(), title.c_str());
}

// ============================================================================
// Shared actions
// ============================================================================
void doNewFile() {
    setEditorTextUI("");
    currentFilePath = "";
    updateWindowTitle();
    doRenderPreview();
    if (canvas) canvas->fitToContent();
    logMsg(L"New document");
}

void doOpenFile() {
    std::string path = openFileDialog(window->getHandle(),
        L"Layout files (*.txt)\0*.txt\0All files (*.*)\0*.*\0",
        L"Open layout file", lastInputDir);
    if (path.empty()) return;

    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        logMsg(L"Cannot open file");
        return;
    }
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    f.close();

    setEditorTextUI(content);
    currentFilePath = path;
    lastInputDir = extractDir(path);
    updateWindowTitle();
    doRenderPreview();
    if (canvas) canvas->fitToContent();
    logMsg(L"Opened: " + StringUtils::utf8ToWide(path));
}

void doSaveFile() {
    if (currentFilePath.empty()) {
        doSaveFileAs();
        return;
    }
    std::string content = getEditorText();
    std::ofstream f(currentFilePath, std::ios::binary);
    if (!f.is_open()) {
        logMsg(L"Cannot write file");
        return;
    }
    f.write(content.data(), content.size());
    f.close();
    logMsg(L"Saved: " + StringUtils::utf8ToWide(currentFilePath));
}

void doSaveFileAs() {
    std::string path = saveFileDialog(window->getHandle(),
        L"Layout files (*.txt)\0*.txt\0All files (*.*)\0*.*\0",
        L"Save layout file as", L"txt", lastInputDir);
    if (path.empty()) return;

    currentFilePath = path;
    lastInputDir = extractDir(path);
    doSaveFile();
    updateWindowTitle();
}

void doRenderPreview() {
    std::string content = getEditorText();
    if (content.empty()) {
        if (currentDocument) {
            delete currentDocument;
            currentDocument = nullptr;
        }
        if (canvas) {
            canvas->setDocument(nullptr);
        }
        return;
    }
    if (!activeFont) return;

    double diam = 0.0, step = 0.0;
    if (!tryParseDouble(exportDiameter, diam) ||
        !tryParseDouble(exportStepover, step) ||
        diam <= 0 || step <= 0) {
        return;
    }

    if (currentDocument) {
        delete currentDocument;
        currentDocument = nullptr;
    }

    std::vector<int> errorLines;
    Document doc = DocumentParser::parseString(content, *activeFont, diam, step, &errorLines);
    currentDocument = new Document(doc);

    highlightEditorErrors(errorLines);

    if (canvas) {
        canvas->setDocument(currentDocument);
    }
}

void doExportGCode() {
    std::string content = getEditorText();
    if (content.empty()) {
        logMsg(L"Editor is empty");
        return;
    }

    // Auto-show save dialog if no output file is set
    if (lastOutputFile.empty()) {
        std::string path = saveFileDialog(window->getHandle(),
            L"G-Code files (*.gcode)\0*.gcode\0All files (*.*)\0*.*\0",
            L"Export G-Code file", L"gcode", lastOutputDir);
        if (path.empty()) return;
        lastOutputFile = path;
        lastOutputDir = extractDir(path);
    }

    if (!activeFont) {
        logMsg(L"No font loaded");
        return;
    }

    double diam = 0.0, step = 0.0, matThick = 0.0, textDep = 0.0, safeH = 0.0;

    // Log raw UI parameter strings for debugging
    {
        wchar_t dbg[512];
        std::wstring wMat(exportMaterialThickness.begin(), exportMaterialThickness.end());
        std::wstring wDep(exportTextDepth.begin(), exportTextDepth.end());
        std::wstring wSafe(exportSafeHeight.begin(), exportSafeHeight.end());
        _snwprintf(dbg, 512, L"Export params: material='%s'  depth='%s'  safe='%s'",
                   wMat.c_str(), wDep.c_str(), wSafe.c_str());
        logMsg(dbg);
    }
    if (!tryParseDouble(exportDiameter, diam) ||
        !tryParseDouble(exportStepover, step) ||
        !tryParseDouble(exportMaterialThickness, matThick) ||
        !tryParseDouble(exportTextDepth, textDep) ||
        !tryParseDouble(exportSafeHeight, safeH)) {
        logMsg(L"Invalid parameter values. Use positive numbers (e.g. 0,30 / 0,15 / 1,50 / 0,20 / 5,00)");
        return;
    }
    if (diam <= 0 || step <= 0 || matThick <= 0 || textDep <= 0 || safeH <= 0) {
        logMsg(L"All parameters must be positive values");
        return;
    }

    Document exportDoc = DocumentParser::parseString(content, *activeFont, diam, step);
    if (exportDoc.getRows().empty()) {
        logMsg(L"Document parsing failed or returned no rows");
        return;
    }

    exportDoc.materialThickness_mm = matThick;
    exportDoc.textDepth_mm = textDep;
    exportDoc.safeHeight_mm = safeH;

    // Log computed Z levels for verification
    {
        double zText = matThick - textDep;
        double zSafe = matThick + safeH;
        wchar_t zBuf[256];
        _snwprintf(zBuf, 256, L"Z levels: text=%.2f  cut=0.00  safe=%.2f  (material=%.2f  depth=%.2f)",
                   zText, zSafe, matThick, textDep);
        logMsg(zBuf);
    }

    GCodeEngine gce;
    gce.exportDocument(lastOutputFile, exportDoc);

    std::wstring msg = L"G-Code exported to: " + StringUtils::utf8ToWide(lastOutputFile);
    logMsg(msg);
}

void doToggleLogWindow() {
    if (!logWindow) {
        logWindow = new LogWindow();
        logWindow->setTitle(L"Vector Letters — Log");
        logWindow->setFont(L"Consolas", 14);
        logWindow->setTextColor(RGB(170, 180, 195));
        logWindow->setBackColor(RGB(22, 22, 28));
        logWindow->enablePersistence(config, "logwin");
    }

    if (logWindow->isOpen()) {
        logWindow->close();
    } else {
        logWindow->open(window ? window->getHandle() : nullptr);
    }
}

void doToggleGrid() {
    gridVisible = !gridVisible;
    if (canvas) {
        canvas->setGridVisible(gridVisible);
    }
}

// ============================================================================
// Font loading
// ============================================================================
bool loadFont(const std::string& fontName) {
    if (activeFont) {
        delete activeFont;
        activeFont = nullptr;
    }

    std::string path = fontsDirectory + fontName + ".lff";
    activeFont = new LffFont();
    if (!activeFont->load(path)) {
        delete activeFont;
        activeFont = nullptr;
        logMsg(L"Failed to load font: " + StringUtils::utf8ToWide(path));
        return false;
    }
    activeFontName = fontName;
    config.setValue("font_name", fontName);
    return true;
}

// ============================================================================
// Tool presets
// ============================================================================
void applyActiveToolPreset() {
    if (activeToolIndex >= 0 && activeToolIndex < (int)toolPresets.size()) {
        const auto& tp = toolPresets[activeToolIndex];
        exportDiameter = tp.diameter;
        exportStepover = tp.stepover;
    }
}

void doSelectTool(int index) {
    if (index < 0 || index >= (int)toolPresets.size()) return;
    activeToolIndex = index;
    applyActiveToolPreset();
    updateToolButtonText();
    saveSettings();
}

// ============================================================================
// Tool presets management dialog
// ============================================================================
static HWND s_hToolList = nullptr;
static HWND s_hToolName = nullptr;
static HWND s_hToolDia = nullptr;
static HWND s_hToolStep = nullptr;
static int  s_toolDlgSel = -1;

static void toolDlgPopulateFields(int idx) {
    if (idx < 0 || idx >= (int)toolPresets.size()) {
        SetWindowTextW(s_hToolName, L"");
        SetWindowTextW(s_hToolDia, L"");
        SetWindowTextW(s_hToolStep, L"");
        return;
    }
    const auto& tp = toolPresets[idx];
    SetWindowTextW(s_hToolName, StringUtils::utf8ToWide(tp.name).c_str());
    SetWindowTextW(s_hToolDia, StringUtils::utf8ToWide(tp.diameter).c_str());
    SetWindowTextW(s_hToolStep, StringUtils::utf8ToWide(tp.stepover).c_str());
}

static void toolDlgSaveFields(int idx) {
    if (idx < 0 || idx >= (int)toolPresets.size()) return;
    auto& tp = toolPresets[idx];
    wchar_t buf[128];
    GetWindowTextW(s_hToolName, buf, 128); tp.name = StringUtils::wideToUtf8(buf);
    GetWindowTextW(s_hToolDia, buf, 128); tp.diameter = StringUtils::wideToUtf8(buf);
    GetWindowTextW(s_hToolStep, buf, 128); tp.stepover = StringUtils::wideToUtf8(buf);
}

static void toolDlgRefreshList(int selectIdx = -1) {
    SendMessageW(s_hToolList, LB_RESETCONTENT, 0, 0);
    for (const auto& tp : toolPresets) {
        std::wstring name = StringUtils::utf8ToWide(tp.name);
        SendMessageW(s_hToolList, LB_ADDSTRING, 0, (LPARAM)name.c_str());
    }
    if (selectIdx >= 0 && selectIdx < (int)toolPresets.size())
        SendMessageW(s_hToolList, LB_SETCURSEL, selectIdx, 0);
}

static LRESULT CALLBACK ToolPresetsDlgProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            int lbW = 170, lbH = 120;
            int fieldX = 200, fieldW = 220;
            int labelW = 70;

            s_hToolList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
                15, 15, lbW, lbH, hwnd, (HMENU)100, _core.hInstance, NULL);

            auto addField = [&](int row, const wchar_t* label, HWND& hEdit, int id) {
                int y = 15 + row * 32;
                CreateWindowExW(0, L"STATIC", label, WS_CHILD | WS_VISIBLE,
                    fieldX, y + 2, labelW, 20, hwnd, NULL, _core.hInstance, NULL);
                hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                    fieldX + labelW + 5, y, fieldW - labelW - 5, 24,
                    hwnd, (HMENU)(UINT_PTR)id, _core.hInstance, NULL);
            };

            addField(0, L"Name:", s_hToolName, 101);
            addField(1, L"Diameter:", s_hToolDia, 102);
            addField(2, L"Stepover:", s_hToolStep, 103);

            CreateWindowExW(0, L"BUTTON", L"Add", WS_CHILD | WS_VISIBLE,
                15, 135, 80, 28, hwnd, (HMENU)110, _core.hInstance, NULL);
            CreateWindowExW(0, L"BUTTON", L"Remove", WS_CHILD | WS_VISIBLE,
                105, 135, 80, 28, hwnd, (HMENU)111, _core.hInstance, NULL);
            CreateWindowExW(0, L"BUTTON", L"Save", WS_CHILD | WS_VISIBLE,
                fieldX, 15 + 3 * 32, 80, 28, hwnd, (HMENU)112, _core.hInstance, NULL);
            CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                180, 175, 80, 28, hwnd, (HMENU)113, _core.hInstance, NULL);

            toolDlgRefreshList(activeToolIndex);
            s_toolDlgSel = activeToolIndex;
            toolDlgPopulateFields(s_toolDlgSel);
            return 0;
        }
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);

            if (wmId == 100 && wmEvent == LBN_SELCHANGE) {
                if (s_toolDlgSel >= 0) toolDlgSaveFields(s_toolDlgSel);
                s_toolDlgSel = (int)SendMessageW(s_hToolList, LB_GETCURSEL, 0, 0);
                toolDlgPopulateFields(s_toolDlgSel);
                return 0;
            }
            if (wmId == 100 && wmEvent == LBN_DBLCLK) {
                int sel = (int)SendMessageW(s_hToolList, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel < (int)toolPresets.size()) {
                    toolDlgSaveFields(sel);
                    activeToolIndex = sel;
                    applyActiveToolPreset();
                    updateToolButtonText();
                    saveSettings();
                }
                return 0;
            }

            switch (wmId) {
                case 110: { // Add
                    if (s_toolDlgSel >= 0) toolDlgSaveFields(s_toolDlgSel);
                    ToolPreset newTp;
                    newTp.name = "New Tool";
                    newTp.diameter = "0,30";
                    newTp.stepover = "0,15";
                    toolPresets.push_back(newTp);
                    int newIdx = (int)toolPresets.size() - 1;
                    toolDlgRefreshList(newIdx);
                    s_toolDlgSel = newIdx;
                    toolDlgPopulateFields(s_toolDlgSel);
                    break;
                }
                case 111: { // Remove
                    if (s_toolDlgSel >= 0 && (int)toolPresets.size() > 1) {
                        toolPresets.erase(toolPresets.begin() + s_toolDlgSel);
                        if (activeToolIndex >= (int)toolPresets.size())
                            activeToolIndex = (int)toolPresets.size() - 1;
                        if (s_toolDlgSel >= (int)toolPresets.size())
                            s_toolDlgSel = (int)toolPresets.size() - 1;
                        toolDlgRefreshList(s_toolDlgSel);
                        toolDlgPopulateFields(s_toolDlgSel);
                    }
                    break;
                }
                case 112: { // Save
                    if (s_toolDlgSel >= 0) {
                        toolDlgSaveFields(s_toolDlgSel);
                        toolDlgRefreshList(s_toolDlgSel);
                        if (s_toolDlgSel == activeToolIndex) {
                            applyActiveToolPreset();
                            updateToolButtonText();
                        }
                        saveSettings();
                    }
                    break;
                }
                case 113: { // Close
                    if (s_toolDlgSel >= 0) toolDlgSaveFields(s_toolDlgSel);
                    applyActiveToolPreset();
                    updateToolButtonText();
                    saveSettings();
                    DestroyWindow(hwnd);
                    break;
                }
            }
            return 0;
        }
        case WM_DESTROY:
            EnableWindow(window->getHandle(), TRUE);
            SetForegroundWindow(window->getHandle());
            return 0;
        case WM_CLOSE:
            if (s_toolDlgSel >= 0) toolDlgSaveFields(s_toolDlgSel);
            applyActiveToolPreset();
            updateToolButtonText();
            saveSettings();
            DestroyWindow(hwnd);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void doShowToolPresets() {
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = ToolPresetsDlgProc;
        wc.hInstance = _core.hInstance;
        wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = L"WL2_ToolPresetsDlg";
        RegisterClassExW(&wc);
        classRegistered = true;
    }

    EnableWindow(window->getHandle(), FALSE);

    RECT parentRect;
    GetWindowRect(window->getHandle(), &parentRect);
    int cx = (parentRect.left + parentRect.right) / 2 - 225;
    int cy = (parentRect.top + parentRect.bottom) / 2 - 165;

    CreateWindowExW(WS_EX_DLGMODALFRAME,
        L"WL2_ToolPresetsDlg", L"Tool Presets",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        cx, cy, 450, 240,
        window->getHandle(), NULL, _core.hInstance, NULL);
}

// ============================================================================
// Workspace settings dialog
// ============================================================================
static HWND s_hDlgWidth = nullptr;
static HWND s_hDlgHeight = nullptr;

static LRESULT CALLBACK WorkspaceDlgProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            CreateWindowExW(0, L"STATIC", L"Width (mm):", WS_CHILD | WS_VISIBLE,
                            20, 20, 100, 22, hwnd, NULL, _core.hInstance, NULL);
            s_hDlgWidth = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                130, 18, 100, 24, hwnd, NULL, _core.hInstance, NULL);

            CreateWindowExW(0, L"STATIC", L"Height (mm):", WS_CHILD | WS_VISIBLE,
                            20, 55, 100, 22, hwnd, NULL, _core.hInstance, NULL);
            s_hDlgHeight = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                130, 53, 100, 24, hwnd, NULL, _core.hInstance, NULL);

            CreateWindowExW(0, L"BUTTON", L"OK",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                50, 95, 80, 28, hwnd, (HMENU)1, _core.hInstance, NULL);
            CreateWindowExW(0, L"BUTTON", L"Cancel",
                WS_CHILD | WS_VISIBLE,
                150, 95, 80, 28, hwnd, (HMENU)2, _core.hInstance, NULL);

            // Fill current values
            wchar_t buf[64];
            _snwprintf(buf, 64, L"%.1f", workspaceWidth);
            SetWindowTextW(s_hDlgWidth, buf);
            _snwprintf(buf, 64, L"%.1f", workspaceHeight);
            SetWindowTextW(s_hDlgHeight, buf);
            return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) { // OK
                wchar_t buf[64];
                GetWindowTextW(s_hDlgWidth, buf, 64);
                std::string sw = StringUtils::wideToUtf8(buf);
                GetWindowTextW(s_hDlgHeight, buf, 64);
                std::string sh = StringUtils::wideToUtf8(buf);

                double w = 0.0, h = 0.0;
                if (tryParseDouble(sw, w) && tryParseDouble(sh, h) && w > 0 && h > 0) {
                    workspaceWidth = w;
                    workspaceHeight = h;
                    if (canvas) {
                        canvas->setGridExtent(workspaceWidth, workspaceHeight);
                        canvas->redraw();
                    }
                    saveSettings();
                }
                DestroyWindow(hwnd);
            } else if (LOWORD(wParam) == 2) { // Cancel
                DestroyWindow(hwnd);
            }
            return 0;
        case WM_DESTROY:
            EnableWindow(window->getHandle(), TRUE);
            SetForegroundWindow(window->getHandle());
            return 0;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void doShowWorkspaceSettings() {
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = WorkspaceDlgProc;
        wc.hInstance = _core.hInstance;
        wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = L"WL2_WorkspaceDlg";
        RegisterClassExW(&wc);
        classRegistered = true;
    }

    EnableWindow(window->getHandle(), FALSE);

    RECT parentRect;
    GetWindowRect(window->getHandle(), &parentRect);
    int cx = (parentRect.left + parentRect.right) / 2 - 145;
    int cy = (parentRect.top + parentRect.bottom) / 2 - 80;

    CreateWindowExW(WS_EX_DLGMODALFRAME,
        L"WL2_WorkspaceDlg", L"Machine Workspace Size",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        cx, cy, 290, 170,
        window->getHandle(), NULL, _core.hInstance, NULL);
}

// ============================================================================
// Relayout — reposition editor + canvas after splitter move or window resize
// ============================================================================
void doRelayout() {
    if (!window) return;
    HWND hwnd = window->getHandle();
    RECT rc;
    GetClientRect(hwnd, &rc);

    int contentTop = 64;       // below toolbar rows
    int margin = 10;
    int splitterW = 6;
    int contentH = rc.bottom - contentTop - 6;

    if (editorWidth < 100) editorWidth = 100;
    if (editorWidth > rc.right - 200) editorWidth = rc.right - 200;

    if (hEditor) {
        MoveWindow(hEditor, margin, contentTop, editorWidth - margin, contentH, TRUE);
    }
    if (hSplitter) {
        MoveWindow(hSplitter, editorWidth, contentTop, splitterW, contentH, TRUE);
    }
    if (canvas) {
        int canvasX = editorWidth + splitterW;
        int canvasW = rc.right - canvasX - margin;
        if (canvasW < 50) canvasW = 50;
        MoveWindow(canvas->getHandle(), canvasX, contentTop, canvasW, contentH, TRUE);
        canvas->fitToContent();
    }
}
