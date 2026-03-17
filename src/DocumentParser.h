// ============================================================================
// DocumentParser.h — Parses .TXT layout files into Document model
// ============================================================================
#ifndef DOCUMENT_PARSER_H
#define DOCUMENT_PARSER_H

#include "Document.h"
#include <string>

class DocumentParser {
public:
    // Parse a layout file (semicolon-separated) into a Document
    static Document parseFile(const std::string& fileName,
                              const std::string& csvDir);
};

#endif // DOCUMENT_PARSER_H
