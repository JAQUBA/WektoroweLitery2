// ============================================================================
// VectorLetterEngine.h — Vector letter engine with tool envelope generation
// ============================================================================
#ifndef VECTOR_LETTER_ENGINE_H
#define VECTOR_LETTER_ENGINE_H

#include "VectorPoint.h"
#include "LffFont.h"
#include <vector>
#include <string>

// --- 2D Point (equivalent of System.Windows.Point) ---
struct Point2D {
    double X = 0.0;
    double Y = 0.0;
    Point2D() = default;
    Point2D(double x, double y) : X(x), Y(y) {}
};

// --- Point collection (equivalent of PointCollection) ---
using PointCollection = std::vector<Point2D>;

class VectorLetterEngine {
public:
    double maxX = 0.0;
    std::vector<std::vector<VectorPoint>> segments;

    VectorLetterEngine();
    VectorLetterEngine(const std::string& fileName, char separator = ';',
                       double scale = 1.0, double yScreenOffset = 3500.0,
                       double height = 5.0, double thickness = 10.0,
                       double diameter = 0.3, double stepover = 0.25);

    // LFF font glyph constructor
    VectorLetterEngine(const LffGlyph& glyph,
                       double scale = 1.0, double yScreenOffset = 3500.0,
                       double height = 5.0, double thickness = 10.0,
                       double diameter = 0.3, double stepover = 0.25);

    void importFromCSV(const std::string& fileName, char separator = ';');
    void importFromLff(const LffGlyph& glyph);
    void exportToCSV(const std::string& fileName, char separator = ';');
    void generateFullPath();

    // Transformations
    void multiplyX(double n);
    void addX(double n);
    void multiplyY(double n);
    void addY(double n);

    // Access to results
    const std::vector<PointCollection>& getPointCollections() const;

    // Export to screen polylines (with Y flip)
    std::vector<PointCollection> exportToScreenPolylines(
        double screenScale = 1.0, double yOffset = 0.0) const;

    double getScale() const { return m_scale; }

private:
    double m_xb = 0.0;
    double m_yb = 0.0;
    double m_scale = 1.0;
    int m_thickness = 0;
    int m_diameter = 0;
    int m_stepover = 0;
    double m_yScreenOffset = 500.0;

    std::vector<PointCollection> m_vectorSegments;

    void drawSegmentAxis(PointCollection& points, const std::vector<VectorPoint>& data);
    void computeQuadrant(int i, double dx, double dy, std::vector<VectorPoint>& data);
    void computeAlphaPrimary(std::vector<VectorPoint>& data);
    void computeAlphaMean(std::vector<VectorPoint>& data);
    void drawSegmentEnvelope(PointCollection& points, std::vector<VectorPoint>& data, double pw);
};

#endif // VECTOR_LETTER_ENGINE_H
