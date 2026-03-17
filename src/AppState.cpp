// ============================================================================
// AppState.cpp — Global state definitions, settings, shared actions
// ============================================================================
#include "AppState.h"
#include "AppUI.h"
#include "Document.h"
#include "DocumentParser.h"
#include "GCodeEngine.h"
#include "CanvasWindow.h"

#include <Core.h>
#include <UI/SimpleWindow/SimpleWindow.h>
#include <UI/Label/Label.h>
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

ConfigManager  config("config.ini");
Document*      currentDocument = nullptr;

std::string    csvDirectory  = "";
std::string    currentFilePath = "";
std::string    lastInputDir  = "";
std::string    lastOutputFile = "";
std::string    lastOutputDir = "";
std::string    exportDiameter = "0,30";
std::string    exportStepover = "0,15";
std::string    exportMaterialThickness = "1,50";
std::string    exportTextDepth = "0,20";
std::string    exportSafeHeight = "5,00";
bool           gridVisible   = true;

double         workspaceWidth  = 300.0;
double         workspaceHeight = 200.0;
int            editorWidth     = 345;

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
        if (fileExists(candidate + "65.csv")) {
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
void loadSettings() {
    csvDirectory  = getFontsDirectory();
    currentFilePath = config.getValue("last_input_file", "");
    lastInputDir  = config.getValue("last_input_dir", "");
    lastOutputFile = config.getValue("last_output_file", "");
    lastOutputDir = config.getValue("last_output_dir", "");
    exportDiameter = config.getValue("export_diameter", "0,30");
    exportStepover = config.getValue("export_stepover", "0,15");
    exportMaterialThickness = config.getValue("export_material_thickness", "1,50");
    exportTextDepth = config.getValue("export_text_depth", "0,20");
    exportSafeHeight = config.getValue("export_safe_height", "5,00");
    gridVisible   = config.getValue("grid_visible", "1") == "1";

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
    config.setValue("export_diameter", exportDiameter);
    config.setValue("export_stepover", exportStepover);
    config.setValue("export_material_thickness", exportMaterialThickness);
    config.setValue("export_text_depth", exportTextDepth);
    config.setValue("export_safe_height", exportSafeHeight);
    config.setValue("grid_visible", gridVisible ? "1" : "0");

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
    return StringUtils::wideToUtf8(buf);
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
            canvas->redraw();
        }
        return;
    }
    if (csvDirectory.empty()) return;
    if (!fileExists(csvDirectory + "65.csv")) return;

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

    try {
        Document doc = DocumentParser::parseString(content, csvDirectory, diam, step);
        currentDocument = new Document(doc);
    } catch (const std::exception&) {
        // Syntax error in editor content — skip rendering
        return;
    }

    if (canvas) {
        canvas->setDocument(currentDocument);
        canvas->redraw();
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

    if (csvDirectory.empty()) {
        logMsg(L"Fonts directory not set");
        return;
    }

    if (!fileExists(csvDirectory + "65.csv")) {
        std::wstring msg = L"Font CSV files not found in: " + StringUtils::utf8ToWide(csvDirectory);
        logMsg(msg);
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

    Document exportDoc = DocumentParser::parseString(content, csvDirectory, diam, step);
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
    }
}
