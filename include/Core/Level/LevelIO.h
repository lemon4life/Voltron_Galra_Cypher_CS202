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

// Design Pattern - Data-Driven Room Templates:
// RoomEditorState is the authoring client and LevelMap is the runtime consumer.
// LevelIO converts between CSV files and PlaceableObject records, allowing room
// layout/content to change without compiling a new room subclass.
class LevelIO {
public:
    /// Writes the current room template to its managed CSV path and returns the saved path.
    static std::string SaveRoomToCSV(
        RoomSize size,
        const std::vector<PlaceableObject>& objects,
        const std::string& existingPath = ""
    );
    /// Lists valid room-template files from the editor's managed save directory.
    static std::vector<SavedRoomInfo> ListSavedRooms();
    /// Validates and loads a saved room template without accepting out-of-bounds placements.
    static bool LoadRoomFromCSV(
        const std::string& path,
        RoomSize& size,
        std::vector<PlaceableObject>& objects
    );
    /// Deletes saved room.
    static bool DeleteSavedRoom(const std::string& path);
};
