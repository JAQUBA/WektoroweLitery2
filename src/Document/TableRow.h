// ============================================================================
// TableRow.h — Row of nameplate tables within a document
// ============================================================================
#ifndef TABLE_ROW_H
#define TABLE_ROW_H

#include "Nameplate.h"
#include <vector>

class TableRow {
public:
    TableRow() = default;

    void addNameplate(const Nameplate& plate) { m_plates.push_back(plate); }

    std::vector<Nameplate>& getNameplates() { return m_plates; }
    const std::vector<Nameplate>& getNameplates() const { return m_plates; }

private:
    std::vector<Nameplate> m_plates;
};

#endif // TABLE_ROW_H
