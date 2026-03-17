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
#include <richedit.h>

static void styleBtn(SimpleWindow* win, Button* btn,
                     COLORREF bg, COLORREF text, COLORREF hover) {
    btn->setBackColor(bg);
    btn->setTextColor(text);
    btn->setHoverColor(hover);
    win->add(btn);
}

// --- Dark theme for editor — no longer needed (RichEdit uses EM_SETBKGNDCOLOR) ---

// --- Splitter drag state ---
HWND hSplitter = nullptr;
static bool splitterDragging = false;
static int  splitterDragStartX = 0;
static int  splitterDragStartEditorW = 0;

// --- Debounced auto-render ---
static const UINT_PTR RENDER_TIMER_ID = 42;
static const UINT RENDER_DELAY_MS = 300;
static bool editorChangeIgnore = false;  // suppress EN_CHANGE during setEditorText

static LRESULT CALLBACK EditorParentSubclassProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR, DWORD_PTR) {
    // Editor auto-render on text change (debounced)
    if (msg == WM_COMMAND && HIWORD(wParam) == EN_CHANGE) {
        HWND hCtrl = (HWND)lParam;
        if (hCtrl == hEditor && !editorChangeIgnore) {
            KillTimer(hwnd, RENDER_TIMER_ID);
            SetTimer(hwnd, RENDER_TIMER_ID, RENDER_DELAY_MS, NULL);
        }
    }

    if (msg == WM_TIMER && wParam == RENDER_TIMER_ID) {
        KillTimer(hwnd, RENDER_TIMER_ID);
        doRenderPreview();
        return 0;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

// --- Splitter window procedure ---
static HBRUSH hSplitterBrush = NULL;
static HBRUSH hSplitterBrushHover = NULL;
static bool   splitterHovered = false;

static LRESULT CALLBACK SplitterProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            if (!hSplitterBrush) hSplitterBrush = CreateSolidBrush(CLR_SPLITTER);
            if (!hSplitterBrushHover) hSplitterBrushHover = CreateSolidBrush(CLR_SPLITTER_HOVER);
            FillRect(hdc, &rc, (splitterDragging || splitterHovered)
                     ? hSplitterBrushHover : hSplitterBrush);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_MOUSEMOVE:
            if (!splitterHovered) {
                splitterHovered = true;
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
                TrackMouseEvent(&tme);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            if (splitterDragging) {
                POINT pt;
                GetCursorPos(&pt);
                int delta = pt.x - splitterDragStartX;
                editorWidth = splitterDragStartEditorW + delta;
                doRelayout();
            }
            return 0;
        case WM_MOUSELEAVE:
            splitterHovered = false;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        case WM_LBUTTONDOWN:
            splitterDragging = true;
            splitterDragStartEditorW = editorWidth;
            POINT pt;
            GetCursorPos(&pt);
            splitterDragStartX = pt.x;
            SetCapture(hwnd);
            return 0;
        case WM_LBUTTONUP:
            if (splitterDragging) {
                splitterDragging = false;
                ReleaseCapture();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        case WM_SETCURSOR:
            SetCursor(LoadCursorW(NULL, (LPCWSTR)IDC_SIZEWE));
            return TRUE;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void createUI(SimpleWindow* win) {
    int y = 4;
    int m = 10;

    // --- Row 1: Action buttons + Tool parameters ---
    styleBtn(win, new Button(m, y, 110, 26, "Export GCode",
        [](Button*) { doExportGCode(); }),
        CLR_EXPORT_BG, CLR_EXPORT_TEXT, CLR_EXPORT_HOVER);

    styleBtn(win, new Button(m + 115, y, 90, 26, "Reset View",
        [](Button*) {
            extern VectorCanvas* canvas;
            if (canvas) canvas->resetView();
        }),
        CLR_TOOL_BG, CLR_TOOL_TEXT, CLR_TOOL_HOVER);

    // Tool parameters (compact, same row)
    int px = m + 225;
    auto addParam = [&](const wchar_t* label, int lw, int fw, std::string* varPtr) {
        auto* lbl = new Label(px, y + 3, lw, 20, label);
        win->add(lbl);
        lbl->setFont(L"Segoe UI", 10, false);
        lbl->setTextColor(CLR_LABEL_TEXT);
        lbl->setBackColor(CLR_WIN_BG);
        px += lw;

        auto* field = new InputField(px, y, fw, 24, varPtr->c_str(),
            [varPtr](InputField*, const char* text) {
                *varPtr = text;
                doRenderPreview();
            });
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

    // --- Layout editor (RichEdit control) ---
    LoadLibraryW(L"Msftedit.dll");

    hEditor = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"RICHEDIT50W", L"",
        WS_CHILD | WS_VISIBLE |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN |
        WS_VSCROLL | WS_HSCROLL,
        m, y, editorWidth - m, 525,
        win->getHandle(),
        NULL,
        _core.hInstance, NULL);

    // Enable EN_CHANGE notifications for RichEdit
    SendMessageW(hEditor, EM_SETEVENTMASK, 0,
        SendMessageW(hEditor, EM_GETEVENTMASK, 0, 0) | ENM_CHANGE);

    // Set background color
    SendMessageW(hEditor, EM_SETBKGNDCOLOR, 0, (LPARAM)CLR_EDITOR_BG);

    // Set default character format (Consolas 12pt, editor text color)
    CHARFORMAT2W cf = {};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_FACE | CFM_SIZE | CFM_COLOR;
    cf.yHeight = 240; // 12pt in twips
    cf.crTextColor = CLR_EDITOR_TEXT;
    lstrcpynW(cf.szFaceName, L"Consolas", LF_FACESIZE);
    SendMessageW(hEditor, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);

    // --- Splitter ---
    static bool splitterClassReg = false;
    if (!splitterClassReg) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = SplitterProc;
        wc.hInstance = _core.hInstance;
        wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_SIZEWE);
        wc.lpszClassName = L"WL2_Splitter";
        RegisterClassExW(&wc);
        splitterClassReg = true;
    }

    hSplitter = CreateWindowExW(0,
        L"WL2_Splitter", L"",
        WS_CHILD | WS_VISIBLE,
        editorWidth, y, 6, 525,
        win->getHandle(), NULL, _core.hInstance, NULL);

    // Dark theme for the editor via parent subclass
    SetWindowSubclass(win->getHandle(), EditorParentSubclassProc, 1, 0);
}

// ============================================================================
// setEditorText wrapper — suppress EN_CHANGE during programmatic text set
// ============================================================================
void setEditorTextUI(const std::string& text) {
    editorChangeIgnore = true;
    setEditorText(text);
    editorChangeIgnore = false;
}

// ============================================================================
// Highlight error lines in the RichEdit editor
// ============================================================================
void highlightEditorErrors(const std::vector<int>& errorLines) {
    if (!hEditor) return;

    SendMessageW(hEditor, WM_SETREDRAW, FALSE, 0);

    // Save current selection
    CHARRANGE crOld;
    SendMessageW(hEditor, EM_EXGETSEL, 0, (LPARAM)&crOld);

    // Reset all text to default format (no underline, normal color)
    CHARRANGE crAll = { 0, -1 };
    SendMessageW(hEditor, EM_EXSETSEL, 0, (LPARAM)&crAll);

    CHARFORMAT2W cfDefault = {};
    cfDefault.cbSize = sizeof(cfDefault);
    cfDefault.dwMask = CFM_COLOR | CFM_UNDERLINE;
    cfDefault.crTextColor = CLR_EDITOR_TEXT;
    cfDefault.dwEffects = 0;
    SendMessageW(hEditor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfDefault);

    // Apply error format to each error line
    if (!errorLines.empty()) {
        CHARFORMAT2W cfError = {};
        cfError.cbSize = sizeof(cfError);
        cfError.dwMask = CFM_COLOR | CFM_UNDERLINE;
        cfError.crTextColor = CLR_ERROR_TEXT;
        cfError.dwEffects = CFE_UNDERLINE;

        for (int line : errorLines) {
            LONG charIdx = (LONG)SendMessageW(hEditor, EM_LINEINDEX, (WPARAM)line, 0);
            if (charIdx < 0) continue;
            LONG lineLen = (LONG)SendMessageW(hEditor, EM_LINELENGTH, (WPARAM)charIdx, 0);
            if (lineLen <= 0) continue;

            CHARRANGE cr = { charIdx, charIdx + lineLen };
            SendMessageW(hEditor, EM_EXSETSEL, 0, (LPARAM)&cr);
            SendMessageW(hEditor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfError);
        }
    }

    // Restore caret and set typing format to default
    SendMessageW(hEditor, EM_EXSETSEL, 0, (LPARAM)&crOld);
    SendMessageW(hEditor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfDefault);

    SendMessageW(hEditor, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hEditor, NULL, TRUE);
}
