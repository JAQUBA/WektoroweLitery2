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

    // --- Layout editor (multiline EDIT control) ---
    hEditor = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT", L"",
        WS_CHILD | WS_VISIBLE |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN |
        WS_VSCROLL | WS_HSCROLL,
        m, y, editorWidth - m, 525,
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
