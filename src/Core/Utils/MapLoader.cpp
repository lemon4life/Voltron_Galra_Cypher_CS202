#include "Core/Utils/MapLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <limits>

namespace {
/// Parses integer cell.
bool ParseIntegerCell(const std::string& text, int& value) {
    if (text.empty()) return false;

    std::size_t parsedCharacters = 0;
    try {
        long long parsed = std::stoll(text, &parsedCharacters, 10);
        if (parsedCharacters != text.size() ||
            parsed < std::numeric_limits<int>::min() ||
            parsed > std::numeric_limits<int>::max()) {
            return false;
        }
        value = static_cast<int>(parsed);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

/// Reports whether the known object id condition is satisfied.
bool IsKnownObjectId(int value) {
    return value == static_cast<int>(MapObjectId::Empty) ||
        (value >= static_cast<int>(MapObjectId::DestructibleBox) &&
         value <= static_cast<int>(MapObjectId::EnhanceMachine));
}
}

/// Parses csv.
bool MapLoader::ParseCSV(const std::string& filepath, std::vector<std::vector<int>>& output) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open level layer: " << filepath << std::endl;
        output.clear();
        return false;
    }

    output.clear();
    std::string line;
    std::size_t expectedColumns = 0;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::stringstream rowStream(line);
        std::string cellString;
        std::vector<int> row;
        while (std::getline(rowStream, cellString, ',')) {
            int value = 0;
            if (!ParseIntegerCell(cellString, value)) {
                std::cerr << "Invalid CSV value in " << filepath << std::endl;
                output.clear();
                return false;
            }
            row.push_back(value);
        }

        if (row.empty()) {
            std::cerr << "Empty CSV row in " << filepath << std::endl;
            output.clear();
            return false;
        }
        if (expectedColumns == 0) {
            expectedColumns = row.size();
        } else if (row.size() != expectedColumns) {
            std::cerr << "Non-rectangular CSV layer: " << filepath << std::endl;
            output.clear();
            return false;
        }
        output.push_back(std::move(row));
    }

    return !output.empty();
}

/// Parses object grid.
bool MapLoader::ParseObjectGrid(const std::string& filepath, std::vector<std::vector<MapObjectId>>& output, const std::vector<std::vector<int>>& referenceLayer) {
    output.clear();
    output.reserve(referenceLayer.size());
    for (const auto& row : referenceLayer) {
        output.push_back(std::vector<MapObjectId>(row.size(), MapObjectId::Empty));
    }

    std::string objectLayerPath = filepath;
    size_t layerNamePosition = objectLayerPath.find("Layer 1");
    if (layerNamePosition == std::string::npos) {
        return true;
    }

    objectLayerPath.replace(layerNamePosition, std::string("Layer 1").size(), "Game Objects");

    std::ifstream objectFile(objectLayerPath);
    if (!objectFile.is_open()) {
        return true;
    }

    bool dimensionsMatch = true;
    std::string line;
    int rowIndex = 0;
    while (std::getline(objectFile, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (rowIndex >= (int)output.size()) {
            dimensionsMatch = false;
            rowIndex++;
            continue;
        }

        std::stringstream rowStream(line);
        std::string cellString;
        int columnIndex = 0;
        while (std::getline(rowStream, cellString, ',')) {
            if (columnIndex >= (int)output[rowIndex].size()) {
                dimensionsMatch = false;
                columnIndex++;
                continue;
            }

            int objectValue = 0;
            if (!ParseIntegerCell(cellString, objectValue) ||
                !IsKnownObjectId(objectValue)) {
                std::cerr << "Invalid map object id in "
                          << objectLayerPath << " at row "
                          << rowIndex + 1 << ", column "
                          << columnIndex + 1 << std::endl;
                dimensionsMatch = false;
            } else {
                output[rowIndex][columnIndex] =
                    static_cast<MapObjectId>(objectValue);
            }
            columnIndex++;
        }

        if (columnIndex != (int)output[rowIndex].size()) {
            dimensionsMatch = false;
        }
        rowIndex++;
    }

    if (rowIndex != (int)output.size()) {
        dimensionsMatch = false;
    }

    if (!dimensionsMatch) {
        std::cerr << "Game Objects layer dimensions do not match Layer 1: " << objectLayerPath << std::endl;
    }

    return dimensionsMatch;
}
