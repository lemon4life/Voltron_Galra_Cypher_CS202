#include "Core/Level/LevelIO.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace {
    const fs::path ROOM_DIRECTORY = "assets/level";

    std::string GetSizePrefix(RoomSize size) {
        switch (size) {
            case RoomSize::SMALL: return "Small";
            case RoomSize::MEDIUM: return "Medium";
            case RoomSize::LARGE: return "Large";
        }
        return "Small";
    }

    int GetRoomTileSize(RoomSize size) {
        switch (size) {
            case RoomSize::SMALL: return 15;
            case RoomSize::MEDIUM: return 20;
            case RoomSize::LARGE: return 25;
        }
        return 15;
    }

    bool TryGetRoomSize(const fs::path& path, RoomSize& size) {
        std::string filename = path.filename().string();
        if (filename.rfind("Small_", 0) == 0) {
            size = RoomSize::SMALL;
            return true;
        }
        if (filename.rfind("Medium_", 0) == 0) {
            size = RoomSize::MEDIUM;
            return true;
        }
        if (filename.rfind("Large_", 0) == 0) {
            size = RoomSize::LARGE;
            return true;
        }
        return false;
    }

    bool IsManagedRoomPath(const fs::path& path, bool mustExist) {
        if (path.extension() != ".csv") return false;

        std::error_code error;
        fs::path root = fs::weakly_canonical(ROOM_DIRECTORY, error);
        if (error) return false;
        fs::path candidate = fs::weakly_canonical(path, error);
        if (error) return false;
        if (candidate.parent_path() != root) return false;
        if (mustExist && !fs::is_regular_file(candidate, error)) return false;

        RoomSize ignoredSize = RoomSize::SMALL;
        return TryGetRoomSize(candidate, ignoredSize);
    }
}

std::string LevelIO::SaveRoomToCSV(
    RoomSize size,
    const std::vector<PlaceableObject>& objects,
    const std::string& existingPath
) {
    std::error_code error;
    fs::create_directories(ROOM_DIRECTORY, error);
    if (error) return "";

    std::string sizePrefix = GetSizePrefix(size);
    fs::path outputPath;
    if (!existingPath.empty() && IsManagedRoomPath(existingPath, true)) {
        RoomSize existingSize = RoomSize::SMALL;
        if (TryGetRoomSize(existingPath, existingSize) &&
            existingSize == size) {
            outputPath = existingPath;
        }
    }

    if (outputPath.empty()) {
        int index = 1;
        do {
            std::ostringstream name;
            name << sizePrefix << "_"
                 << std::setw(2) << std::setfill('0') << index
                 << ".csv";
            outputPath = ROOM_DIRECTORY / name.str();
            index++;
        } while (fs::exists(outputPath));
    }

    std::ofstream output(outputPath, std::ios::trunc);
    if (!output.is_open()) {
        std::cerr << "Failed to save room: " << outputPath << std::endl;
        return "";
    }

    output << "ObjectID,GridX,GridY\n";
    for (const PlaceableObject& object : objects) {
        output << object.objectID << ','
               << object.gridX << ','
               << object.gridY << '\n';
    }
    return outputPath.generic_string();
}

std::vector<SavedRoomInfo> LevelIO::ListSavedRooms() {
    std::vector<SavedRoomInfo> rooms;
    std::error_code error;
    if (!fs::is_directory(ROOM_DIRECTORY, error)) return rooms;

    for (const fs::directory_entry& entry :
         fs::directory_iterator(ROOM_DIRECTORY, error)) {
        if (error) break;
        if (!entry.is_regular_file() || entry.path().extension() != ".csv") {
            continue;
        }
        RoomSize size = RoomSize::SMALL;
        if (!TryGetRoomSize(entry.path(), size)) continue;
        rooms.push_back({
            entry.path().stem().string(),
            entry.path().generic_string(),
            size
        });
    }

    std::sort(
        rooms.begin(),
        rooms.end(),
        [](const SavedRoomInfo& first, const SavedRoomInfo& second) {
            return first.name < second.name;
        }
    );
    return rooms;
}

bool LevelIO::LoadRoomFromCSV(
    const std::string& path,
    RoomSize& size,
    std::vector<PlaceableObject>& objects
) {
    if (!IsManagedRoomPath(path, true) || !TryGetRoomSize(path, size)) {
        return false;
    }

    std::ifstream input(path);
    if (!input.is_open()) return false;

    std::vector<PlaceableObject> loadedObjects;
    std::string line;
    std::getline(input, line);
    int roomTileSize = GetRoomTileSize(size);
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::stringstream row(line);
        std::string token;
        PlaceableObject object = {};
        try {
            if (!std::getline(row, token, ',')) return false;
            object.objectID = std::stoi(token);
            if (!std::getline(row, token, ',')) return false;
            object.gridX = std::stoi(token);
            if (!std::getline(row, token, ',')) return false;
            object.gridY = std::stoi(token);
        } catch (...) {
            return false;
        }

        if (object.gridX < 0 || object.gridX >= roomTileSize ||
            object.gridY < 0 || object.gridY >= roomTileSize) {
            return false;
        }
        loadedObjects.push_back(object);
    }

    objects = std::move(loadedObjects);
    return true;
}

bool LevelIO::DeleteSavedRoom(const std::string& path) {
    if (!IsManagedRoomPath(path, true)) return false;
    std::error_code error;
    bool removed = fs::remove(fs::weakly_canonical(path, error), error);
    return removed && !error;
}
