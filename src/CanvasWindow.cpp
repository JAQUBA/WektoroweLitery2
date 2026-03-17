// ============================================================================
// CanvasWindow.cpp — Zoomable/pannable GDI canvas implementation
// ============================================================================
#include "CanvasWindow.h"
#include <windowsx.h>
#include <algorithm>
#include <cmath>

const wchar_t* CanvasWindow::CLASS_NAME = L"VectorLettersCanvas";
bool CanvasWindow::s_classRegistered = false;

CanvasWindow::CanvasWindow() {}

CanvasWindow::~CanvasWindow() {
    destroyResources();
    if (m_hwnd) DestroyWindow(m_hwnd);
}

// ============================================================================
// Window creation
// ============================================================================
void CanvasWindow::create(HWND parent, int x, int y, int w, int h) {
    m_parent = parent;

    if (!s_classRegistered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        RegisterClassW(&wc);
        s_classRegistered = true;
    }

    m_hwnd = CreateWindowExW(
        0, CLASS_NAME, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        x, y, w, h,
        parent, nullptr, GetModuleHandleW(nullptr), this);

    createResources();
}

// ============================================================================
// GDI Resources
// ============================================================================
void CanvasWindow::createResources() {
    m_penGrid = CreatePen(PS_SOLID, 1, CLR_GRID_LINE);
    m_penVector = CreatePen(PS_SOLID, 1, CLR_VECTOR_LINE);
    m_penFrame = CreatePen(PS_SOLID, 2, CLR_FRAME_LINE);
    m_brushBg = CreateSolidBrush(CLR_CANVAS_BG);
}

void CanvasWindow::destroyResources() {
    if (m_penGrid) { DeleteObject(m_penGrid); m_penGrid = nullptr; }
    if (m_penVector) { DeleteObject(m_penVector); m_penVector = nullptr; }
    if (m_penFrame) { DeleteObject(m_penFrame); m_penFrame = nullptr; }
    if (m_brushBg) { DeleteObject(m_brushBg); m_brushBg = nullptr; }
}

// ============================================================================
// View control
// ============================================================================
void CanvasWindow::setDocument(const Document* doc) {
    m_document = doc;
    redraw();
}

void CanvasWindow::setGridVisible(bool visible) {
    m_gridVisible = visible;
    redraw();
}

void CanvasWindow::resetView() {
    m_zoom = 0.15;
    m_panX = 10.0;
    m_panY = 10.0;
    redraw();
}

void CanvasWindow::redraw() {
    if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE);
}

// ============================================================================
// Coordinate transforms
// ============================================================================
int CanvasWindow::toScreenX(double worldX) const {
    return static_cast<int>(worldX * m_zoom * 10.0 + m_panX);
}

int CanvasWindow::toScreenY(double worldY) const {
    // Flip Y: world bottom = screen top
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    return rc.bottom - static_cast<int>(worldY * m_zoom * 10.0 + m_panY);
}

// ============================================================================
// Drawing
// ============================================================================
void CanvasWindow::onPaint(HDC hdc, const RECT& rc) {
    // Background
    FillRect(hdc, &rc, m_brushBg);

    // Grid
    if (m_gridVisible) drawGrid(hdc);

    // Document content
    if (m_document) drawDocument(hdc);
}

void CanvasWindow::drawGrid(HDC hdc) {
    SelectObject(hdc, m_penGrid);

    // Horizontal lines every 10mm
    for (int y = 0; y <= 300; y += 10) {
        int sy = toScreenY(static_cast<double>(y));
        MoveToEx(hdc, toScreenX(0), sy, nullptr);
        LineTo(hdc, toScreenX(210), sy);
    }

    // Vertical lines every 10mm
    for (int x = 0; x <= 210; x += 10) {
        int sx = toScreenX(static_cast<double>(x));
        MoveToEx(hdc, sx, toScreenY(0), nullptr);
        LineTo(hdc, sx, toScreenY(300));
    }
}

void CanvasWindow::drawDocument(HDC hdc) {
    if (!m_document) return;

    for (const auto& row : m_document->getRows()) {
        for (const auto& plate : row.getNameplates()) {
            drawNameplate(hdc, plate);
        }
    }
}

void CanvasWindow::drawNameplate(HDC hdc, const Nameplate& plate) {
    // Draw frame
    if (plate.hasFrame) {
        HPEN oldPen = (HPEN)SelectObject(hdc, m_penFrame);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        int left   = toScreenX(plate.frameLeft_mm);
        int bottom = toScreenY(plate.frameBottom_mm);
        int right  = toScreenX(plate.frameLeft_mm + plate.frameWidth_mm);
        int top    = toScreenY(plate.frameBottom_mm + plate.frameHeight_mm);
        // Rectangle expects top < bottom in screen coords
        Rectangle(hdc, left, top, right, bottom);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
    }

    // Draw letter vectors
    SelectObject(hdc, m_penVector);
    for (const auto& letter : plate.getLetters()) {
        double scale = 3000.0 / plate.textHeight_mm;
        for (const auto& segment : letter.getPointCollections()) {
            drawPolyline(hdc, segment, scale, m_penVector);
        }
    }
}

void CanvasWindow::drawPolyline(HDC hdc, const PointCollection& points,
                                 double scale, HPEN pen) {
    if (points.size() < 2) return;
    SelectObject(hdc, pen);

    // Points are in internal coords (scale = 3000/textHeight)
    // Convert to mm: pt / scale
    MoveToEx(hdc, toScreenX(points[0].X / scale),
                  toScreenY(points[0].Y / scale), nullptr);

    for (size_t i = 1; i < points.size(); i++) {
        LineTo(hdc, toScreenX(points[i].X / scale),
                    toScreenY(points[i].Y / scale));
    }
}

// ============================================================================
// Window Procedure
// ============================================================================
LRESULT CALLBACK CanvasWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    CanvasWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<CanvasWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
    } else {
        self = reinterpret_cast<CanvasWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (!self) return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);

            // Double buffering
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

            self->onPaint(memDC, rc);

            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
            SelectObject(memDC, oldBmp);
            DeleteObject(memBmp);
            DeleteDC(memDC);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_MOUSEWHEEL: {
            short delta = GET_WHEEL_DELTA_WPARAM(wParam);
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hwnd, &pt);

            double oldZoom = self->m_zoom;
            double zoomDelta = (delta > 0) ? 0.02 : -0.02;
            self->m_zoom += zoomDelta;
            if (self->m_zoom < 0.02) self->m_zoom = 0.02;
            if (self->m_zoom > 5.0) self->m_zoom = 5.0;

            // Zoom towards mouse position
            double ratio = self->m_zoom / oldZoom;
            self->m_panX = pt.x - ratio * (pt.x - self->m_panX);
            self->m_panY = pt.y - ratio * (pt.y - self->m_panY);

            self->redraw();
            return 0;
        }

        case WM_LBUTTONDOWN: {
            self->m_dragging = true;
            self->m_dragStart.x = GET_X_LPARAM(lParam);
            self->m_dragStart.y = GET_Y_LPARAM(lParam);
            self->m_dragPanStartX = self->m_panX;
            self->m_dragPanStartY = self->m_panY;
            SetCapture(hwnd);
            return 0;
        }

        case WM_MOUSEMOVE: {
            if (self->m_dragging) {
                int dx = GET_X_LPARAM(lParam) - self->m_dragStart.x;
                int dy = GET_Y_LPARAM(lParam) - self->m_dragStart.y;
                self->m_panX = self->m_dragPanStartX + dx;
                self->m_panY = self->m_dragPanStartY - dy;
                self->redraw();
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            self->m_dragging = false;
            ReleaseCapture();
            return 0;
        }

        case WM_LBUTTONDBLCLK: {
            self->resetView();
            return 0;
        }

        case WM_ERASEBKGND:
            return 1; // handled in WM_PAINT
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
