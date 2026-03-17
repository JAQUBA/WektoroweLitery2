// ============================================================================
// DocumentParser.cpp — Layout file parser implementation
//
// File format (semicolon-separated lines):
//   # comment                                             — ignored line
//   l                                                     — new row (line break)
//   t;width;height;dx;dy;?;textH;cond;thick;text          — text-only plate
//   tw;width;height;dx;dy;?;textH;cond;thick;text         — plate with frame
//   w;width;height                                        — frame-only element
// ============================================================================
#include "DocumentParser.h"
#include "Nameplate.h"
#include "TableRow.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <stdexcept>

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static double parseDouble(const std::string& s) {
    std::string tmp = trim(s);
    std::replace(tmp.begin(), tmp.end(), ',', '.');
    return std::stod(tmp);
}

static std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> tokens;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, sep))
        tokens.push_back(token);
    return tokens;
}

static std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

static void stripUtf8BOM(std::string& s) {
    if (s.size() >= 3 &&
        static_cast<unsigned char>(s[0]) == 0xEF &&
        static_cast<unsigned char>(s[1]) == 0xBB &&
        static_cast<unsigned char>(s[2]) == 0xBF) {
        s.erase(0, 3);
    }
}

// --- Shared parsing logic (works on any std::istream) ---
static Document parseStreamImpl(std::istream& input,
                                const std::string& csvDir,
                                double diameter,
                                double stepover,
                                std::vector<int>* errorLines = nullptr) {
    Document doc;
    doc.millingDiameter_mm = diameter;
    doc.stepover_mm = stepover;
    TableRow currentRow;
    Nameplate currentPlate;

    double xPos = 0.0;
    double yPos = 0.0;
    double maxRowHeight = 0.0;
    double frameW = 0.0, frameH = 0.0;
    double shiftX = 0.0, shiftY = 0.0;

    const char separator = ';';

    std::string line;
    bool firstLine = true;
    int lineNumber = 0;
    while (std::getline(input, line)) {
        if (firstLine) {
            stripUtf8BOM(line);
            firstLine = false;
        }
        int currentLine = lineNumber++;
        line = trim(line);
        if (line.empty()) continue;

        try {

        auto row = split(line, separator);
        if (row.size() < 2 && toLower(row[0])[0] != 'l') continue;

        std::string cmd = toLower(row[0]);

        switch (cmd[0]) {
            case 'l': {
                // New row
                doc.addRow(currentRow);
                currentRow = TableRow();
                xPos = 0;
                yPos += maxRowHeight;
                break;
            }

            case 't': {
                // Nameplate (t = text only, tw = text + frame)
                currentPlate = Nameplate();
                currentPlate.diameter = doc.millingDiameter_mm;
                currentPlate.stepover = doc.stepover_mm;

                if (row.size() > 1) frameW = parseDouble(row[1]);
                if (row.size() > 2) frameH = parseDouble(row[2]);
                if (row.size() > 3) shiftX = parseDouble(row[3]);
                if (row.size() > 4) shiftY = parseDouble(row[4]);
                // row[5] is unused/centering
                if (row.size() > 6) currentPlate.textHeight_mm = parseDouble(row[6]);
                if (row.size() > 7) currentPlate.condensation = parseDouble(row[7]) / 100.0;
                if (row.size() > 8) currentPlate.thickness = parseDouble(row[8]);

                std::string text;
                if (row.size() > 9) text = row[9];

                if (cmd.size() > 1 && cmd[1] == 'w') {
                    // Plate with frame
                    currentPlate.hasFrame = true;
                    currentPlate.frameLeft_mm = xPos;
                    currentPlate.frameBottom_mm = yPos;
                    currentPlate.frameWidth_mm = frameW;
                    currentPlate.frameHeight_mm = frameH;
                    xPos += currentPlate.frameWidth_mm;
                    maxRowHeight = currentPlate.frameHeight_mm;
                } else {
                    // Text only (no frame)
                    currentPlate.hasFrame = false;
                    currentPlate.frameLeft_mm = xPos + shiftX;
                    currentPlate.frameBottom_mm = yPos + shiftY;
                    currentPlate.frameWidth_mm = frameW;
                    currentPlate.frameHeight_mm = frameH;
                }

                currentPlate.appendText(text, csvDir);
                currentRow.addNameplate(currentPlate);
                break;
            }

            case 'w': {
                // Frame-only element
                currentPlate = Nameplate();
                currentPlate.hasFrame = true;
                currentPlate.frameLeft_mm = xPos;
                currentPlate.frameBottom_mm = yPos;
                currentPlate.frameWidth_mm = frameW;
                currentPlate.frameHeight_mm = frameH;
                currentRow.addNameplate(currentPlate);
                xPos += frameW;
                maxRowHeight = frameH;
                break;
            }
        }

        } catch (const std::exception&) {
            if (errorLines) errorLines->push_back(currentLine);
        }
    }

    // Flush last row if it has any nameplates (fixes missing final row
    // when the file does not end with an 'L;' command).
    if (!currentRow.getNameplates().empty()) {
        doc.addRow(currentRow);
    }

    return doc;
}

// --- Public API ---

Document DocumentParser::parseFile(const std::string& fileName,
                                    const std::string& csvDir,
                                    double diameter,
                                    double stepover) {
    std::ifstream file(fileName);
    if (!file.is_open()) return Document();
    return parseStreamImpl(file, csvDir, diameter, stepover, nullptr);
}

Document DocumentParser::parseString(const std::string& content,
                                      const std::string& csvDir,
                                      double diameter,
                                      double stepover,
                                      std::vector<int>* errorLines) {
    std::istringstream stream(content);
    return parseStreamImpl(stream, csvDir, diameter, stepover, errorLines);
}
