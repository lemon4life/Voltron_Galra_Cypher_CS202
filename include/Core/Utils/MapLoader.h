#pragma once
#include <vector>
#include <string>
#include "Core/LevelAccess.h"

class MapLoader {
public:
    static bool ParseCSV(const std::string& filepath, std::vector<std::vector<int>>& output);
    static bool ParseObjectGrid(const std::string& filepath, std::vector<std::vector<MapObjectId>>& output, const std::vector<std::vector<int>>& referenceLayer);
};
