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
    for (char* p = buf; *p; p++) if (*p == '.') *p = ',';
    return buf;
}

static std::string fmtF3(double val) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.3f", val);
    for (char* p = buf; *p; p++) if (*p == '.') *p = ',';
    return buf;
}

GCodeEngine::GCodeEngine() {
    init();
}

void GCodeEngine::init() {
    m_buffer.str("");
    m_buffer.clear();
    m_lineCounter = 0;
    m_lineJump = 5;
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

void GCodeEngine::prolog() {
    char buf[64];

    m_lineCounter += m_lineJump;
    std::snprintf(buf, sizeof(buf), "N%04d G90", m_lineCounter);
    appendLine(buf);

    m_lineCounter += m_lineJump;
    std::snprintf(buf, sizeof(buf), "N%04d G21", m_lineCounter);
    appendLine(buf);

    if (!m_laserMode) {
        m_lineCounter += m_lineJump;
        std::snprintf(buf, sizeof(buf), "N%04d G00 Z0,20", m_lineCounter);
        appendLine(buf);
    }

    m_lineCounter += m_lineJump;
    std::snprintf(buf, sizeof(buf), "N%04d G00 X0,000 Y0,000", m_lineCounter);
    appendLine(buf);
}

void GCodeEngine::epilog() {
    char buf[64];

    if (!m_laserMode) {
        m_lineCounter += m_lineJump;
        std::snprintf(buf, sizeof(buf), "N%04d G00 Z0,20", m_lineCounter);
        appendLine(buf);
    } else {
        std::snprintf(buf, sizeof(buf), "N%04d M05", m_lineCounter);
        appendLine(buf);
    }

    m_lineCounter += m_lineJump;
    std::snprintf(buf, sizeof(buf), "N%04d G00 X0,000 Y0,000", m_lineCounter);
    appendLine(buf);

    if (!m_laserMode) {
        m_lineCounter += m_lineJump;
        std::snprintf(buf, sizeof(buf), "N%04d G00 Z0,20", m_lineCounter);
        appendLine(buf);

        m_lineCounter += m_lineJump;
        std::snprintf(buf, sizeof(buf), "N%04d G00 X0 Y0", m_lineCounter);
        appendLine(buf);
    }

    m_lineCounter += m_lineJump;
    std::snprintf(buf, sizeof(buf), "N%04d M30", m_lineCounter);
    appendLine(buf);
}

void GCodeEngine::workingZ(double z) {
    char buf[64];
    m_lineCounter += m_lineJump;
    if (!m_laserMode) {
        std::snprintf(buf, sizeof(buf), "N%04d G01 Z%s", m_lineCounter, fmtF2(z).c_str());
    } else {
        std::snprintf(buf, sizeof(buf), "N%04d M03", m_lineCounter);
    }
    appendLine(buf);
}

void GCodeEngine::workingXY(double x, double y) {
    char buf[64];
    m_lineCounter += m_lineJump;
    std::snprintf(buf, sizeof(buf), "N%04d G01 X%s Y%s", m_lineCounter, fmtF3(x).c_str(), fmtF3(y).c_str());
    appendLine(buf);
}

void GCodeEngine::idleZ(double z) {
    char buf[64];
    m_lineCounter += m_lineJump;
    if (!m_laserMode) {
        std::snprintf(buf, sizeof(buf), "N%04d G00 Z%s", m_lineCounter, fmtF2(z).c_str());
    } else {
        std::snprintf(buf, sizeof(buf), "N%04d M05", m_lineCounter);
    }
    appendLine(buf);
}

void GCodeEngine::idleXY(double x, double y) {
    char buf[64];
    m_lineCounter += m_lineJump;
    std::snprintf(buf, sizeof(buf), "N%04d G00 X%s Y%s", m_lineCounter, fmtF3(x).c_str(), fmtF3(y).c_str());
    appendLine(buf);
}

// ============================================================================
// Export single nameplate frame
// ============================================================================
void GCodeEngine::exportSingleFrame(const std::string& fileName, const Document& doc,
                                     double left, double bottom,
                                     double width, double height) {
    init();
    idleZ(doc.idleDepth_mm);
    idleXY(left, bottom);

    workingZ(doc.cuttingDepth_mm);

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

    prolog();

    for (const auto& row : doc.getRows()) {
        for (const auto& plate : row.getNameplates()) {
            double scale = 3000.0 / plate.textHeight_mm;

            // Draw frame if present
            if (plate.hasFrame) {
                idleZ(doc.idleDepth_mm);
                idleXY(plate.frameLeft_mm, plate.frameBottom_mm);

                workingZ(doc.cuttingDepth_mm);

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
                    idleZ(doc.idleDepth_mm);
                    idleXY(pt.X / scale, pt.Y / scale);

                    if (segment.size() > 1) {
                        workingZ(doc.workingDepth_mm);

                        for (size_t i = 1; i < segment.size(); i++) {
                            pt = segment[i];
                            workingXY(pt.X / scale, pt.Y / scale);
                        }
                    }
                }
            }
        }
    }

    epilog();
    dumpToFile(fileName);
}
