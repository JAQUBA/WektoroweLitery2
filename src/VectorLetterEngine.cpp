// ============================================================================
// VectorLetterEngine.cpp — Vector letter engine implementation
// ============================================================================
#include "VectorLetterEngine.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <stdexcept>

// ============================================================================
// Constructors
// ============================================================================
VectorLetterEngine::VectorLetterEngine() {}

VectorLetterEngine::VectorLetterEngine(const std::string& fileName, char separator,
                                       double scale, double yScreenOffset,
                                       double height, double thickness,
                                       double diameter, double stepover)
{
    m_yScreenOffset = yScreenOffset;
    m_scale = scale;
    m_thickness = static_cast<int>(thickness * 30.0);
    m_diameter = static_cast<int>(diameter * 3000.0 / height);
    m_stepover = static_cast<int>(stepover * 3000.0 / height);
    importFromCSV(fileName, separator);
}

// ============================================================================
// Draw segment axis (from back to front)
// ============================================================================
void VectorLetterEngine::drawSegmentAxis(PointCollection& points,
                                          const std::vector<VectorPoint>& data) {
    int lastIdx = static_cast<int>(data.size()) - 1;
    for (int i = lastIdx; i >= 0; i--) {
        m_xb = data[i].x;
        m_yb = data[i].y;
        points.push_back(Point2D(m_xb * m_scale, m_yb * m_scale));
    }
}

// ============================================================================
// Compute quadrant for angle calculation
// ============================================================================
void VectorLetterEngine::computeQuadrant(int i, double dx, double dy,
                                          std::vector<VectorPoint>& data) {
    double dyOverDx = std::abs(dy) / std::abs(dx);
    double aa = std::atan(dyOverDx);

    if (dx >= 0.0) {         // quadrant 1 & 4
        if (dy >= 0.0)       // quadrant 1
            data[i].alphaPrimary = aa;
        else                 // quadrant 4
            data[i].alphaPrimary = (2.0 * M_PI) - aa;
    } else {                 // quadrant 2 & 3
        if (dy >= 0.0)       // quadrant 2
            data[i].alphaPrimary = M_PI - aa;
        else                 // quadrant 3
            data[i].alphaPrimary = M_PI + aa;
    }
}

// ============================================================================
// Compute primary angles (alpha_p) for all points in segment
// ============================================================================
void VectorLetterEngine::computeAlphaPrimary(std::vector<VectorPoint>& data) {
    int lastIdx = static_cast<int>(data.size()) - 1;
    for (int i = 0; i < lastIdx; i++) {
        computeQuadrant(i,
                        (data[i + 1].y - data[i].y) * -1.0,
                        (data[i + 1].x - data[i].x),
                        data);
    }
    data[lastIdx].alphaPrimary = data[lastIdx - 1].alphaPrimary;
}

// ============================================================================
// Compute mean angles (alpha_m) for all points in segment
// ============================================================================
void VectorLetterEngine::computeAlphaMean(std::vector<VectorPoint>& data) {
    int lastIdx = static_cast<int>(data.size()) - 1;
    double cosAlpha = std::cos(data[0].alphaPrimary);

    if (data[0].hasSerif) {
        data[0].alphaMean = (cosAlpha >= 0.0) ? 0.0 : M_PI;
        data[0].widthFactor = 1.0 / cosAlpha;
    } else {
        data[0].alphaMean = data[0].alphaPrimary;
    }

    double d270 = (M_PI * 3.0) / 2.0;

    for (int i = 1; i < lastIdx; i++) {
        data[i].alphaMean = (data[i].alphaPrimary + data[i - 1].alphaPrimary) / 2.0;

        if (data[i].alphaMean >= (2.0 * M_PI))
            data[i].alphaMean -= M_PI;

        if (((data[i].alphaPrimary <= M_PI) || (data[i - 1].alphaPrimary <= M_PI)) &&
            ((data[i].alphaPrimary >= d270) || (data[i - 1].alphaPrimary >= d270)))
            data[i].alphaMean += M_PI;
    }

    data[lastIdx].alphaPrimary = data[lastIdx - 1].alphaPrimary;
    cosAlpha = std::cos(data[lastIdx].alphaPrimary);

    if (data[lastIdx].hasSerif) {
        data[lastIdx].alphaMean = (cosAlpha >= 0.0) ? 0.0 : M_PI;
        data[lastIdx].widthFactor = 1.0 / cosAlpha;
    } else {
        data[lastIdx].alphaMean = data[lastIdx].alphaPrimary;
    }
}

// ============================================================================
// Helper: add point if not a terminator
// ============================================================================
static inline void addPointIfNotTerminator(PointCollection& points, double xb, double yb,
                                            double scale, bool isTerminator) {
    if (!isTerminator)
        points.push_back(Point2D(xb * scale, yb * scale));
}

// ============================================================================
// Draw segment envelope (forward + endcap + reverse + startcap)
// ============================================================================
void VectorLetterEngine::drawSegmentEnvelope(PointCollection& points,
                                              std::vector<VectorPoint>& data,
                                              double pw) {
    double alpha, alphaP, dx, dy, px, py, px2, py2, f, z;
    int lastIdx = static_cast<int>(data.size()) - 1;

    // --- Forward pass: draw envelope on one side ---
    for (int i = 0; i <= lastIdx; i++) {
        alpha = data[i].alphaMean;
        alphaP = std::abs(data[i].alphaMean - data[i].alphaPrimary);
        m_xb = data[i].x + (std::cos(alpha) * pw / std::cos(alphaP));
        m_yb = data[i].y + (std::sin(alpha) * pw / std::cos(alphaP));
        points.push_back(Point2D(m_xb * m_scale, m_yb * m_scale));
    }

    // --- End cap ---
    alphaP = std::abs(data[lastIdx].alphaMean - data[lastIdx].alphaPrimary);

    if (data[lastIdx].hasSerif) {
        alpha = data[lastIdx].alphaPrimary;
        f = pw * std::abs(std::tan(alpha));
        z = std::sqrt((f * f) + (pw * pw));
        bool notTerm = !data[lastIdx].isTerminator;

        if (alpha > M_PI) {
            if (alpha > (M_PI * 1.5)) { // quadrant 4
                m_xb = data[lastIdx].x - f + z;
                m_yb = data[lastIdx].y - pw;
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
                m_xb -= (2 * z);
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
            } else { // quadrant 3
                m_xb = data[lastIdx].x - f - z;
                m_yb = data[lastIdx].y + pw;
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
                m_xb += (2 * z);
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
            }
        } else {
            if (alpha > (M_PI * 0.5)) { // quadrant 2
                m_xb = data[lastIdx].x + f - z;
                m_yb = data[lastIdx].y + pw;
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
                m_xb += (2 * z);
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
            } else { // quadrant 1
                m_xb = data[lastIdx].x + f + z;
                m_yb = data[lastIdx].y - pw;
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
                m_xb -= (2 * z);
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
            }
        }
    } else {
        alpha = data[lastIdx].alphaMean;
        dx = std::cos(alpha) * pw / std::cos(alphaP);
        dy = std::sin(alpha) * pw / std::cos(alphaP);

        // perpendicular
        py = dx * -1.0;
        px = dy;
        m_xb += px;
        m_yb += py;
        addPointIfNotTerminator(points, m_xb, m_yb, m_scale, data[lastIdx].isTerminator);

        // perpendicular
        py2 = px * -2.0;
        px2 = py * 2.0;
        m_xb += px2;
        m_yb += py2;
        addPointIfNotTerminator(points, m_xb, m_yb, m_scale, data[lastIdx].isTerminator);
    }

    // --- Reverse pass: draw envelope on the other side ---
    for (int i = lastIdx; i >= 0; i--) {
        alpha = data[i].alphaMean;
        alpha = (alpha > M_PI) ? alpha - M_PI : alpha + M_PI;

        alphaP = std::abs(data[i].alphaMean - data[i].alphaPrimary);
        m_xb = data[i].x + (std::cos(alpha) * pw / std::cos(alphaP));
        m_yb = data[i].y + (std::sin(alpha) * pw / std::cos(alphaP));
        points.push_back(Point2D(m_xb * m_scale, m_yb * m_scale));
    }

    // --- Start cap ---
    if (data[0].hasSerif) {
        alpha = data[0].alphaPrimary;
        f = pw * std::abs(std::tan(alpha));
        z = std::sqrt((f * f) + (pw * pw));
        bool notTerm = !data[lastIdx].isTerminator;

        if (alpha > M_PI) {
            if (alpha > (M_PI * 1.5)) { // quadrant 4
                m_xb = data[0].x + f - z;
                m_yb = data[0].y + pw;
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
                m_xb += (2 * z);
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
                m_xb = data[0].x + z;
                m_yb = data[0].y;
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
            } else { // quadrant 3
                m_xb = data[0].x + f + z;
                m_yb = data[0].y - pw;
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
                m_xb -= (2 * z);
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
                m_xb = data[0].x - z;
                m_yb = data[0].y;
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
            }
        } else {
            if (alpha > (M_PI * 0.5)) { // quadrant 2
                m_xb = data[0].x - f + z;
                m_yb = data[0].y - pw;
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
                m_xb -= (2 * z);
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
                m_xb = data[0].x - z;
                m_yb = data[0].y;
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
            } else { // quadrant 1
                m_xb = data[0].x - f - z;
                m_yb = data[0].y + pw;
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
                m_xb += (2 * z);
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
                m_xb = data[0].x + z;
                m_yb = data[0].y;
                addPointIfNotTerminator(points, m_xb, m_yb, m_scale, !notTerm);
            }
        }
    } else {
        alphaP = std::abs(data[0].alphaMean - data[0].alphaPrimary);
        alpha = data[0].alphaMean;
        alpha = (alpha > M_PI) ? alpha - M_PI : alpha + M_PI;

        dx = std::cos(alpha) * pw / std::cos(alphaP);
        dy = std::sin(alpha) * pw / std::cos(alphaP);

        // perpendicular
        py = dx * -1.0;
        px = dy;
        m_xb += px;
        m_yb += py;
        addPointIfNotTerminator(points, m_xb, m_yb, m_scale, data[lastIdx].isTerminator);

        // perpendicular
        py2 = px * -2.0;
        px2 = py * 2.0;
        m_xb += px2;
        m_yb += py2;
        addPointIfNotTerminator(points, m_xb, m_yb, m_scale, data[lastIdx].isTerminator);

        // perpendicular
        m_xb += py2 * 0.5;
        m_yb += px2 * -0.5;
        addPointIfNotTerminator(points, m_xb, m_yb, m_scale, data[lastIdx].isTerminator);
    }
}

// ============================================================================
// CSV Import — reads vector letter data from CSV file
// ============================================================================
void VectorLetterEngine::importFromCSV(const std::string& fileName, char separator) {
    bool newSegment = false;

    std::ifstream file(fileName);
    if (!file.is_open()) return;

    std::vector<VectorPoint> segment;
    std::string line;

    while (std::getline(file, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        // Split by separator
        std::vector<std::string> row;
        std::istringstream ss(line);
        std::string token;
        while (std::getline(ss, token, separator))
            row.push_back(token);

        if (row.size() > 1) {
            if (newSegment) {
                newSegment = false;
                segments.push_back(segment);
                segment.clear();
            }

            VectorPoint point;
            point.x = std::stod(row[0]);
            point.y = std::stod(row[1]);

            if (maxX < point.x) maxX = point.x;

            if (row.size() >= 3) {
                point.options = row[2];
                std::string lower = row[2];
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                point.hasSerif = (lower.find('h') != std::string::npos);
                point.isTerminator = (lower.find('z') != std::string::npos);
                newSegment = (lower.find('k') != std::string::npos);
            }

            segment.push_back(point);
        }
    }

    if (newSegment) {
        segments.push_back(segment);
    }
}

// ============================================================================
// CSV Export — writes vector letter data to CSV file
// ============================================================================
void VectorLetterEngine::exportToCSV(const std::string& fileName, char separator) {
    std::ofstream file(fileName);
    if (!file.is_open()) return;

    for (size_t seg = 0; seg < segments.size(); seg++) {
        for (size_t pt = 0; pt < segments[seg].size(); pt++) {
            std::string opts;
            if (!segments[seg][pt].options.empty())
                opts = std::string(1, separator) + segments[seg][pt].options;

            file << static_cast<int>(std::round(segments[seg][pt].x))
                 << separator
                 << static_cast<int>(std::round(segments[seg][pt].y))
                 << opts << "\n";
        }
    }
}

// ============================================================================
// Generate full toolpath with envelope offsets
// ============================================================================
void VectorLetterEngine::generateFullPath() {
    m_vectorSegments.clear();

    for (auto& data : segments) {
        PointCollection pointCollection;
        computeAlphaPrimary(data);
        computeAlphaMean(data);
        drawSegmentAxis(pointCollection, data);

        int shift = m_stepover + (m_diameter / 2);
        int passCount = 0;

        while (shift < (m_thickness / 2)) {
            passCount++;
            shift += m_stepover;
        }

        shift = m_stepover;

        if (passCount > 0) {
            for (int i = 0; i < passCount; i++) {
                drawSegmentEnvelope(pointCollection, data, shift);
                shift += m_stepover;
            }
            drawSegmentEnvelope(pointCollection, data, (m_thickness - m_diameter) / 2);
        }

        m_vectorSegments.push_back(pointCollection);
    }
}

// ============================================================================
// Get raw point collections
// ============================================================================
const std::vector<PointCollection>& VectorLetterEngine::getPointCollections() const {
    return m_vectorSegments;
}

// ============================================================================
// Export to screen polylines (Y-flipped for display)
// ============================================================================
std::vector<PointCollection> VectorLetterEngine::exportToScreenPolylines(
    double screenScale, double yOffset) const {

    std::vector<PointCollection> result;
    result.reserve(m_vectorSegments.size());

    for (const auto& seg : m_vectorSegments) {
        PointCollection screenPoints;
        screenPoints.reserve(seg.size());
        for (const auto& pt : seg) {
            screenPoints.push_back(Point2D(
                pt.X * screenScale,
                (m_yScreenOffset - pt.Y) * screenScale));
        }
        result.push_back(screenPoints);
    }
    return result;
}

// ============================================================================
// Transformations
// ============================================================================
void VectorLetterEngine::multiplyX(double n) {
    for (auto& seg : segments)
        for (auto& pt : seg)
            pt.x *= n;
}

void VectorLetterEngine::addX(double n) {
    for (auto& seg : segments)
        for (auto& pt : seg)
            pt.x += n;
}

void VectorLetterEngine::multiplyY(double n) {
    for (auto& seg : segments)
        for (auto& pt : seg)
            pt.y *= n;
}

void VectorLetterEngine::addY(double n) {
    for (auto& seg : segments)
        for (auto& pt : seg)
            pt.y += n;
}
