#pragma once

#include <vector>
#include <string>

enum class RoomSize { SMALL, MEDIUM, LARGE };

struct PlaceableObject {
    int objectID;
    int gridX;
    int gridY;
};

struct SavedRoomInfo {
    std::string name;
    std::string path;
    RoomSize size;
};

class LevelIO {
public:
    static std::string SaveRoomToCSV(
        RoomSize size,
        const std::vector<PlaceableObject>& objects,
        const std::string& existingPath = ""
    );
    static std::vector<SavedRoomInfo> ListSavedRooms();
    static bool LoadRoomFromCSV(
        const std::string& path,
        RoomSize& size,
        std::vector<PlaceableObject>& objects
    );
    static bool DeleteSavedRoom(const std::string& path);
};
