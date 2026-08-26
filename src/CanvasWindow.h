// ============================================================================
// CanvasWindow.h — Application-specific canvas for vector document rendering
// ============================================================================
#ifndef CANVAS_WINDOW_H
#define CANVAS_WINDOW_H

#include "Document/Document.h"
#include "Font/VectorLetterEngine.h"
#include "theme.h"
#include <UI/CanvasWindow/CanvasWindow.h>

class VectorCanvas : public CanvasWindow {
public:
    VectorCanvas();
    ~VectorCanvas() override;

    void setDocument(const Document* doc);
    void fitToContent();
    void fitToWorkspace();

    void setRapidMovesVisible(bool visible);
    bool isRapidMovesVisible() const { return m_showRapidMoves; }

    void setHUDVisible(bool visible);
    bool isHUDVisible() const { return m_showHUD; }

    void setVectorArrowsVisible(bool visible);
    bool isVectorArrowsVisible() const { return m_showVectorArrows; }

protected:
    void onDraw(HDC hdc, const RECT& clientRect) override;

private:
    const Document* m_document = nullptr;
    double m_activeDefaultZoom = 1.0;
    bool m_showRapidMoves = true;
    bool m_showHUD = true;
    bool m_showVectorArrows = true;

    HPEN m_penVector = nullptr;
    HPEN m_penFrame = nullptr;
    HPEN m_penWorkspace = nullptr;
    HPEN m_penRapid = nullptr;
    HPEN m_penOriginX = nullptr;
    HPEN m_penOriginY = nullptr;
    HPEN m_penKerf = nullptr;

    void drawWorkspaceBounds(HDC hdc);
    void drawOriginAxis(HDC hdc);
    void drawRapidTraverses(HDC hdc);
    void drawDocument(HDC hdc);
    void drawNameplate(HDC hdc, const Nameplate& plate);
    void drawPolyline(HDC hdc, const PointCollection& points, double scale, HPEN pen);
    void drawStartAndDirection(HDC hdc, const PointCollection& points, double scale);
    void drawHUDOverlay(HDC hdc, const RECT& rc);
};

#endif // CANVAS_WINDOW_H
