// ============================================================================
// GCodeEngine.cpp — G-Code generator implementation
//                    with G2/G3 arc fitting and path optimization for GRBL
// ============================================================================
#include "GCodeEngine.h"
#include <Common/ArcMath.h>
#include <fstream>
#include <cstdio>
#include <cmath>
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
    m_totalRawPoints = 0;
    m_totalMoves = 0;
    m_arcMoves = 0;
    m_reducedPoints = 0;
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
    appendLine("G21 G90 G17 G94 G54");
    appendLine("G91.1");   // incremental arc center mode (I/J relative to start)

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
        std::snprintf(buf, sizeof(buf), "G01 Z%s F%s", fmtF2(z).c_str(), fmtF2(m_feedZ).c_str());
    } else {
        std::snprintf(buf, sizeof(buf), "M03");
    }
    appendLine(buf);
}

void GCodeEngine::workingXY(double x, double y) {
    char buf[128];
    if (m_needFeedXY) {
        std::snprintf(buf, sizeof(buf), "G01 X%s Y%s F%s", fmtF3(x).c_str(), fmtF3(y).c_str(), fmtF2(m_feedXY).c_str());
        m_needFeedXY = false;
    } else {
        std::snprintf(buf, sizeof(buf), "G01 X%s Y%s", fmtF3(x).c_str(), fmtF3(y).c_str());
    }
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
    m_feedXY = doc.feedXY_mm;
    m_feedZ  = doc.feedZ_mm;

    double safeZ = doc.materialThickness_mm + doc.safeHeight_mm;
    double textZ = doc.materialThickness_mm - doc.textDepth_mm;
    if (textZ < 0.0) textZ = 0.0;
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
                m_needFeedXY = true;

                workingXY(plate.frameLeft_mm, plate.frameBottom_mm + plate.frameHeight_mm);
                workingXY(plate.frameLeft_mm + plate.frameWidth_mm,
                          plate.frameBottom_mm + plate.frameHeight_mm);
                workingXY(plate.frameLeft_mm + plate.frameWidth_mm, plate.frameBottom_mm);
                workingXY(plate.frameLeft_mm, plate.frameBottom_mm);
            }

            // Draw letter vectors (optimized with G2/G3 arcs)
            for (const auto& letter : plate.getLetters()) {
                for (const auto& segment : letter.getPointCollections()) {
                    if (segment.empty()) continue;

                    m_totalRawPoints += static_cast<int>(segment.size());

                    Point2D pt = segment[0];
                    idleZ(safeZ);
                    idleXY(pt.X / scale, pt.Y / scale);

                    if (segment.size() > 1) {
                        workingZ(textZ);
                        m_needFeedXY = true;

                        auto moves = optimizePath(segment, scale);
                        emitOptimizedPath(moves);
                    }
                }
            }
        }
    }

    // Optimization stats comment
    {
        char stats[256];
        int lineMoves = m_totalMoves - m_arcMoves;
        std::snprintf(stats, sizeof(stats),
            "; Optimized: %d moves (%d arcs, %d lines) from %d raw points (%d short moves / near-duplicate points removed)",
            m_totalMoves, m_arcMoves, lineMoves, m_totalRawPoints, m_reducedPoints);
        appendLine(stats);
    }

    epilog(safeZ);
    dumpToFile(fileName);
}

// ============================================================================
// Greedily fit the longest valid arc starting at pts[from], up to pts[maxTo].
// Returns end index of arc (>= from + MIN_ARC_POINTS - 1) or 0 if no arc.
// ============================================================================
size_t GCodeEngine::tryFitArc(const std::vector<Point2D>& pts, size_t from, size_t maxTo,
                               GCodeMove& move) {
    // Need at least 3 points to define a circle
    if (from + 2 > maxTo) return 0;

    // Initial 3-point circle fit
    double ox, oy, r;
    if (!arcmath::fitCircle3(pts[from].X, pts[from].Y,
                             pts[from + 1].X, pts[from + 1].Y,
                             pts[from + 2].X, pts[from + 2].Y,
                             ox, oy, r))
        return 0;

    if (r < MIN_ARC_RADIUS || r > MAX_ARC_RADIUS)
        return 0;

    // Determine CW/CCW from cross product of first 3 points
    double cross = (pts[from + 1].X - pts[from].X) * (pts[from + 2].Y - pts[from].Y) -
                   (pts[from + 1].Y - pts[from].Y) * (pts[from + 2].X - pts[from].X);
    bool ccw = (cross > 0);

    // Greedily extend — add points while they stay on the circle
    constexpr double kPi = 3.14159265358979323846;
    size_t arcEnd = from + 2;
    double prevAngle = std::atan2(pts[from + 2].Y - oy, pts[from + 2].X - ox);

    for (size_t j = from + 3; j <= maxTo; j++) {
        double dx = pts[j].X - ox;
        double dy = pts[j].Y - oy;
        double dist = std::sqrt(dx * dx + dy * dy);
        if (std::fabs(dist - r) > ARC_TOLERANCE) break;

        // Check angular monotonicity
        double angle = std::atan2(dy, dx);
        double delta = angle - prevAngle;
        while (delta > kPi) delta -= 2.0 * kPi;
        while (delta < -kPi) delta += 2.0 * kPi;

        if (ccw && delta < -0.01) break;
        if (!ccw && delta > 0.01) break;

        prevAngle = angle;
        arcEnd = j;
    }

    size_t arcLen = arcEnd - from + 1;
    if (arcLen < static_cast<size_t>(MIN_ARC_POINTS)) return 0;

    // Refit circle with first / mid / end for best accuracy
    size_t mid = (from + arcEnd) / 2;
    if (!arcmath::fitCircle3(pts[from].X, pts[from].Y,
                             pts[mid].X, pts[mid].Y,
                             pts[arcEnd].X, pts[arcEnd].Y,
                             ox, oy, r))
        return 0;

    if (r < MIN_ARC_RADIUS || r > MAX_ARC_RADIUS)
        return 0;

    // Validate ALL points on the refitted circle
    for (size_t k = from; k <= arcEnd; k++) {
        double dx = pts[k].X - ox;
        double dy = pts[k].Y - oy;
        double dist = std::sqrt(dx * dx + dy * dy);
        if (std::fabs(dist - r) > ARC_TOLERANCE)
            return 0;
    }

    // GRBL radius check: |r_start - r_end| must be small
    {
        double dx1 = pts[from].X - ox, dy1 = pts[from].Y - oy;
        double dx2 = pts[arcEnd].X - ox, dy2 = pts[arcEnd].Y - oy;
        double r1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
        double r2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
        double rdiff = std::fabs(r1 - r2);
        double maxDiff = (0.001 * r > 0.005) ? 0.001 * r : 0.005;
        if (rdiff > maxDiff)
            return 0;
    }

    // Re-determine direction after refit
    double turnSum = 0.0;
    for (size_t k = from + 1; k < arcEnd; k++) {
        turnSum += arcmath::turnAngle(pts[k-1].X, pts[k-1].Y,
                          pts[k].X, pts[k].Y,
                          pts[k+1].X, pts[k+1].Y);
    }
    ccw = (turnSum > 0);

    move.type = ccw ? GCodeMove::ARC_CCW : GCodeMove::ARC_CW;
    move.x = pts[arcEnd].X;
    move.y = pts[arcEnd].Y;
    move.ci = ox - pts[from].X;
    move.cj = oy - pts[from].Y;

    return arcEnd;
}

// ============================================================================
// Try collinear reduction: find farthest point from 'from' to 'maxTo'
// where all intermediate points are within tolerance of the straight line
// ============================================================================
size_t GCodeEngine::tryCollinearReduce(const std::vector<Point2D>& pts,
                                        size_t from, size_t maxTo) {
    size_t best = from + 1;
    for (size_t j = from + 2; j <= maxTo; j++) {
        bool allOk = true;
        for (size_t k = from + 1; k < j; k++) {
            double d = arcmath::pointToLineDist(pts[k].X, pts[k].Y,
                                                pts[from].X, pts[from].Y,
                                                pts[j].X, pts[j].Y);
            if (d > COLLINEAR_TOLERANCE) {
                allOk = false;
                break;
            }
        }
        if (allOk) best = j;
        else break;
    }
    return best;
}

// ============================================================================
// Build optimized move list from raw PointCollection
//
// Algorithm:
//   1. Scale to mm, remove duplicate consecutive points
//   2. Compute turn angle at each interior point
//   3. Split path at "corners" (|turn| > CORNER_THRESHOLD)
//   4. For each smooth segment between corners:
//      a. If turns are consistent (same sign) and enough points → try arc fit
//      b. Otherwise → collinear reduction (merge straight runs into one G01)
// ============================================================================
std::vector<GCodeMove> GCodeEngine::optimizePath(const PointCollection& rawPoints,
                                                  double scale) {
    std::vector<GCodeMove> moves;
    if (rawPoints.size() < 2) return moves;

    // --- Step 1: Scale to mm and remove near-duplicate points ---
    std::vector<Point2D> pts;
    pts.reserve(rawPoints.size());
    for (const auto& p : rawPoints)
        pts.push_back(Point2D(p.X / scale, p.Y / scale));

    std::vector<Point2D> filtered;
    filtered.reserve(pts.size());
    filtered.push_back(pts[0]);
    for (size_t k = 1; k < pts.size(); k++) {
        double dx = pts[k].X - filtered.back().X;
        double dy = pts[k].Y - filtered.back().Y;
        if (dx * dx + dy * dy >= MIN_MOVE_LEN * MIN_MOVE_LEN)
            filtered.push_back(pts[k]);
    }
    m_reducedPoints += static_cast<int>(pts.size() - filtered.size());

    if (filtered.size() < 2) return moves;

    size_t N = filtered.size();

    // --- Step 2: Compute turn angles at interior points ---
    // turnAngles[k] = turn angle at filtered[k] (only valid for k=1..N-2)
    std::vector<double> turns(N, 0.0);
    for (size_t k = 1; k + 1 < N; k++) {
        turns[k] = arcmath::turnAngle(filtered[k-1].X, filtered[k-1].Y,
                          filtered[k].X, filtered[k].Y,
                          filtered[k+1].X, filtered[k+1].Y);
    }

    // --- Step 3: Find corner indices (sharp turns that break smooth regions) ---
    // Corners split the path into segments where arc fitting makes sense
    std::vector<size_t> corners;
    corners.push_back(0);  // path start
    for (size_t k = 1; k + 1 < N; k++) {
        if (std::fabs(turns[k]) > CORNER_THRESHOLD)
            corners.push_back(k);
    }
    corners.push_back(N - 1);  // path end

    // --- Step 4: Process smooth segments with greedy arc detection ---
    size_t emittedPos = 0;

    for (size_t ci = 0; ci + 1 < corners.size(); ci++) {
        size_t segFrom = corners[ci];
        size_t segTo   = corners[ci + 1];

        if (segFrom < emittedPos) segFrom = emittedPos;
        if (segTo <= segFrom) continue;

        size_t pos = segFrom;
        while (pos < segTo) {
            // Try greedy arc fit starting at pos
            GCodeMove arcMove;
            size_t arcEnd = tryFitArc(filtered, pos, segTo, arcMove);
            if (arcEnd > 0) {
                moves.push_back(arcMove);
                m_arcMoves++;
                m_totalMoves++;
                emittedPos = arcEnd;
                pos = arcEnd;
                continue;
            }

            // Fallback: collinear reduction
            size_t lineEnd = tryCollinearReduce(filtered, pos, segTo);

            GCodeMove m;
            m.type = GCodeMove::LINE;
            m.x = filtered[lineEnd].X;
            m.y = filtered[lineEnd].Y;
            moves.push_back(m);
            m_totalMoves++;

            emittedPos = lineEnd;
            pos = lineEnd;
        }
    }

    return moves;
}

// ============================================================================
// Emit optimized G-code moves (G01 lines + G02/G03 arcs)
// ============================================================================
void GCodeEngine::emitOptimizedPath(const std::vector<GCodeMove>& moves) {
    char buf[128];
    for (const auto& m : moves) {
        const char* fSuffix = "";
        std::string fStr;
        if (m_needFeedXY) {
            fStr = " F" + fmtF2(m_feedXY);
            fSuffix = fStr.c_str();
            m_needFeedXY = false;
        }
        switch (m.type) {
        case GCodeMove::LINE:
            std::snprintf(buf, sizeof(buf), "G01 X%s Y%s%s",
                          fmtF3(m.x).c_str(), fmtF3(m.y).c_str(), fSuffix);
            break;
        case GCodeMove::ARC_CW:
            std::snprintf(buf, sizeof(buf), "G02 X%s Y%s I%s J%s%s",
                          fmtF3(m.x).c_str(), fmtF3(m.y).c_str(),
                          fmtF3(m.ci).c_str(), fmtF3(m.cj).c_str(), fSuffix);
            break;
        case GCodeMove::ARC_CCW:
            std::snprintf(buf, sizeof(buf), "G03 X%s Y%s I%s J%s%s",
                          fmtF3(m.x).c_str(), fmtF3(m.y).c_str(),
                          fmtF3(m.ci).c_str(), fmtF3(m.cj).c_str(), fSuffix);
            break;
        }
        appendLine(buf);
    }
}
