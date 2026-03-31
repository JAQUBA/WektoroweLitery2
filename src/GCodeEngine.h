// ============================================================================
// GCodeEngine.h — G-Code generator for CNC / laser engraving
//                  with G2/G3 arc fitting and path optimization for GRBL
// ============================================================================
#ifndef GCODE_ENGINE_H
#define GCODE_ENGINE_H

#include "Document.h"
#include "VectorLetterEngine.h"
#include <string>
#include <sstream>
#include <vector>

// --- Optimized G-code move (line or arc) ---
// NOTE: Arc center offsets ci/cj (I/J in G-code) are relative to the arc start point.
//       This assumes the controller is in incremental arc center mode (GRBL default).
//       On non-GRBL controllers that default to absolute arc center mode (G90.1),
//       the prolog sets G91.1 to ensure incremental I/J interpretation.
struct GCodeMove {
    enum Type { LINE, ARC_CW, ARC_CCW };
    Type type = LINE;
    double x = 0.0, y = 0.0;       // endpoint (mm, absolute)
    double ci = 0.0, cj = 0.0;     // I,J center offsets for arcs (relative to start)
};

class GCodeEngine {
public:
    GCodeEngine();

    // Export entire document (all rows, plates, letters)
    void exportDocument(const std::string& fileName, const Document& doc);

    // Export single nameplate frame
    void exportSingleFrame(const std::string& fileName, const Document& doc,
                           double left = 0.0, double bottom = 0.0,
                           double width = 50.0, double height = 20.0);

    // Optimization stats (call after exportDocument)
    int getTotalRawPoints() const { return m_totalRawPoints; }
    int getTotalMoves() const { return m_totalMoves; }
    int getArcMoves() const { return m_arcMoves; }
    int getReducedPoints() const { return m_reducedPoints; }

private:
    std::ostringstream m_buffer;
    bool m_laserMode = false;
    double m_feedXY = 300.0;
    double m_feedZ  = 100.0;
    bool m_needFeedXY = true;   // emit F on first working XY move after Z plunge

    // Optimization statistics
    int m_totalRawPoints = 0;
    int m_totalMoves = 0;
    int m_arcMoves = 0;
    int m_reducedPoints = 0;

    void init();
    void dumpToFile(const std::string& fileName);

    void prolog(double safeHeight);
    void epilog(double safeHeight);

    void idleZ(double z);
    void idleXY(double x, double y);
    void workingZ(double z);
    void workingXY(double x, double y);

    // G-Code line formatting helper
    void appendLine(const std::string& content);

    // --- Path optimization (G2/G3 arcs + collinear reduction) ---

    // Tolerance constants (mm)
    static constexpr double ARC_TOLERANCE      = 0.01;    // max deviation from arc (all points)
    static constexpr double COLLINEAR_TOLERANCE = 0.005;   // max deviation from line
    static constexpr double MIN_MOVE_LEN        = 0.003;   // skip shorter moves
    static constexpr double MAX_ARC_RADIUS      = 100.0;   // larger arcs → lines
    static constexpr double MIN_ARC_RADIUS      = 0.05;    // smaller arcs → lines
    static constexpr double CORNER_THRESHOLD    = 0.45;    // ~25° — turn angle to split path (radians)
    static constexpr int    MIN_ARC_POINTS      = 4;       // minimum points to attempt arc fit

    // Build optimized move list from raw PointCollection
    std::vector<GCodeMove> optimizePath(const PointCollection& points, double scale);

    // Emit optimized moves as G-code
    void emitOptimizedPath(const std::vector<GCodeMove>& moves);

    // Greedily fit the longest valid arc starting at pts[from], up to pts[maxTo].
    // Returns end index of the arc (>= from+MIN_ARC_POINTS-1), or 0 if no arc found.
    size_t tryFitArc(const std::vector<Point2D>& pts, size_t from, size_t maxTo,
                     GCodeMove& move);

    // Try collinear reduction on segment [from..to]. Returns the farthest
    // endpoint index where all intermediate points are within tolerance.
    size_t tryCollinearReduce(const std::vector<Point2D>& pts, size_t from, size_t to);

    // Fit a circle through 3 points; returns false if collinear
    static bool fitCircle3(double x1, double y1, double x2, double y2,
                           double x3, double y3,
                           double& ox, double& oy, double& r);

    // Perpendicular distance from point (px,py) to line (ax,ay)→(bx,by)
    static double pointToLineDist(double px, double py,
                                  double ax, double ay, double bx, double by);

    // Normalized turn angle at point B in path A→B→C (positive = CCW)
    static double turnAngle(double ax, double ay, double bx, double by,
                            double cx, double cy);
};

#endif // GCODE_ENGINE_H
