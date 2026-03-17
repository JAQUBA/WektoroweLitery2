// ============================================================================
// DocumentParser.cpp — Layout file parser implementation
//
// File format (semicolon-separated lines):
//   p;diameter;stepover                     — laser mode params
//   f;diameter;stepover;idleZ;workZ;cutZ    — milling mode params
//   l                                       — new row (line break)
//   t;width;height;dx;dy;?;textH;cond;thick;text  — text-only plate
//   tw;width;height;dx;dy;?;textH;cond;thick;text — plate with frame
//   w;width;height                          — frame-only element
// ============================================================================
#include "DocumentParser.h"
#include "Nameplate.h"
#include "TableRow.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
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

Document DocumentParser::parseFile(const std::string& fileName,
                                    const std::string& csvDir) {
    Document doc;
    TableRow currentRow;
    Nameplate currentPlate;

    double xPos = 0.0;
    double yPos = 0.0;
    double maxRowHeight = 0.0;
    double frameW = 0.0, frameH = 0.0;
    double shiftX = 0.0, shiftY = 0.0;

    const char separator = ';';

    std::ifstream file(fileName);
    if (!file.is_open()) return doc;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty()) continue;

        auto row = split(line, separator);
        if (row.size() < 2 && toLower(row[0])[0] != 'l') continue;

        std::string cmd = toLower(row[0]);

        switch (cmd[0]) {
            case 'p': {
                // Laser mode parameters
                doc.laserMode = true;
                if (row.size() > 1) doc.millingDiameter_mm = std::stod(row[1]);
                if (row.size() > 2) doc.stepover_mm = std::stod(row[2]);
                break;
            }

            case 'f': {
                // Milling mode parameters
                if (row.size() > 1) doc.millingDiameter_mm = std::stod(row[1]);
                if (row.size() > 2) doc.stepover_mm = std::stod(row[2]);
                if (row.size() > 3) doc.idleDepth_mm = std::stod(row[3]);
                if (row.size() > 4) doc.workingDepth_mm = std::stod(row[4]);
                if (row.size() > 5) doc.cuttingDepth_mm = std::stod(row[5]);
                break;
            }

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

                if (row.size() > 1) frameW = std::stod(row[1]);
                if (row.size() > 2) frameH = std::stod(row[2]);
                if (row.size() > 3) shiftX = std::stod(row[3]);
                if (row.size() > 4) shiftY = std::stod(row[4]);
                // row[5] is unused/centering
                if (row.size() > 6) currentPlate.textHeight_mm = std::stod(row[6]);
                if (row.size() > 7) currentPlate.condensation = std::stod(row[7]) / 100.0;
                if (row.size() > 8) currentPlate.thickness = std::stod(row[8]);

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
                currentPlate.hasFrame = true;
                currentPlate.frameLeft_mm = xPos;
                currentPlate.frameBottom_mm = yPos;
                currentPlate.frameWidth_mm = frameW;
                currentPlate.frameHeight_mm = frameH;
                xPos += currentPlate.frameWidth_mm;
                maxRowHeight = currentPlate.frameHeight_mm;
                break;
            }
        }
    }

    return doc;
}
