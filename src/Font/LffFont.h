// ============================================================================
// LffFont.h — LibreCAD Font Format (.lff) parser
// ============================================================================
#ifndef LFF_FONT_H
#define LFF_FONT_H

#include <string>
#include <vector>
#include <unordered_map>

// --- Single point in an LFF polyline (with optional arc bulge) ---
struct LffPoint {
    double x = 0.0;
    double y = 0.0;
    double bulge = 0.0;  // 0 = line, non-zero = arc (tan(angle/4))
};

// --- A single glyph (character) in the LFF font ---
struct LffGlyph {
    int codePoint = 0;
    double width = 0.0;                            // max X extent
    std::vector<std::vector<LffPoint>> strokes;     // each stroke is a polyline
};

// --- LFF font file loader ---
class LffFont {
public:
    bool load(const std::string& filePath);

    const LffGlyph* getGlyph(int codePoint) const;
    double getLetterSpacing() const { return m_letterSpacing; }
    double getWordSpacing() const { return m_wordSpacing; }
    const std::string& getName() const { return m_name; }

    // List available .lff files in a directory
    static std::vector<std::string> listFonts(const std::string& directory);

private:
    std::string m_name;
    double m_letterSpacing = 3.0;
    double m_wordSpacing = 6.75;

    std::unordered_map<int, LffGlyph> m_glyphs;
    // Raw storage before reference resolution
    struct RawGlyph {
        int codePoint = 0;
        std::vector<std::string> lines;            // polyline lines + Cxxxx refs
    };

    void resolveReferences(std::unordered_map<int, RawGlyph>& rawGlyphs);
    void computeWidths();
    static std::vector<LffPoint> parsePolyline(const std::string& line);
    static LffPoint parsePoint(const std::string& token);
};

#endif // LFF_FONT_H
