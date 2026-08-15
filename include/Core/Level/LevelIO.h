#pragma once

#include <vector>
#include <string>

enum class RoomSize { SMALL, MEDIUM, LARGE };

struct PlaceableObject {
    int objectID;
    int gridX;
    int gridY;
};

class LevelIO {
public:
    static std::string SaveRoomToCSV(RoomSize size, const std::vector<PlaceableObject>& objects);
};
