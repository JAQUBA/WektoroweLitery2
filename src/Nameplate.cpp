// ============================================================================
// Nameplate.cpp — Nameplate text layout and vector generation
// ============================================================================
#include "Nameplate.h"
#include <cmath>

void Nameplate::appendText(const std::string& txt, const std::string& csvDir) {
    m_frameCenterX_mm = (frameLeft_mm + frameWidth_mm) / 2.0;
    m_frameCenterY_mm = (frameBottom_mm + frameHeight_mm) / 2.0;

    m_cursorX = frameLeft_mm * (SHIFT_Y_BASE / textHeight_mm);
    text = txt;

    int charCount = static_cast<int>(txt.size());

    for (int i = 0; i < charCount; i++) {
        char ch = txt[i];

        if (ch == ' ') {
            m_letters.push_back(VectorLetterEngine());
            m_advanceX = SPACE_WIDTH * condensation;
        } else {
            std::string charCode = std::to_string(static_cast<int>(ch));
            std::string csvPath = csvDir + charCode + ".csv";

            m_letters.push_back(VectorLetterEngine(
                csvPath, ';', 1.0,
                900000.0 / textHeight_mm,
                textHeight_mm, thickness, diameter, stepover));

            m_letters[i].multiplyX(condensation);
            m_advanceX = m_letters[i].maxX * condensation;

            // Shift letter segments to current cursor position
            size_t segCount = m_letters[i].segments.size();
            for (size_t seg = 0; seg < segCount; seg++) {
                size_t ptCount = m_letters[i].segments[seg].size();
                for (size_t pt = 0; pt < ptCount; pt++) {
                    m_letters[i].segments[seg][pt].x += m_cursorX;
                }
            }
        }
        m_cursorX += m_advanceX + (SPACE_WIDTH * condensation);
    }

    m_cursorX -= (SPACE_WIDTH * condensation);

    // Center text within frame
    m_textWidth_mm = (m_cursorX / (SHIFT_Y_BASE / textHeight_mm)) / 2.0;
    textLeft_mm = m_frameCenterX_mm - m_textWidth_mm;
    textBottom_mm = m_frameCenterY_mm - (textHeight_mm / 2.0);

    m_cursorX = textLeft_mm * (SHIFT_Y_BASE / textHeight_mm);
    m_cursorY = ((frameBottom_mm / 2.0) + textBottom_mm) * (SHIFT_Y_BASE / textHeight_mm);

    // Apply centering offset and generate full paths
    for (int i = 0; i < charCount; i++) {
        size_t segCount = m_letters[i].segments.size();
        for (size_t seg = 0; seg < segCount; seg++) {
            size_t ptCount = m_letters[i].segments[seg].size();
            for (size_t pt = 0; pt < ptCount; pt++) {
                m_letters[i].segments[seg][pt].x += m_cursorX;
                m_letters[i].segments[seg][pt].y += m_cursorY;
            }
        }
        m_letters[i].generateFullPath();
    }
}
