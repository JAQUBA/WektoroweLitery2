// ============================================================================
// DocumentParser.cpp — Layout file parser implementation
//
// File format (semicolon-separated lines):
//   # comment                                             — ignored line
//   l                                                     — new row (line break)
//   t;width;height;dx;dy;textH;cond;thick;text          — text-only plate
//   tw;width;height;dx;dy;textH;cond;thick;text         — plate with frame
//   w;width;height                                        — frame-only element
// ============================================================================
#include "DocumentParser.h"
#include "Nameplate.h"
#include "TableRow.h"

#include <Util/NumberUtils.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <map>
#include <stdexcept>

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static double parseDouble(const std::string& s) {
    return NumberUtils::parseDouble(s);
}

static std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> tokens;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, sep))
        tokens.push_back(token);
    return tokens;
}

static std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

static void stripUtf8BOM(std::string& s) {
    if (s.size() >= 3 &&
        static_cast<unsigned char>(s[0]) == 0xEF &&
        static_cast<unsigned char>(s[1]) == 0xBB &&
        static_cast<unsigned char>(s[2]) == 0xBF) {
        s.erase(0, 3);
    }
}

static bool isVL2Content(const std::string& content) {
    std::istringstream input(content);
    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        return line[0] == '[' || line.rfind("version=", 0) == 0;
    }
    return false;
}

// --- Shared parsing logic (works on any std::istream) ---
static Document parseStreamImpl(std::istream& input,
                                const LffFont& font,
                                double diameter,
                                double stepover,
                                std::vector<int>* errorLines = nullptr) {
    Document doc;
    doc.millingDiameter_mm = diameter;
    doc.stepover_mm = stepover;
    TableRow currentRow;
    Nameplate currentPlate;

    double xPos = 0.0;
    double yPos = 0.0;
    double maxRowHeight = 0.0;
    double frameW = 0.0, frameH = 0.0;
    double shiftX = 0.0, shiftY = 0.0;

    const char separator = ';';

    std::string line;
    bool firstLine = true;
    int lineNumber = 0;
    while (std::getline(input, line)) {
        if (firstLine) {
            stripUtf8BOM(line);
            firstLine = false;
        }
        int currentLine = lineNumber++;
        line = trim(line);
        if (line.empty()) continue;

        try {

        auto row = split(line, separator);
        if (row.size() < 2 && toLower(row[0])[0] != 'l') continue;

        std::string cmd = toLower(row[0]);

        switch (cmd[0]) {
            case 'l': {
                // New row
                doc.addRow(currentRow);
                currentRow = TableRow();
                xPos = 0;
                yPos += maxRowHeight;
                break;
            }

            case 't': {
                // Nameplate (t = text only, tw = text + frame)
                if (row.size() > 1) frameW = parseDouble(row[1]);
                if (row.size() > 2) frameH = parseDouble(row[2]);
                if (row.size() > 3) shiftX = parseDouble(row[3]);
                if (row.size() > 4) shiftY = parseDouble(row[4]);

                double textH = 0.0;
                double cond = 1.0;
                double thick = 0.0;

                if (row.size() > 5) textH = parseDouble(row[5]);
                if (row.size() > 6) cond = parseDouble(row[6]) / 100.0;
                if (row.size() > 7) thick = parseDouble(row[7]);

                std::string text;
                if (row.size() > 8) text = row[8];

                bool isFramePlate = (cmd.size() > 1 && cmd[1] == 'w');
                bool isAdvancePlate = (cmd.size() > 1 && cmd[1] == 'a');

                std::vector<std::string> subLines;
                if (text.find('|') != std::string::npos) {
                    std::vector<std::string> rawSub = split(text, '|');
                    for (const auto& s : rawSub) subLines.push_back(trim(s));
                } else {
                    subLines.push_back(text);
                }

                int numSubLines = static_cast<int>(subLines.size());

                for (int i = 0; i < numSubLines; i++) {
                    currentPlate = Nameplate();
                    currentPlate.diameter = doc.millingDiameter_mm;
                    currentPlate.stepover = doc.stepover_mm;
                    currentPlate.textHeight_mm = textH;
                    currentPlate.condensation = cond;
                    currentPlate.thickness = thick;
                    currentPlate.textOffsetX_mm = shiftX;

                    if (numSubLines > 1) {
                        double autoOff = ((numSubLines - 1) / 2.0 - i) * (textH * 1.5);
                        currentPlate.textOffsetY_mm = shiftY + autoOff;
                    } else {
                        currentPlate.textOffsetY_mm = shiftY;
                    }

                    currentPlate.hasFrame = false;
                    currentPlate.frameLeft_mm = xPos;
                    currentPlate.frameBottom_mm = yPos;
                    currentPlate.frameWidth_mm = frameW;
                    currentPlate.frameHeight_mm = frameH;

                    currentPlate.appendText(subLines[i], font);
                    currentRow.addNameplate(currentPlate);
                }

                if (isFramePlate) {
                    currentPlate = Nameplate();
                    currentPlate.hasFrame = true;
                    currentPlate.frameLeft_mm = xPos;
                    currentPlate.frameBottom_mm = yPos;
                    currentPlate.frameWidth_mm = frameW;
                    currentPlate.frameHeight_mm = frameH;
                    currentRow.addNameplate(currentPlate);
                    xPos += frameW;
                    if (frameH > maxRowHeight) maxRowHeight = frameH;
                } else if (isAdvancePlate) {
                    xPos += frameW;
                    if (frameH > maxRowHeight) maxRowHeight = frameH;
                }
                break;
            }

            case 'w': {
                // Frame-only element
                currentPlate = Nameplate();
                if (row.size() > 1) frameW = parseDouble(row[1]);
                if (row.size() > 2) frameH = parseDouble(row[2]);
                currentPlate.hasFrame = true;
                currentPlate.frameLeft_mm = xPos;
                currentPlate.frameBottom_mm = yPos;
                currentPlate.frameWidth_mm = frameW;
                currentPlate.frameHeight_mm = frameH;
                currentRow.addNameplate(currentPlate);
                xPos += frameW;
                maxRowHeight = frameH;
                break;
            }
        }

        } catch (const std::exception&) {
            if (errorLines) errorLines->push_back(currentLine);
        }
    }

    // Flush last row if it has any nameplates (fixes missing final row
    // when the file does not end with an 'L;' command).
    if (!currentRow.getNameplates().empty()) {
        doc.addRow(currentRow);
    }

    return doc;
}

struct VL2Template {
    std::map<std::string, std::string> values;
};

static std::string vl2Value(const std::map<std::string, std::string>& values,
                            const std::string& key,
                            const std::string& fallback = "") {
    auto it = values.find(key);
    return it == values.end() ? fallback : it->second;
}

static bool hasVL2Value(const std::map<std::string, std::string>& values,
                        const std::string& key) {
    return values.find(key) != values.end();
}

static std::pair<std::string, std::string> vl2Size(
    const std::map<std::string, std::string>& values) {
    std::string width = vl2Value(values, "width");
    std::string height = vl2Value(values, "height");
    std::string size = vl2Value(values, "size");
    size_t separator = size.find('x');
    if ((width.empty() || height.empty()) && separator != std::string::npos) {
        width = trim(size.substr(0, separator));
        height = trim(size.substr(separator + 1));
    }
    return { width.empty() ? "0" : width, height.empty() ? "0" : height };
}

static std::string quoteVL2Text(const std::string& text) {
    // The legacy format uses the final field as free text. Keep semicolons
    // out of generated records because they are field separators there.
    std::string result = text;
    std::replace(result.begin(), result.end(), ';', ',');
    return result;
}

// The legacy engine expects thickness as an abstract stroke-width unit
// (unit = mm * 100 / textHeight_mm); VL2 files express it as real mm.
static std::string thicknessMmToLegacyUnits(const std::string& thicknessMm,
                                            const std::string& textHeightMm) {
    double thickness = 0.0;
    double height = 0.0;
    if (!NumberUtils::tryParseDouble(thicknessMm, thickness) ||
        !NumberUtils::tryParseDouble(textHeightMm, height) ||
        height <= 0.0) {
        return thicknessMm;
    }
    double units = thickness * 100.0 / height;
    std::ostringstream out;
    out << units;
    return out.str();
}

static std::string buildVL2Record(const std::string& command,
                                  const std::map<std::string, std::string>& values,
                                  const std::string& text) {
    auto dimensions = vl2Size(values);
    std::string textHeight = vl2Value(values, "text_height");
    if (textHeight.empty()) {
        std::string txtVal = vl2Value(values, "text");
        double d = 0.0;
        if (!txtVal.empty() && NumberUtils::tryParseDouble(txtVal, d)) {
            textHeight = txtVal;
        } else {
            textHeight = "0";
        }
    }
    std::string offsetX = vl2Value(values, "offset_x");
    std::string offsetY = vl2Value(values, "offset_y");
    std::string offset = vl2Value(values, "offset");
    size_t separator = offset.find(',');
    if (separator != std::string::npos) {
        if (offsetX.empty()) offsetX = trim(offset.substr(0, separator));
        if (offsetY.empty()) offsetY = trim(offset.substr(separator + 1));
    }
    if (offsetX.empty()) offsetX = "0";
    if (offsetY.empty()) offsetY = "0";

    return command + ";" + dimensions.first + ";" + dimensions.second + ";" +
        offsetX + ";" + offsetY + ";" +
        textHeight + ";" +
        vl2Value(values, "condensation", "100") + ";" +
        thicknessMmToLegacyUnits(vl2Value(values, "thickness", "0"), textHeight) +
        ";" + quoteVL2Text(text);
}

static void addCompactOptions(std::map<std::string, std::string>& item,
                              const std::string& options) {
    size_t start = 0;
    while (start < options.size()) {
        size_t end = options.find(',', start);
        if (end == std::string::npos) end = options.size();
        std::string option = trim(options.substr(start, end - start));
        size_t equal = option.find('=');
        if (equal != std::string::npos) {
            std::string key = toLower(trim(option.substr(0, equal)));
            std::string value = trim(option.substr(equal + 1));
            if (key == "text" || key == "height") key = "text_height";
            else if (key == "cond") key = "condensation";
            else if (key == "thick") key = "thickness";
            item[key] = value;
        } else if (!option.empty() && item.find("offset") != item.end()) {
            item["offset"] += "," + option;
        }
        start = end + 1;
    }
}

static void parseLineOptions(const std::string& rawLine,
                             std::string& outText,
                             std::map<std::string, std::string>& outOptions) {
    outText = trim(rawLine);
    outOptions.clear();

    size_t openParen = outText.find('(');
    while (openParen != std::string::npos) {
        size_t closeParen = outText.find(')', openParen);
        if (closeParen == std::string::npos) break;

        std::string inner = outText.substr(openParen + 1, closeParen - openParen - 1);
        if (inner.find('=') != std::string::npos) {
            size_t start = 0;
            while (start < inner.size()) {
                size_t end = inner.find(',', start);
                if (end == std::string::npos) end = inner.size();
                std::string opt = trim(inner.substr(start, end - start));
                size_t eq = opt.find('=');
                if (eq != std::string::npos) {
                    std::string key = toLower(trim(opt.substr(0, eq)));
                    std::string val = trim(opt.substr(eq + 1));
                    if (key == "text" || key == "height") key = "text_height";
                    else if (key == "cond") key = "condensation";
                    else if (key == "thick") key = "thickness";
                    outOptions[key] = val;
                }
                start = end + 1;
            }
            outText = trim(outText.substr(0, openParen) + " " + outText.substr(closeParen + 1));
            openParen = outText.find('(');
        } else {
            openParen = outText.find('(', closeParen + 1);
        }
    }
}

static bool parseCompactItem(const std::string& line,
                             std::map<std::string, std::string>& item) {
    size_t colon = line.find(':');
    if (colon == std::string::npos) return false;

    std::string selector = trim(line.substr(0, colon));
    std::string payload = trim(line.substr(colon + 1));
    if (payload.empty()) return false;

    size_t optionsStart = selector.find('(');
    if (optionsStart != std::string::npos && selector.back() == ')') {
        std::string options = selector.substr(optionsStart + 1,
            selector.size() - optionsStart - 2);
        std::string selectorName = toLower(trim(selector.substr(0, optionsStart)));
        if (selectorName == "plate") {
            item["use"] = "";
            item["__inline"] = "true";
        } else {
            item["use"] = selectorName;
        }
        addCompactOptions(item, options);
    } else if (selector.empty()) {
        item["use"] = "";
    } else {
        item["use"] = toLower(selector);
    }

    std::vector<std::string> lines = split(payload, '|');
    std::string cleanPayload = "";

    for (size_t i = 0; i < lines.size(); i++) {
        std::string lineText;
        std::map<std::string, std::string> lineOpts;
        parseLineOptions(lines[i], lineText, lineOpts);

        std::string lineKey = "line" + std::to_string(i + 1);
        item[lineKey] = lineText;

        for (const auto& opt : lineOpts) {
            item[lineKey + "_" + opt.first] = opt.second;
        }

        if (i > 0) cleanPayload += " | ";
        cleanPayload += lineText;
    }

    item["text"] = cleanPayload;
    return true;
}

static void appendVL2Item(std::vector<std::string>& legacyLines,
                          const std::map<std::string, VL2Template>& templates,
                          const std::map<std::string, std::string>& item,
                          int lineNumber,
                          std::vector<int>* errorLines) {
    bool inlineItem = vl2Value(item, "__inline") == "true";
    std::string templateName = vl2Value(item, "use", vl2Value(item, "plate"));
    std::map<std::string, std::string> values;

    if (inlineItem) {
        auto defaultIt = templates.find("__default__");
        if (defaultIt != templates.end()) values = defaultIt->second.values;
    } else {
        auto templateIt = templates.find(toLower(templateName));
        if (templateIt == templates.end() && templateName.empty()) {
            templateIt = templates.find("__default__");
        }
        if (templateIt == templates.end()) {
            if (errorLines) errorLines->push_back(lineNumber);
            return;
        }
        values = templateIt->second.values;
    }

    for (const auto& pair : item) {
        if (pair.first != "use" && pair.first != "plate" &&
            pair.first != "__inline")
            values[pair.first] = pair.second;
    }

    std::string type = toLower(vl2Value(values, "type", "plate"));
    if (type == "empty" || type == "frame") {
        auto dimensions = vl2Size(values);
        legacyLines.push_back("w;" + dimensions.first + ";" + dimensions.second);
        return;
    }

    // Gather line texts
    std::vector<std::string> lineTexts;

    int checkIdx = 1;
    while (true) {
        std::string key = "line" + std::to_string(checkIdx);
        auto lineIt = item.find(key);
        std::string lineVal = lineIt != item.end() ? lineIt->second : vl2Value(values, key);
        if (lineIt == item.end() && !hasVL2Value(values, key)) break;
        lineTexts.push_back(lineVal);
        checkIdx++;
    }

    if (lineTexts.empty()) {
        std::string rawText = vl2Value(item, "text", vl2Value(values, "text"));
        if (rawText.find('|') != std::string::npos) {
            std::vector<std::string> splitLines = split(rawText, '|');
            for (auto& s : splitLines) {
                lineTexts.push_back(trim(s));
            }
        } else if (!rawText.empty()) {
            lineTexts.push_back(rawText);
        }
    }

    bool hasFrame = toLower(vl2Value(values, "frame", "true")) != "false";

    if (lineTexts.size() <= 1) {
        std::string text = lineTexts.empty() ? "" : lineTexts[0];
        std::string command = hasFrame ? "tw" : "ta";
        std::string prefix = "line1_";
        for (const auto& pair : values) {
            if (pair.first.rfind(prefix, 0) == 0) {
                std::string k = pair.first.substr(prefix.size());
                if (k != "text") values[k] = pair.second;
            }
        }
        legacyLines.push_back(buildVL2Record(command, values, text));
    } else {
        int numLines = static_cast<int>(lineTexts.size());

        std::string baseOffStr = vl2Value(values, "offset_y");
        std::string offsetStr = vl2Value(values, "offset");
        size_t sep = offsetStr.find(',');
        if (baseOffStr.empty() && sep != std::string::npos) {
            baseOffStr = trim(offsetStr.substr(sep + 1));
        }
        double baseOffsetY = 0.0;
        if (!baseOffStr.empty()) {
            NumberUtils::tryParseDouble(baseOffStr, baseOffsetY);
        }

        for (int i = 0; i < numLines; i++) {
            int lineIndex = i + 1;
            std::string text = lineTexts[i];

            std::map<std::string, std::string> lineValues = values;
            std::string prefix = "line" + std::to_string(lineIndex) + "_";

            for (const auto& pair : values) {
                if (pair.first.rfind(prefix, 0) == 0)
                    lineValues[pair.first.substr(prefix.size())] = pair.second;
            }

            bool hasExplicitOffset = hasVL2Value(values, prefix + "offset_y") ||
                                     hasVL2Value(item, prefix + "offset_y") ||
                                     hasVL2Value(item, "line" + std::to_string(lineIndex) + "_offset_y");

            if (!hasExplicitOffset) {
                double lineTextHeight = 6.0;
                std::string hStr = vl2Value(lineValues, "text_height");
                if (hStr.empty()) {
                    std::string tStr = vl2Value(lineValues, "text");
                    double d = 0.0;
                    if (!tStr.empty() && NumberUtils::tryParseDouble(tStr, d)) {
                        hStr = tStr;
                    }
                }
                if (!hStr.empty()) {
                    NumberUtils::tryParseDouble(hStr, lineTextHeight);
                }
                if (lineTextHeight <= 0.0) lineTextHeight = 6.0;

                double autoOffsetY = baseOffsetY + ((numLines - 1) / 2.0 - i) * (lineTextHeight * 1.5);
                std::ostringstream ss;
                ss << autoOffsetY;
                lineValues["offset_y"] = ss.str();
            }

            std::string cmd = (!hasFrame && i == numLines - 1) ? "ta" : "t";
            legacyLines.push_back(buildVL2Record(cmd, lineValues, text));
        }

        if (hasFrame) {
            auto dimensions = vl2Size(values);
            legacyLines.push_back("w;" + dimensions.first + ";" + dimensions.second);
        }
    }
}

static Document parseVL2Impl(const std::string& content,
                             const LffFont& font,
                             double diameter,
                             double stepover,
                             std::vector<int>* errorLines) {
    std::map<std::string, VL2Template> templates;
    std::vector<std::string> legacyLines;
    std::map<std::string, std::string> sectionValues;
    std::string section;
    std::string currentTemplate;
    std::map<std::string, std::string> currentItem;
    bool itemOpen = false;
    bool rowSeen = false;

    auto flushItem = [&](int lineNumber) {
        if (!itemOpen) return;
        appendVL2Item(legacyLines, templates, currentItem, lineNumber, errorLines);
        currentItem.clear();
        itemOpen = false;
    };

    std::istringstream input(content);
    std::string line;
    int lineNumber = 0;
    while (std::getline(input, line)) {
        int currentLine = lineNumber++;
        if (currentLine == 0) stripUtf8BOM(line);
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line.front() == '[' && line.back() == ']') {
            flushItem(currentLine);
            std::string header = trim(line.substr(1, line.size() - 2));
            std::istringstream headerStream(header);
            headerStream >> section;
            section = toLower(section);
            currentTemplate.clear();
            if (section == "template") {
                headerStream >> currentTemplate;
                currentTemplate = toLower(currentTemplate);
                templates[currentTemplate] = VL2Template();
            } else if (section == "default") {
                currentTemplate = "__default__";
                templates[currentTemplate] = VL2Template();
            } else if (section == "row") {
                if (rowSeen) legacyLines.push_back("l");
                rowSeen = true;
            }
            continue;
        }

        if (section == "row" && line.find(':') != std::string::npos) {
            flushItem(currentLine);
            if (parseCompactItem(line, currentItem)) {
                itemOpen = true;
                continue;
            }
        }

        size_t equal = line.find('=');
        if (equal == std::string::npos) {
            if (section == "row") {
                flushItem(currentLine);
            }
            if (section == "row" && parseCompactItem(line, currentItem)) {
                itemOpen = true;
                continue;
            }
            if (errorLines) errorLines->push_back(currentLine);
            continue;
        }
        std::string key = toLower(trim(line.substr(0, equal)));
        std::string value = trim(line.substr(equal + 1));

        if ((section == "template" || section == "default") &&
            !currentTemplate.empty()) {
            if (key == "text") key = "text_height";
            templates[currentTemplate].values[key] = value;
        } else if (section == "row") {
            if (key == "use" || key == "plate") flushItem(currentLine);
            currentItem[key] = value;
            itemOpen = true;
        } else if (section != "settings" && key != "version") {
            if (errorLines) errorLines->push_back(currentLine);
        }
    }
    flushItem(lineNumber);

    std::ostringstream legacy;
    for (const auto& generated : legacyLines) legacy << generated << "\n";
    std::istringstream legacyInput(legacy.str());
    return parseStreamImpl(legacyInput, font, diameter, stepover, nullptr);
}

// --- Public API ---

Document DocumentParser::parseFile(const std::string& fileName,
                                    const LffFont& font,
                                    double diameter,
                                    double stepover) {
    std::ifstream file(fileName);
    if (!file.is_open()) return Document();
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return parseString(content, font, diameter, stepover, nullptr);
}

Document DocumentParser::parseString(const std::string& content,
                                      const LffFont& font,
                                      double diameter,
                                      double stepover,
                                      std::vector<int>* errorLines) {
    if (isVL2Content(content))
        return parseVL2String(content, font, diameter, stepover, errorLines);

    std::istringstream stream(content);
    return parseStreamImpl(stream, font, diameter, stepover, errorLines);
}

Document DocumentParser::parseVL2String(const std::string& content,
                                         const LffFont& font,
                                         double diameter,
                                         double stepover,
                                         std::vector<int>* errorLines) {
    return parseVL2Impl(content, font, diameter, stepover, errorLines);
}
