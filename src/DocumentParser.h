// ============================================================================
// DocumentParser.h — Parses .TXT layout files into Document model
// ============================================================================
#ifndef DOCUMENT_PARSER_H
#define DOCUMENT_PARSER_H

#include "Document.h"
#include <string>

class DocumentParser {
public:
    // Parse a layout file (semicolon-separated) into a Document.
    // diameter/stepover override any values from the f-line in the file.
    static Document parseFile(const std::string& fileName,
                              const std::string& csvDir,
                              double diameter = 0.0,
                              double stepover = 0.0);
};

#endif // DOCUMENT_PARSER_H
