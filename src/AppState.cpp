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
bool           gridVisible   = true;

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
    csvDirectory  = config.getValue("csv_directory", "");
    lastInputFile = config.getValue("last_input_file", "");
    lastOutputFile = config.getValue("last_output_file", "");
    gridVisible   = config.getValue("grid_visible", "1") == "1";
}

void saveSettings() {
    config.setValue("csv_directory", csvDirectory);
    config.setValue("last_input_file", lastInputFile);
    config.setValue("last_output_file", lastOutputFile);
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
        logMsg(L"CSV directory not set");
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

    GCodeEngine gce;
    gce.exportDocument(lastOutputFile, *currentDocument);

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
