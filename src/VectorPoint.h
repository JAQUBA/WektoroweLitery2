// ============================================================================
// VectorPoint.h — Vector point with angles and options for envelope generation
// ============================================================================
#ifndef VECTOR_POINT_H
#define VECTOR_POINT_H

#include <string>

struct VectorPoint {
    double x = 0.0;
    double y = 0.0;
    double alphaPrimary = 0.0;    // segment angle (alpha primary)
    double widthFactor = 0.0;     // width correction factor
    double alphaMean = 0.0;       // averaged angle (alpha mean)
    bool hasSerif = false;        // serif endpoint (stopka)
    bool isTerminator = false;    // segment terminator
    std::string options;
};

#endif // VECTOR_POINT_H
