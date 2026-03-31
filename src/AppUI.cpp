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
#include "LffFont.h"
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

static HWND hToolButton = nullptr;
static HWND hFontButton = nullptr;

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

    // Tool preset selector button
    auto* toolBtn = new Button(m + 115, y, 200, 26, "Tool",
        [](Button*) { showToolPopup(); });
    styleBtn(win, toolBtn, CLR_TOOL_BG, CLR_TOOL_TEXT, CLR_TOOL_HOVER);
    hToolButton = toolBtn->getHandle();
    updateToolButtonText();

    // Font selector button
    auto* fontBtn = new Button(m + 320, y, 160, 26, "Font",
        [](Button*) { showFontPopup(); });
    styleBtn(win, fontBtn, CLR_TOOL_BG, CLR_TOOL_TEXT, CLR_TOOL_HOVER);
    hFontButton = fontBtn->getHandle();
    updateFontButtonText();

    // --- Material parameter fields ---
    int px = m + 485;

    auto* lblMat = new Label(px, y + 3, 60, 20, L"Material:");
    win->add(lblMat);
    lblMat->setFont(L"Segoe UI", 11, true);
    lblMat->setTextColor(CLR_LABEL_TEXT);
    lblMat->setBackColor(CLR_WIN_BG);

    fldMaterial = new InputField(px + 63, y, 55, 24, exportMaterialThickness.c_str(),
        [](InputField* f, const char* text) {
            exportMaterialThickness = text;
        });
    win->add(fldMaterial);

    px += 125;

    auto* lblDep = new Label(px, y + 3, 48, 20, L"Depth:");
    win->add(lblDep);
    lblDep->setFont(L"Segoe UI", 11, true);
    lblDep->setTextColor(CLR_LABEL_TEXT);
    lblDep->setBackColor(CLR_WIN_BG);

    fldDepth = new InputField(px + 51, y, 55, 24, exportTextDepth.c_str(),
        [](InputField* f, const char* text) {
            exportTextDepth = text;
        });
    win->add(fldDepth);

    px += 113;

    auto* lblSafe = new Label(px, y + 3, 48, 20, L"Safe H:");
    win->add(lblSafe);
    lblSafe->setFont(L"Segoe UI", 11, true);
    lblSafe->setTextColor(CLR_LABEL_TEXT);
    lblSafe->setBackColor(CLR_WIN_BG);

    fldSafeH = new InputField(px + 51, y, 55, 24, exportSafeHeight.c_str(),
        [](InputField* f, const char* text) {
            exportSafeHeight = text;
        });
    win->add(fldSafeH);

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
// Tool preset selector popup and button text
// ============================================================================
void showToolPopup() {
    if (!hToolButton || !window) return;
    RECT rc;
    GetWindowRect(hToolButton, &rc);

    HMENU hMenu = CreatePopupMenu();
    for (int i = 0; i < (int)toolPresets.size(); i++) {
        std::wstring name = StringUtils::utf8ToWide(toolPresets[i].name);
        UINT flags = MF_STRING;
        if (i == activeToolIndex) flags |= MF_CHECKED;
        AppendMenuW(hMenu, flags, 10000 + i, name.c_str());
    }
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, 10999, L"Manage tools...");

    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
                             rc.left, rc.bottom, 0, window->getHandle(), NULL);
    DestroyMenu(hMenu);

    if (cmd >= 10000 && cmd < 10999) {
        doSelectTool(cmd - 10000);
    } else if (cmd == 10999) {
        doShowToolPresets();
    }
}

void updateToolButtonText() {
    if (!hToolButton) return;
    std::wstring text = L"\u25BC ";
    if (activeToolIndex >= 0 && activeToolIndex < (int)toolPresets.size()) {
        text += StringUtils::utf8ToWide(toolPresets[activeToolIndex].name);
    }
    SetWindowTextW(hToolButton, text.c_str());
}

// ============================================================================
// Font selector popup and button text
// ============================================================================
void showFontPopup() {
    if (!hFontButton || !window) return;
    RECT rc;
    GetWindowRect(hFontButton, &rc);

    std::vector<std::string> fonts = LffFont::listFonts(fontsDirectory);

    HMENU hMenu = CreatePopupMenu();
    for (int i = 0; i < (int)fonts.size(); i++) {
        std::wstring name = StringUtils::utf8ToWide(fonts[i]);
        UINT flags = MF_STRING;
        if (fonts[i] == activeFontName) flags |= MF_CHECKED;
        AppendMenuW(hMenu, flags, 11000 + i, name.c_str());
    }

    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
                             rc.left, rc.bottom, 0, window->getHandle(), NULL);
    DestroyMenu(hMenu);

    if (cmd >= 11000 && cmd < 11000 + (int)fonts.size()) {
        int idx = cmd - 11000;
        if (loadFont(fonts[idx])) {
            updateFontButtonText();
        }
    }
}

void updateFontButtonText() {
    if (!hFontButton) return;
    std::wstring text = L"\u25BC " + StringUtils::utf8ToWide(activeFontName);
    SetWindowTextW(hFontButton, text.c_str());
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
