// ============================================================================
// GCodeEngine.cpp — G-Code generator implementation
// ============================================================================
#include "GCodeEngine.h"
#include <fstream>
#include <cstdio>
#include <string>

static std::string fmtF2(double val) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", val);
    return buf;
}

static std::string fmtF3(double val) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.3f", val);
    return buf;
}

GCodeEngine::GCodeEngine() {
    init();
}

void GCodeEngine::init() {
    m_buffer.str("");
    m_buffer.clear();
}

void GCodeEngine::dumpToFile(const std::string& fileName) {
    std::ofstream file(fileName);
    if (file.is_open()) {
        file << m_buffer.str();
    }
}

// ============================================================================
// Line formatting
// ============================================================================
void GCodeEngine::appendLine(const std::string& content) {
    m_buffer << content << "\n";
}

void GCodeEngine::prolog(double safeHeight) {
    appendLine("G90");
    appendLine("F1000");
    appendLine("G21");

    if (!m_laserMode) {
        appendLine("G00 Z" + fmtF2(safeHeight));
    }

    appendLine("G00 X0.000 Y0.000");
}

void GCodeEngine::epilog(double safeHeight) {
    if (!m_laserMode) {
        appendLine("G00 Z" + fmtF2(safeHeight));
    } else {
        appendLine("M05");
    }

    appendLine("G00 X0.000 Y0.000");
    appendLine("M30");
}

void GCodeEngine::workingZ(double z) {
    char buf[64];
    if (!m_laserMode) {
        std::snprintf(buf, sizeof(buf), "G01 Z%s", fmtF2(z).c_str());
    } else {
        std::snprintf(buf, sizeof(buf), "M03");
    }
    appendLine(buf);
}

void GCodeEngine::workingXY(double x, double y) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "G01 X%s Y%s", fmtF3(x).c_str(), fmtF3(y).c_str());
    appendLine(buf);
}

void GCodeEngine::idleZ(double z) {
    char buf[64];
    if (!m_laserMode) {
        std::snprintf(buf, sizeof(buf), "G00 Z%s", fmtF2(z).c_str());
    } else {
        std::snprintf(buf, sizeof(buf), "M05");
    }
    appendLine(buf);
}

void GCodeEngine::idleXY(double x, double y) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "G00 X%s Y%s", fmtF3(x).c_str(), fmtF3(y).c_str());
    appendLine(buf);
}

// ============================================================================
// Export single nameplate frame
// ============================================================================
void GCodeEngine::exportSingleFrame(const std::string& fileName, const Document& doc,
                                     double left, double bottom,
                                     double width, double height) {
    init();
    idleZ(doc.materialThickness_mm + doc.safeHeight_mm);
    idleXY(left, bottom);

    workingZ(0.0);

    workingXY(left, bottom + height);
    workingXY(left + width, bottom + height);
    workingXY(left + width, bottom);
    workingXY(left, bottom);

    dumpToFile(fileName);
}

// ============================================================================
// Export entire document
// ============================================================================
void GCodeEngine::exportDocument(const std::string& fileName, const Document& doc) {
    init();
    m_laserMode = doc.laserMode;

    double safeZ = doc.materialThickness_mm + doc.safeHeight_mm;
    double textZ = doc.materialThickness_mm - doc.textDepth_mm;
    double cutZ  = 0.0;

    prolog(safeZ);

    for (const auto& row : doc.getRows()) {
        for (const auto& plate : row.getNameplates()) {
            double scale = 3000.0 / plate.textHeight_mm;

            // Draw frame if present
            if (plate.hasFrame) {
                idleZ(safeZ);
                idleXY(plate.frameLeft_mm, plate.frameBottom_mm);

                workingZ(cutZ);

                workingXY(plate.frameLeft_mm, plate.frameBottom_mm + plate.frameHeight_mm);
                workingXY(plate.frameLeft_mm + plate.frameWidth_mm,
                          plate.frameBottom_mm + plate.frameHeight_mm);
                workingXY(plate.frameLeft_mm + plate.frameWidth_mm, plate.frameBottom_mm);
                workingXY(plate.frameLeft_mm, plate.frameBottom_mm);
            }

            // Draw letter vectors
            for (const auto& letter : plate.getLetters()) {
                for (const auto& segment : letter.getPointCollections()) {
                    if (segment.empty()) continue;

                    Point2D pt = segment[0];
                    idleZ(safeZ);
                    idleXY(pt.X / scale, pt.Y / scale);

                    if (segment.size() > 1) {
                        workingZ(textZ);

                        for (size_t i = 1; i < segment.size(); i++) {
                            pt = segment[i];
                            workingXY(pt.X / scale, pt.Y / scale);
                        }
                    }
                }
            }
        }
    }

    epilog(safeZ);
    dumpToFile(fileName);
}
