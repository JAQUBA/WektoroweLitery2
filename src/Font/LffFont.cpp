// ============================================================================
// LffFont.cpp — LibreCAD Font Format (.lff) parser implementation
// ============================================================================
#include "LffFont.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <windows.h>

// ============================================================================
// Helpers
// ============================================================================
static std::string trimStr(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static int hexToInt(const std::string& hex) {
    return static_cast<int>(std::strtol(hex.c_str(), nullptr, 16));
}

// ============================================================================
// Parse a single point token: "x,y" or "x,y,Abulge"
// ============================================================================
LffPoint LffFont::parsePoint(const std::string& token) {
    LffPoint pt;
    std::string t = trimStr(token);
    if (t.empty()) return pt;

    // Find comma positions
    size_t c1 = t.find(',');
    if (c1 == std::string::npos) return pt;

    pt.x = std::stod(t.substr(0, c1));

    size_t c2 = t.find(',', c1 + 1);
    if (c2 == std::string::npos) {
        // No bulge: "x,y"
        pt.y = std::stod(t.substr(c1 + 1));
    } else {
        // Has bulge: "x,y,Abulge"
        pt.y = std::stod(t.substr(c1 + 1, c2 - c1 - 1));
        std::string bulgeStr = t.substr(c2 + 1);
        if (!bulgeStr.empty() && (bulgeStr[0] == 'A' || bulgeStr[0] == 'a'))
            bulgeStr = bulgeStr.substr(1);
        if (!bulgeStr.empty())
            pt.bulge = std::stod(bulgeStr);
    }
    return pt;
}

// ============================================================================
// Parse a polyline line: "x1,y1;x2,y2,Abulge;x3,y3"
// ============================================================================
std::vector<LffPoint> LffFont::parsePolyline(const std::string& line) {
    std::vector<LffPoint> points;
    std::istringstream ss(line);
    std::string token;
    while (std::getline(ss, token, ';')) {
        std::string t = trimStr(token);
        if (t.empty()) continue;
        points.push_back(parsePoint(t));
    }
    return points;
}

// ============================================================================
// Load an LFF font file
// ============================================================================
bool LffFont::load(const std::string& filePath) {
    m_glyphs.clear();
    m_name.clear();

    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::unordered_map<int, RawGlyph> rawGlyphs;
    int currentCP = -1;

    std::string line;
    while (std::getline(file, line)) {
        // Remove trailing \r
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        std::string trimmed = trimStr(line);
        if (trimmed.empty()) continue;

        // --- Header comment ---
        if (trimmed[0] == '#') {
            // Parse metadata from comments
            size_t colon = trimmed.find(':');
            if (colon != std::string::npos) {
                std::string key = trimStr(trimmed.substr(1, colon - 1));
                std::string val = trimStr(trimmed.substr(colon + 1));
                std::string keyLow = key;
                std::transform(keyLow.begin(), keyLow.end(), keyLow.begin(), ::tolower);

                if (keyLow == "name") m_name = val;
                else if (keyLow == "letterspacing") {
                    try { m_letterSpacing = std::stod(val); } catch (...) {}
                }
                else if (keyLow == "wordspacing") {
                    try { m_wordSpacing = std::stod(val); } catch (...) {}
                }
            }
            continue;
        }

        // --- Non-comment metadata line (e.g., "LetterSpacing: 3") ---
        if (trimmed.find("LetterSpacing") != std::string::npos ||
            trimmed.find("WordSpacing") != std::string::npos ||
            trimmed.find("LineSpacingFactor") != std::string::npos) {
            size_t colon = trimmed.find(':');
            if (colon != std::string::npos) {
                std::string key = trimStr(trimmed.substr(0, colon));
                std::string val = trimStr(trimmed.substr(colon + 1));
                std::string keyLow = key;
                std::transform(keyLow.begin(), keyLow.end(), keyLow.begin(), ::tolower);

                if (keyLow == "letterspacing") {
                    try { m_letterSpacing = std::stod(val); } catch (...) {}
                }
                else if (keyLow == "wordspacing") {
                    try { m_wordSpacing = std::stod(val); } catch (...) {}
                }
            }
            continue;
        }

        // --- Glyph header: [XXXX] optional_char ---
        if (trimmed[0] == '[') {
            size_t close = trimmed.find(']');
            if (close != std::string::npos) {
                std::string hexStr = trimmed.substr(1, close - 1);
                currentCP = hexToInt(hexStr);
                rawGlyphs[currentCP].codePoint = currentCP;
            }
            continue;
        }

        // --- Glyph data line (polyline or reference) ---
        if (currentCP >= 0) {
            rawGlyphs[currentCP].lines.push_back(trimmed);
        }
    }

    file.close();

    resolveReferences(rawGlyphs);
    computeWidths();

    return !m_glyphs.empty();
}

// ============================================================================
// Resolve Cxxxx references (glyph inheritance)
// ============================================================================
void LffFont::resolveReferences(std::unordered_map<int, RawGlyph>& rawGlyphs) {
    // Process each raw glyph into final glyph with resolved references
    for (auto& pair : rawGlyphs) {
        int cp = pair.first;
        const RawGlyph& raw = pair.second;

        LffGlyph glyph;
        glyph.codePoint = cp;

        for (const auto& line : raw.lines) {
            std::string trimmed = trimStr(line);
            if (trimmed.empty()) continue;

            if (trimmed[0] == 'C' || trimmed[0] == 'c') {
                // Reference to another glyph: Cxxxx
                std::string hexRef = trimmed.substr(1);
                int refCP = hexToInt(hexRef);

                // Resolve the referenced glyph (one level deep)
                auto it = rawGlyphs.find(refCP);
                if (it != rawGlyphs.end()) {
                    for (const auto& refLine : it->second.lines) {
                        std::string rt = trimStr(refLine);
                        if (rt.empty()) continue;
                        if (rt[0] == 'C' || rt[0] == 'c') {
                            // Nested reference (resolve one more level)
                            std::string hexRef2 = rt.substr(1);
                            int refCP2 = hexToInt(hexRef2);
                            auto it2 = rawGlyphs.find(refCP2);
                            if (it2 != rawGlyphs.end()) {
                                for (const auto& refLine2 : it2->second.lines) {
                                    std::string rt2 = trimStr(refLine2);
                                    if (!rt2.empty() && rt2[0] != 'C' && rt2[0] != 'c') {
                                        auto pts = parsePolyline(rt2);
                                        if (pts.size() >= 2)
                                            glyph.strokes.push_back(pts);
                                    }
                                }
                            }
                        } else {
                            auto pts = parsePolyline(rt);
                            if (pts.size() >= 2)
                                glyph.strokes.push_back(pts);
                        }
                    }
                }
            } else {
                // Regular polyline data
                auto pts = parsePolyline(trimmed);
                if (pts.size() >= 2)
                    glyph.strokes.push_back(pts);
            }
        }

        m_glyphs[cp] = glyph;
    }
}

// ============================================================================
// Compute glyph widths (max X extent)
// ============================================================================
void LffFont::computeWidths() {
    for (auto& pair : m_glyphs) {
        double maxX = 0.0;
        for (const auto& stroke : pair.second.strokes) {
            for (const auto& pt : stroke) {
                if (pt.x > maxX) maxX = pt.x;
            }
        }
        pair.second.width = maxX;
    }
}

// ============================================================================
// Get glyph by Unicode code point
// ============================================================================
const LffGlyph* LffFont::getGlyph(int codePoint) const {
    auto it = m_glyphs.find(codePoint);
    if (it != m_glyphs.end())
        return &it->second;
    return nullptr;
}

// ============================================================================
// List available .lff font files in a directory
// ============================================================================
std::vector<std::string> LffFont::listFonts(const std::string& directory) {
    std::vector<std::string> fonts;
    std::string searchPath = directory + "*.lff";
    std::wstring wSearch(searchPath.begin(), searchPath.end());

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(wSearch.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return fonts;

    do {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::wstring wName(findData.cFileName);
            std::string name(wName.begin(), wName.end());
            // Strip .lff extension for display name
            size_t dot = name.rfind('.');
            if (dot != std::string::npos)
                name = name.substr(0, dot);
            fonts.push_back(name);
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    std::sort(fonts.begin(), fonts.end());
    return fonts;
}
