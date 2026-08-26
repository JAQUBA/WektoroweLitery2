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

    Document() = default;

    void addRow(const TableRow& row) { m_rows.push_back(row); }

    const std::vector<TableRow>& getRows() const { return m_rows; }

private:
    std::vector<TableRow> m_rows;
};

#endif // DOCUMENT_H
