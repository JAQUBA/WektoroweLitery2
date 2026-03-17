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

ConfigManager  config("config.ini");
Document*      currentDocument = nullptr;

std::string    csvDirectory  = "";
std::string    lastInputFile = "";
std::string    lastOutputFile = "";
std::string    lastInputDir  = "";
std::string    lastOutputDir = "";
std::string    exportDiameter = "0,30";
std::string    exportStepover = "0,15";
std::string    exportMaterialThickness = "1,50";
std::string    exportTextDepth = "0,20";
std::string    exportSafeHeight = "5,00";
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
    lastInputFile = config.getValue("last_input_file", "");
    lastOutputFile = config.getValue("last_output_file", "");
    lastInputDir  = config.getValue("last_input_dir", "");
    lastOutputDir = config.getValue("last_output_dir", "");
    exportDiameter = config.getValue("export_diameter", "0,30");
    exportStepover = config.getValue("export_stepover", "0,15");
    exportMaterialThickness = config.getValue("export_material_thickness", "1,50");
    exportTextDepth = config.getValue("export_text_depth", "0,20");
    exportSafeHeight = config.getValue("export_safe_height", "5,00");
    gridVisible   = config.getValue("grid_visible", "1") == "1";
}

void saveSettings() {
    config.setValue("last_input_file", lastInputFile);
    config.setValue("last_output_file", lastOutputFile);
    config.setValue("last_input_dir", lastInputDir);
    config.setValue("last_output_dir", lastOutputDir);
    config.setValue("export_diameter", exportDiameter);
    config.setValue("export_stepover", exportStepover);
    config.setValue("export_material_thickness", exportMaterialThickness);
    config.setValue("export_text_depth", exportTextDepth);
    config.setValue("export_safe_height", exportSafeHeight);
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

    // Parse UI tool parameters
    double diam = 0.0, step = 0.0;
    if (!tryParseDouble(exportDiameter, diam) ||
        !tryParseDouble(exportStepover, step) ||
        diam <= 0 || step <= 0) {
        logMsg(L"Invalid tool parameters. Use positive values (e.g. 0,30 / 0,15)");
        return;
    }

    if (currentDocument) {
        delete currentDocument;
        currentDocument = nullptr;
    }

    Document doc = DocumentParser::parseFile(lastInputFile, csvDirectory, diam, step);
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
    if (lastInputFile.empty()) {
        logMsg(L"No input file selected");
        return;
    }
    if (lastOutputFile.empty()) {
        logMsg(L"No output file specified");
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

    // Reparse with current UI parameters so export always uses program config.
    Document exportDoc = DocumentParser::parseFile(lastInputFile, csvDirectory, diam, step);
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
