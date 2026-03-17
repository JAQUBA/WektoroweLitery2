// ============================================================================
// CanvasWindow.h — Application-specific canvas for vector document rendering
// ============================================================================
#ifndef CANVAS_WINDOW_H
#define CANVAS_WINDOW_H

#include "Document.h"
#include "VectorLetterEngine.h"
#include "theme.h"
#include <UI/CanvasWindow/CanvasWindow.h>

class VectorCanvas : public CanvasWindow {
public:
    VectorCanvas();
    ~VectorCanvas() override;

    void setDocument(const Document* doc);

protected:
    void onDraw(HDC hdc, const RECT& clientRect) override;

private:
    const Document* m_document = nullptr;

    HPEN m_penVector = nullptr;
    HPEN m_penFrame = nullptr;

    void drawDocument(HDC hdc);
    void drawNameplate(HDC hdc, const Nameplate& plate);
    void drawPolyline(HDC hdc, const PointCollection& points, double scale, HPEN pen);
};

#endif // CANVAS_WINDOW_H
