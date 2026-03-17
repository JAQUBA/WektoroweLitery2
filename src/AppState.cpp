// ============================================================================
// AppState.cpp — Global state definitions, settings, shared actions
// ============================================================================
#include "AppState.h"
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
#include <vector>
#include <algorithm>

// ============================================================================
// Global variable definitions
// ============================================================================
SimpleWindow*  window       = nullptr;
Label*         lblStatus    = nullptr;
Label*         lblInfo      = nullptr;
LogWindow*     logWindow    = nullptr;

ConfigManager  config("wektorowe_litery.ini");
Document*      currentDocument = nullptr;

std::string    csvDirectory  = "";
std::string    lastInputFile = "";
std::string    lastOutputFile = "";
std::string    exportIdleDepth = "0,10";
std::string    exportWorkDepth = "-0,10";
std::string    exportCutDepth = "-1,45";
bool           gridVisible   = true;

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
extern CanvasWindow* canvas;

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
    lastInputFile = config.getValue("last_input_file", "");
    lastOutputFile = config.getValue("last_output_file", "");
    exportIdleDepth = config.getValue("export_idle_depth", "0,10");
    exportWorkDepth = config.getValue("export_work_depth", "-0,10");
    exportCutDepth = config.getValue("export_cut_depth", "-1,45");
    gridVisible   = config.getValue("grid_visible", "1") == "1";
}

void saveSettings() {
    config.setValue("last_input_file", lastInputFile);
    config.setValue("last_output_file", lastOutputFile);
    config.setValue("export_idle_depth", exportIdleDepth);
    config.setValue("export_work_depth", exportWorkDepth);
    config.setValue("export_cut_depth", exportCutDepth);
    config.setValue("grid_visible", gridVisible ? "1" : "0");
}

// ============================================================================
// Shared actions
// ============================================================================
void doRunDocument() {
    if (lastInputFile.empty()) {
        logMsg(L"No input file selected");
        return;
    }
    if (csvDirectory.empty()) {
        logMsg(L"Fonts directory not set");
        return;
    }

    if (!fileExists(csvDirectory + "65.csv")) {
        std::wstring msg = L"Font CSV files not found in: " + std::wstring(csvDirectory.begin(), csvDirectory.end());
        logMsg(msg);
        return;
    }

    logMsg(L"Parsing document...");

    if (currentDocument) {
        delete currentDocument;
        currentDocument = nullptr;
    }

    Document doc = DocumentParser::parseFile(lastInputFile, csvDirectory);
    currentDocument = new Document(doc);

    if (canvas) {
        canvas->setDocument(currentDocument);
        canvas->redraw();
    }

    if (lblInfo) {
        std::wstring info = L"Loaded: " + std::wstring(lastInputFile.begin(), lastInputFile.end());
        lblInfo->setText(info.c_str());
    }

    logMsg(L"Document parsed and rendered");
}

void doExportGCode() {
    if (!currentDocument) {
        logMsg(L"No document to export");
        return;
    }
    if (lastOutputFile.empty()) {
        logMsg(L"No output file specified");
        return;
    }

    double idleDepth = 0.0, workDepth = 0.0, cutDepth = 0.0;
    if (!tryParseDouble(exportIdleDepth, idleDepth) ||
        !tryParseDouble(exportWorkDepth, workDepth) ||
        !tryParseDouble(exportCutDepth, cutDepth)) {
        logMsg(L"Invalid depth values. Use numeric values (e.g. 0,10 / -0,10 / -1,45)");
        return;
    }

    Document exportDoc = *currentDocument;
    exportDoc.idleDepth_mm = idleDepth;
    exportDoc.workingDepth_mm = workDepth;
    exportDoc.cuttingDepth_mm = cutDepth;

    GCodeEngine gce;
    gce.exportDocument(lastOutputFile, exportDoc);

    std::wstring msg = L"G-Code exported to: " + std::wstring(lastOutputFile.begin(), lastOutputFile.end());
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
