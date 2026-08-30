#pragma once
#include <vector>
#include <string>
#include "Core/LevelAccess.h"

class MapLoader {
public:
    /// Parses csv.
    static bool ParseCSV(const std::string& filepath, std::vector<std::vector<int>>& output);
    /// Parses object grid.
    static bool ParseObjectGrid(const std::string& filepath, std::vector<std::vector<MapObjectId>>& output, const std::vector<std::vector<int>>& referenceLayer);
};
