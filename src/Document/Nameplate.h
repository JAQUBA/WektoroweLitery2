// ============================================================================
// Nameplate.h — Single nameplate (text within a frame)
// ============================================================================
#ifndef NAMEPLATE_H
#define NAMEPLATE_H

#include "../Font/VectorLetterEngine.h"
#include "../Font/LffFont.h"
#include <vector>
#include <string>

class Nameplate {
public:
    double frameLeft_mm = 0.0;
    double frameWidth_mm = 0.0;
    double frameBottom_mm = 0.0;
    double frameHeight_mm = 0.0;
    std::string text;
    double textHeight_mm = 0.0;
    double condensation = 1.0;
    double thickness = 0.0;
    double diameter = 0.0;
    double stepover = 0.0;
    bool hasFrame = false;
    double maxX = 0.0;
    double textLeft_mm = 0.0;
    double textBottom_mm = 0.0;
    double textOffsetX_mm = 0.0;
    double textOffsetY_mm = 0.0;

    Nameplate() = default;

    void appendText(const std::string& txt, const LffFont& font);
    bool getBoundingBox(double& minX, double& minY, double& maxX, double& maxY) const;

    const std::vector<VectorLetterEngine>& getLetters() const { return m_letters; }

private:
    static constexpr double SHIFT_Y_BASE = 3000.0;

    double m_frameCenterX_mm = 0.0;
    double m_frameCenterY_mm = 0.0;
    double m_textWidth_mm = 0.0;
    double m_cursorX = 0.0;
    double m_advanceX = 0.0;
    double m_cursorY = 1300.0;

    std::vector<VectorLetterEngine> m_letters;
};

#endif // NAMEPLATE_H
