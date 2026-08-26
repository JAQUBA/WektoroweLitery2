// ============================================================================
// DocumentParser.h — Parses .TXT layout files into Document model
// ============================================================================
#ifndef DOCUMENT_PARSER_H
#define DOCUMENT_PARSER_H

#include "Document.h"
#include "../Font/LffFont.h"
#include <string>
#include <vector>

class DocumentParser {
public:
    // Parse a layout file (VL2 or legacy semicolon-separated format).
    static Document parseFile(const std::string& fileName,
                              const LffFont& font,
                              double diameter = 0.0,
                              double stepover = 0.0);

    // Parse layout content from a string (VL2 or legacy format).
    // If errorLines is non-null, 0-based line numbers with parse errors are appended.
    static Document parseString(const std::string& content,
                                const LffFont& font,
                                double diameter = 0.0,
                                double stepover = 0.0,
                                std::vector<int>* errorLines = nullptr);

    // Parse the explicit VL2 block format.
    static Document parseVL2String(const std::string& content,
                                   const LffFont& font,
                                   double diameter = 0.0,
                                   double stepover = 0.0,
                                   std::vector<int>* errorLines = nullptr);
};

#endif // DOCUMENT_PARSER_H
