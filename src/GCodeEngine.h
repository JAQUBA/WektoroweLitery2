// ============================================================================
// GCodeEngine.h — G-Code generator for CNC / laser engraving
// ============================================================================
#ifndef GCODE_ENGINE_H
#define GCODE_ENGINE_H

#include "Document.h"
#include "VectorLetterEngine.h"
#include <string>
#include <sstream>
#include <vector>

class GCodeEngine {
public:
    GCodeEngine();

    // Export entire document (all rows, plates, letters)
    void exportDocument(const std::string& fileName, const Document& doc);

    // Export single nameplate frame
    void exportSingleFrame(const std::string& fileName, const Document& doc,
                           double left = 0.0, double bottom = 0.0,
                           double width = 50.0, double height = 20.0);

private:
    int m_lineCounter = 0;
    int m_lineJump = 5;
    std::ostringstream m_buffer;
    bool m_laserMode = false;

    void init();
    void dumpToFile(const std::string& fileName);

    void prolog();
    void epilog();

    void idleZ(double z);
    void idleXY(double x, double y);
    void workingZ(double z);
    void workingXY(double x, double y);

    // G-Code line formatting helper
    void appendLine(const std::string& content);
};

#endif // GCODE_ENGINE_H
