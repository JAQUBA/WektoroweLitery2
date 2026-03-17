// ============================================================================
// CanvasWindow.h — Zoomable/pannable GDI canvas for vector rendering
// ============================================================================
#ifndef CANVAS_WINDOW_H
#define CANVAS_WINDOW_H

#include "Document.h"
#include "VectorLetterEngine.h"
#include "theme.h"
#include <windows.h>
#include <vector>
#include <string>

class CanvasWindow {
public:
    CanvasWindow();
    ~CanvasWindow();

    void create(HWND parent, int x, int y, int w, int h);
    void setDocument(const Document* doc);
    void setGridVisible(bool visible);
    void resetView();
    void redraw();
    HWND getHandle() const { return m_hwnd; }

private:
    HWND m_hwnd = nullptr;
    HWND m_parent = nullptr;
    const Document* m_document = nullptr;
    bool m_gridVisible = true;

    // View transform (pan + zoom)
    double m_zoom = 0.15;
    double m_panX = 10.0;
    double m_panY = 10.0;

    // Mouse drag state
    bool m_dragging = false;
    POINT m_dragStart = {};
    double m_dragPanStartX = 0.0;
    double m_dragPanStartY = 0.0;

    // Canvas virtual size (mm * 10)
    static constexpr int CANVAS_W = 2100;
    static constexpr int CANVAS_H = 3000;

    // GDI pens/brushes (created once)
    HPEN m_penGrid = nullptr;
    HPEN m_penVector = nullptr;
    HPEN m_penFrame = nullptr;
    HBRUSH m_brushBg = nullptr;

    void createResources();
    void destroyResources();

    void onPaint(HDC hdc, const RECT& rc);
    void drawGrid(HDC hdc);
    void drawDocument(HDC hdc);
    void drawNameplate(HDC hdc, const Nameplate& plate);
    void drawPolyline(HDC hdc, const PointCollection& points, double scale, HPEN pen);

    // World-to-screen transform
    int toScreenX(double worldX) const;
    int toScreenY(double worldY) const;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static const wchar_t* CLASS_NAME;
    static bool s_classRegistered;
};

#endif // CANVAS_WINDOW_H
