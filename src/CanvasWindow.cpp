// ============================================================================
// CanvasWindow.cpp — Application-specific canvas for vector document rendering
// ============================================================================
#include "CanvasWindow.h"
#include "AppState.h"
#include <cmath>
#include <cstdio>
#include <vector>

struct PathSegment2D {
    double startX = 0.0;
    double startY = 0.0;
    double endX = 0.0;
    double endY = 0.0;
};

VectorCanvas::VectorCanvas() {
    m_penVector = CreatePen(PS_SOLID, 1, CLR_VECTOR_LINE);
    m_penFrame = CreatePen(PS_SOLID, 2, CLR_FRAME_LINE);
    m_penWorkspace = CreatePen(PS_DASH, 1, CLR_WORKSPACE_LINE);
    m_penRapid = CreatePen(PS_DOT, 1, CLR_RAPID_LINE);
    m_penOriginX = CreatePen(PS_SOLID, 2, CLR_ORIGIN_X);
    m_penOriginY = CreatePen(PS_SOLID, 2, CLR_ORIGIN_Y);
    m_penKerf = CreatePen(PS_SOLID, 1, CLR_TOOL_KERF);
}

VectorCanvas::~VectorCanvas() {
    if (m_penVector)    { DeleteObject(m_penVector);    m_penVector = nullptr; }
    if (m_penFrame)     { DeleteObject(m_penFrame);     m_penFrame = nullptr; }
    if (m_penWorkspace) { DeleteObject(m_penWorkspace); m_penWorkspace = nullptr; }
    if (m_penRapid)     { DeleteObject(m_penRapid);     m_penRapid = nullptr; }
    if (m_penOriginX)   { DeleteObject(m_penOriginX);   m_penOriginX = nullptr; }
    if (m_penOriginY)   { DeleteObject(m_penOriginY);   m_penOriginY = nullptr; }
    if (m_penKerf)      { DeleteObject(m_penKerf);      m_penKerf = nullptr; }
}

void VectorCanvas::setDocument(const Document* doc) {
    m_document = doc;
    redraw();
}

void VectorCanvas::fitToWorkspace() {
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

    m_activeDefaultZoom = zoom;
    setDefaultZoom(zoom);
    setDefaultPan(panX, panY);
    redraw();
}

void VectorCanvas::fitToContent() {
    if (!m_hwnd) return;
    if (!m_document) {
        fitToWorkspace();
        return;
    }

    double minX = 0.0, minY = 0.0, maxX = 0.0, maxY = 0.0;
    if (!m_document->getBoundingBox(minX, minY, maxX, maxY)) {
        fitToWorkspace();
        return;
    }

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int clientW = rc.right;
    int clientH = rc.bottom;
    if (clientW <= 0 || clientH <= 0) return;

    double marginMM = 10.0;
    double targetMinX = minX - marginMM;
    double targetMinY = minY - marginMM;
    double bboxW = (maxX - minX) + 2.0 * marginMM;
    double bboxH = (maxY - minY) + 2.0 * marginMM;
    if (bboxW <= 0.0) bboxW = 10.0;
    if (bboxH <= 0.0) bboxH = 10.0;

    int marginPx = 25;
    double availW = clientW - 2.0 * marginPx;
    double availH = clientH - 2.0 * marginPx;
    if (availW <= 0 || availH <= 0) return;

    double zoomW = availW / (bboxW * 10.0);
    double zoomH = availH / (bboxH * 10.0);
    double zoom = (zoomW < zoomH) ? zoomW : zoomH;

    double panX = (clientW - bboxW * zoom * 10.0) / 2.0 - targetMinX * zoom * 10.0;
    double panY = (clientH - bboxH * zoom * 10.0) / 2.0 - targetMinY * zoom * 10.0;

    m_activeDefaultZoom = zoom;
    setDefaultZoom(zoom);
    setDefaultPan(panX, panY);
    redraw();
}

void VectorCanvas::setRapidMovesVisible(bool visible) {
    m_showRapidMoves = visible;
    redraw();
}

void VectorCanvas::setHUDVisible(bool visible) {
    m_showHUD = visible;
    redraw();
}

void VectorCanvas::setVectorArrowsVisible(bool visible) {
    m_showVectorArrows = visible;
    redraw();
}

// ============================================================================
// Drawing
// ============================================================================
void VectorCanvas::onDraw(HDC hdc, const RECT& rc) {
    drawWorkspaceBounds(hdc);
    drawOriginAxis(hdc);
    if (m_document) {
        if (m_showRapidMoves) {
            drawRapidTraverses(hdc);
        }
        drawDocument(hdc);
    }
    if (m_showHUD) {
        drawHUDOverlay(hdc, rc);
    }
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

void VectorCanvas::drawOriginAxis(HDC hdc) {
    int x0 = toScreenX(0);
    int y0 = toScreenY(0);

    int oldBkMode = SetBkMode(hdc, TRANSPARENT);

    // Draw origin circle (5px radius)
    HBRUSH origBrush = CreateSolidBrush(CLR_ORIGIN_X);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, origBrush);
    HPEN oldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
    Ellipse(hdc, x0 - 4, y0 - 4, x0 + 5, y0 + 5);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(origBrush);

    // Draw X axis arrow (Red)
    oldPen = (HPEN)SelectObject(hdc, m_penOriginX);
    MoveToEx(hdc, x0, y0, nullptr);
    LineTo(hdc, x0 + 35, y0);
    LineTo(hdc, x0 + 28, y0 - 4);
    MoveToEx(hdc, x0 + 35, y0, nullptr);
    LineTo(hdc, x0 + 28, y0 + 4);

    COLORREF oldTextColor = SetTextColor(hdc, CLR_ORIGIN_X);
    TextOutW(hdc, x0 + 40, y0 - 8, L"X0", 2);

    // Draw Y axis arrow (Green)
    SelectObject(hdc, m_penOriginY);
    MoveToEx(hdc, x0, y0, nullptr);
    LineTo(hdc, x0, y0 - 35);
    LineTo(hdc, x0 - 4, y0 - 28);
    MoveToEx(hdc, x0, y0 - 35, nullptr);
    LineTo(hdc, x0 + 4, y0 - 28);

    SetTextColor(hdc, CLR_ORIGIN_Y);
    TextOutW(hdc, x0 - 8, y0 - 52, L"Y0", 2);

    SetTextColor(hdc, oldTextColor);
    SetBkMode(hdc, oldBkMode);
    SelectObject(hdc, oldPen);
}

void VectorCanvas::drawRapidTraverses(HDC hdc) {
    if (!m_document) return;

    std::vector<PathSegment2D> paths;

    for (const auto& row : m_document->getRows()) {
        for (const auto& plate : row.getNameplates()) {
            if (plate.hasFrame && plate.frameWidth_mm > 0.0 && plate.frameHeight_mm > 0.0) {
                PathSegment2D seg;
                seg.startX = plate.frameLeft_mm;
                seg.startY = plate.frameBottom_mm;
                seg.endX = plate.frameLeft_mm;
                seg.endY = plate.frameBottom_mm;
                paths.push_back(seg);
            }

            if (plate.textHeight_mm > 0.0) {
                double scale = 3000.0 / plate.textHeight_mm;
                for (const auto& letter : plate.getLetters()) {
                    for (const auto& poly : letter.getPointCollections()) {
                        if (poly.size() >= 2) {
                            PathSegment2D seg;
                            seg.startX = poly.front().X / scale;
                            seg.startY = poly.front().Y / scale;
                            seg.endX = poly.back().X / scale;
                            seg.endY = poly.back().Y / scale;
                            paths.push_back(seg);
                        }
                    }
                }
            }
        }
    }

    if (paths.empty()) return;

    HPEN oldPen = (HPEN)SelectObject(hdc, m_penRapid);

    // Rapid traverse from (0,0) to first path
    MoveToEx(hdc, toScreenX(0), toScreenY(0), nullptr);
    LineTo(hdc, toScreenX(paths[0].startX), toScreenY(paths[0].startY));

    // Rapid traverses between paths
    for (size_t i = 1; i < paths.size(); i++) {
        MoveToEx(hdc, toScreenX(paths[i-1].endX), toScreenY(paths[i-1].endY), nullptr);
        LineTo(hdc, toScreenX(paths[i].startX), toScreenY(paths[i].startY));
    }

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
    for (const auto& letter : plate.getLetters()) {
        double scale = 3000.0 / plate.textHeight_mm;
        for (const auto& segment : letter.getPointCollections()) {
            drawPolyline(hdc, segment, scale, m_penVector);
            if (m_showVectorArrows) {
                drawStartAndDirection(hdc, segment, scale);
            }
        }
    }
}

void VectorCanvas::drawPolyline(HDC hdc, const PointCollection& points,
                                 double scale, HPEN pen) {
    if (points.size() < 2) return;
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);

    MoveToEx(hdc, toScreenX(points[0].X / scale),
                  toScreenY(points[0].Y / scale), nullptr);

    for (size_t i = 1; i < points.size(); i++) {
        LineTo(hdc, toScreenX(points[i].X / scale),
                    toScreenY(points[i].Y / scale));
    }
    SelectObject(hdc, oldPen);
}

void VectorCanvas::drawStartAndDirection(HDC hdc, const PointCollection& points, double scale) {
    if (points.size() < 2) return;

    int sx = toScreenX(points[0].X / scale);
    int sy = toScreenY(points[0].Y / scale);

    // Draw start point dot
    HBRUSH startBrush = CreateSolidBrush(CLR_VECTOR_LINE);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, startBrush);
    HPEN oldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
    Ellipse(hdc, sx - 2, sy - 2, sx + 3, sy + 3);

    // Draw direction arrow on first segment if long enough
    int nx = toScreenX(points[1].X / scale);
    int ny = toScreenY(points[1].Y / scale);
    double dx = nx - sx;
    double dy = ny - sy;
    double len = std::sqrt(dx * dx + dy * dy);

    if (len >= 12.0) {
        double mx = (sx + nx) / 2.0;
        double my = (sy + ny) / 2.0;
        double angle = std::atan2(dy, dx);

        HPEN arrowPen = CreatePen(PS_SOLID, 1, CLR_VECTOR_LINE);
        SelectObject(hdc, arrowPen);

        double wingAngle = 0.5; // rad
        double wingLen = 6.0;

        MoveToEx(hdc, (int)mx, (int)my, nullptr);
        LineTo(hdc, (int)(mx - wingLen * std::cos(angle - wingAngle)),
                    (int)(my - wingLen * std::sin(angle - wingAngle)));
        MoveToEx(hdc, (int)mx, (int)my, nullptr);
        LineTo(hdc, (int)(mx - wingLen * std::cos(angle + wingAngle)),
                    (int)(my - wingLen * std::sin(angle + wingAngle)));

        DeleteObject(arrowPen);
    }

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(startBrush);
}

void VectorCanvas::drawHUDOverlay(HDC hdc, const RECT& rc) {
    int left = 12;
    int top = 12;
    int width = 260;
    int height = 95;

    // HUD background box
    HBRUSH bgBrush = CreateSolidBrush(CLR_HUD_BG);
    HPEN framePen = CreatePen(PS_SOLID, 1, CLR_WORKSPACE_LINE);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, bgBrush);
    HPEN oldPen = (HPEN)SelectObject(hdc, framePen);

    RoundRect(hdc, left, top, left + width, top + height, 8, 8);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(framePen);
    DeleteObject(bgBrush);

    // HUD text
    HFONT hFont = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              VARIABLE_PITCH | FF_SWISS, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
    int oldBkMode = SetBkMode(hdc, TRANSPARENT);
    COLORREF oldColor = SetTextColor(hdc, CLR_HUD_TEXT);

    wchar_t lineBuf[128];

    // Line 1: Zoom level
    int zoomPct = (int)std::round((getZoom() / (m_activeDefaultZoom > 0 ? m_activeDefaultZoom : 1.0)) * 100.0);
    _snwprintf(lineBuf, 128, L"Skala podglądu: %d%%", zoomPct);
    TextOutW(hdc, left + 10, top + 8, lineBuf, (int)wcslen(lineBuf));

    // Line 2: Workspace size
    _snwprintf(lineBuf, 128, L"Stół roboczy: %.1f × %.1f mm", workspaceWidth, workspaceHeight);
    TextOutW(hdc, left + 10, top + 28, lineBuf, (int)wcslen(lineBuf));

    // Line 3: Content Bounding Box
    if (m_document) {
        double minX = 0, minY = 0, maxX = 0, maxY = 0;
        if (m_document->getBoundingBox(minX, minY, maxX, maxY)) {
            _snwprintf(lineBuf, 128, L"Zakres: %.1f × %.1f mm (X: %.1f..%.1f)",
                       maxX - minX, maxY - minY, minX, maxX);
        } else {
            _snwprintf(lineBuf, 128, L"Zakres: Brak zawartości");
        }
    } else {
        _snwprintf(lineBuf, 128, L"Zakres: Brak dokumentu");
    }
    TextOutW(hdc, left + 10, top + 48, lineBuf, (int)wcslen(lineBuf));

    // Line 4: Tool info
    if (m_document && m_document->millingDiameter_mm > 0.0) {
        _snwprintf(lineBuf, 128, L"Frez/Narzędzie: Ø %.2f mm, Krok %.2f mm",
                   m_document->millingDiameter_mm, m_document->stepover_mm);
    } else {
        _snwprintf(lineBuf, 128, L"Narzędzie: Domyślne V-Bit 0.3 mm");
    }
    TextOutW(hdc, left + 10, top + 68, lineBuf, (int)wcslen(lineBuf));

    SetTextColor(hdc, oldColor);
    SetBkMode(hdc, oldBkMode);
    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}
