// ============================================================================
// Nameplate.cpp — Nameplate text layout and vector generation
// ============================================================================
#include "Nameplate.h"
#include <cmath>
#include <windows.h>

// Scale factor: LFF height (9 units) → internal units (3000)
static const double LFF_SCALE = 3000.0 / 9.0;

// Convert UTF-8 string to vector of Unicode code points
static std::vector<int> utf8ToCodePoints(const std::string& utf8) {
    std::vector<int> codePoints;
    if (utf8.empty()) return codePoints;

    int wideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                      utf8.c_str(), -1, nullptr, 0);
    UINT codePage = CP_UTF8;

    // Fallback for ANSI files saved in Windows-1250 / system codepage.
    if (wideLen <= 0) {
        wideLen = MultiByteToWideChar(1250, 0, utf8.c_str(), -1, nullptr, 0);
        codePage = 1250;
    }
    if (wideLen <= 0) {
        wideLen = MultiByteToWideChar(CP_ACP, 0, utf8.c_str(), -1, nullptr, 0);
        codePage = CP_ACP;
    }
    if (wideLen <= 0) return codePoints;

    std::vector<wchar_t> wide(wideLen);
    MultiByteToWideChar(codePage, 0, utf8.c_str(), -1, wide.data(), wideLen);

    // Exclude null terminator
    for (int i = 0; i < wideLen - 1; i++)
        codePoints.push_back(static_cast<int>(wide[i]));

    return codePoints;
}

void Nameplate::appendText(const std::string& txt, const LffFont& font) {
    m_frameCenterX_mm = (frameLeft_mm + frameWidth_mm) / 2.0;
    m_frameCenterY_mm = (frameBottom_mm + frameHeight_mm) / 2.0;

    m_cursorX = frameLeft_mm * (SHIFT_Y_BASE / textHeight_mm);
    text = txt;

    // Spacing from LFF font (scaled to internal units)
    double letterSpacing = font.getLetterSpacing() * LFF_SCALE;
    double wordSpacing = font.getWordSpacing() * LFF_SCALE;

    auto codePoints = utf8ToCodePoints(txt);
    int charCount = static_cast<int>(codePoints.size());

    for (int i = 0; i < charCount; i++) {
        int cp = codePoints[i];

        if (cp == ' ') {
            m_letters.push_back(VectorLetterEngine());
            m_advanceX = wordSpacing * condensation;
        } else {
            const LffGlyph* glyph = font.getGlyph(cp);

            if (glyph && !glyph->strokes.empty()) {
                m_letters.push_back(VectorLetterEngine(
                    *glyph, 1.0,
                    900000.0 / textHeight_mm,
                    textHeight_mm, thickness, diameter, stepover));

                m_letters[i].multiplyX(condensation);
                m_advanceX = m_letters[i].maxX * condensation;
            } else {
                // Keep layout stable even if a glyph is missing.
                m_letters.push_back(VectorLetterEngine());
                m_advanceX = wordSpacing * condensation;
            }

            // Shift letter segments to current cursor position
            size_t segCount = m_letters[i].segments.size();
            for (size_t seg = 0; seg < segCount; seg++) {
                size_t ptCount = m_letters[i].segments[seg].size();
                for (size_t pt = 0; pt < ptCount; pt++) {
                    m_letters[i].segments[seg][pt].x += m_cursorX;
                }
            }
        }
        m_cursorX += m_advanceX + (letterSpacing * condensation);
    }

    m_cursorX -= (letterSpacing * condensation);

    // Center text within frame
    m_textWidth_mm = (m_cursorX / (SHIFT_Y_BASE / textHeight_mm)) / 2.0;
    textLeft_mm = m_frameCenterX_mm - m_textWidth_mm + textOffsetX_mm;
    textBottom_mm = m_frameCenterY_mm - (textHeight_mm / 2.0) + textOffsetY_mm;

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

bool Nameplate::getBoundingBox(double& outMinX, double& outMinY, double& outMaxX, double& outMaxY) const {
    double bMinX = 1e9, bMinY = 1e9, bMaxX = -1e9, bMaxY = -1e9;
    bool foundAny = false;

    if (hasFrame && frameWidth_mm > 0.0 && frameHeight_mm > 0.0) {
        bMinX = frameLeft_mm;
        bMinY = frameBottom_mm;
        bMaxX = frameLeft_mm + frameWidth_mm;
        bMaxY = frameBottom_mm + frameHeight_mm;
        foundAny = true;
    }

    if (textHeight_mm > 0.0) {
        double scale = 3000.0 / textHeight_mm;
        for (const auto& letter : m_letters) {
            for (const auto& segment : letter.getPointCollections()) {
                for (const auto& pt : segment) {
                    double wx = pt.X / scale;
                    double wy = pt.Y / scale;
                    if (wx < bMinX) bMinX = wx;
                    if (wy < bMinY) bMinY = wy;
                    if (wx > bMaxX) bMaxX = wx;
                    if (wy > bMaxY) bMaxY = wy;
                    foundAny = true;
                }
            }
        }
    }

    if (foundAny) {
        outMinX = bMinX;
        outMinY = bMinY;
        outMaxX = bMaxX;
        outMaxY = bMaxY;
        return true;
    }
    return false;
}
