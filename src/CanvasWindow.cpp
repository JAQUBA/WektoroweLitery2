// ============================================================================
// CanvasWindow.cpp — Application-specific canvas for vector document rendering
// ============================================================================
#include "CanvasWindow.h"
#include "AppState.h"

VectorCanvas::VectorCanvas() {
    m_penVector = CreatePen(PS_SOLID, 1, CLR_VECTOR_LINE);
    m_penFrame = CreatePen(PS_SOLID, 2, CLR_FRAME_LINE);
    m_penWorkspace = CreatePen(PS_DASH, 1, CLR_WORKSPACE_LINE);
}

VectorCanvas::~VectorCanvas() {
    if (m_penVector) { DeleteObject(m_penVector); m_penVector = nullptr; }
    if (m_penFrame) { DeleteObject(m_penFrame); m_penFrame = nullptr; }
    if (m_penWorkspace) { DeleteObject(m_penWorkspace); m_penWorkspace = nullptr; }
}

void VectorCanvas::setDocument(const Document* doc) {
    m_document = doc;
    redraw();
}

void VectorCanvas::fitToContent() {
    if (!m_hwnd) return;
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int clientW = rc.right;
    int clientH = rc.bottom;
    if (clientW <= 0 || clientH <= 0) return;

    double worldW = workspaceWidth;
    double worldH = workspaceHeight;
    if (worldW <= 0 || worldH <= 0) return;

    int margin = 20;
    double availW = clientW - 2.0 * margin;
    double availH = clientH - 2.0 * margin;
    if (availW <= 0 || availH <= 0) return;

    double zoomW = availW / (worldW * 10.0);
    double zoomH = availH / (worldH * 10.0);
    double zoom = (zoomW < zoomH) ? zoomW : zoomH;

    double usedW = worldW * zoom * 10.0;
    double usedH = worldH * zoom * 10.0;
    double panX = (clientW - usedW) / 2.0;
    double panY = (clientH - usedH) / 2.0;

    setDefaultZoom(zoom);
    setDefaultPan(panX, panY);
    redraw();
}

// ============================================================================
// Drawing
// ============================================================================
void VectorCanvas::onDraw(HDC hdc, const RECT& rc) {
    drawWorkspaceBounds(hdc);
    if (m_document) drawDocument(hdc);
}

void VectorCanvas::drawWorkspaceBounds(HDC hdc) {
    HPEN oldPen = (HPEN)SelectObject(hdc, m_penWorkspace);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    int left   = toScreenX(0);
    int bottom = toScreenY(0);
    int right  = toScreenX(workspaceWidth);
    int top    = toScreenY(workspaceHeight);
    Rectangle(hdc, left, top, right, bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
}

void VectorCanvas::drawDocument(HDC hdc) {
    if (!m_document) return;

    for (const auto& row : m_document->getRows()) {
        for (const auto& plate : row.getNameplates()) {
            drawNameplate(hdc, plate);
        }
    }
}

void VectorCanvas::drawNameplate(HDC hdc, const Nameplate& plate) {
    // Draw frame
    if (plate.hasFrame) {
        HPEN oldPen = (HPEN)SelectObject(hdc, m_penFrame);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        int left   = toScreenX(plate.frameLeft_mm);
        int bottom = toScreenY(plate.frameBottom_mm);
        int right  = toScreenX(plate.frameLeft_mm + plate.frameWidth_mm);
        int top    = toScreenY(plate.frameBottom_mm + plate.frameHeight_mm);
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

void VectorCanvas::drawPolyline(HDC hdc, const PointCollection& points,
                                 double scale, HPEN pen) {
    if (points.size() < 2) return;
    SelectObject(hdc, pen);

    MoveToEx(hdc, toScreenX(points[0].X / scale),
                  toScreenY(points[0].Y / scale), nullptr);

    for (size_t i = 1; i < points.size(); i++) {
        LineTo(hdc, toScreenX(points[i].X / scale),
                    toScreenY(points[i].Y / scale));
    }
}
