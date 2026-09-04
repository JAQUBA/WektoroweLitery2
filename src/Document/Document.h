// ============================================================================
// Document.h — Document model (collection of table rows)
// ============================================================================
#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "TableRow.h"
#include <vector>

class Document {
public:
    double millingDiameter_mm = 0.0;
    double stepover_mm = 0.0;
    double materialThickness_mm = 0.0;
    double textDepth_mm = 0.0;
    double safeHeight_mm = 5.0;
    double feedXY_mm = 300.0;
    double feedZ_mm = 100.0;
    double spindleRPM = 0.0;
    bool laserMode = false;
    bool repeatFrameCut = false;

    Document() = default;

    void addRow(const TableRow& row) { m_rows.push_back(row); }

    void rotate90() {
        double minX = 0.0, minY = 0.0, maxX = 0.0, maxY = 0.0;
        if (!getBoundingBox(minX, minY, maxX, maxY)) return;
        for (auto& row : m_rows)
            for (auto& plate : row.getNameplates())
                plate.rotate90(maxX);
    }

    const std::vector<TableRow>& getRows() const { return m_rows; }

    bool getBoundingBox(double& minX, double& minY, double& maxX, double& maxY) const {
        double gMinX = 1e9, gMinY = 1e9, gMaxX = -1e9, gMaxY = -1e9;
        bool foundAny = false;

        for (const auto& row : m_rows) {
            for (const auto& plate : row.getNameplates()) {
                double pMinX, pMinY, pMaxX, pMaxY;
                if (plate.getBoundingBox(pMinX, pMinY, pMaxX, pMaxY)) {
                    if (pMinX < gMinX) gMinX = pMinX;
                    if (pMinY < gMinY) gMinY = pMinY;
                    if (pMaxX > gMaxX) gMaxX = pMaxX;
                    if (pMaxY > gMaxY) gMaxY = pMaxY;
                    foundAny = true;
                }
            }
        }

        if (foundAny) {
            minX = gMinX;
            minY = gMinY;
            maxX = gMaxX;
            maxY = gMaxY;
            return true;
        }
        return false;
    }

private:
    std::vector<TableRow> m_rows;
};

#endif // DOCUMENT_H
