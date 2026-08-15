#include "Core/Level/LevelIO.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

std::string LevelIO::SaveRoomToCSV(RoomSize size, const std::vector<PlaceableObject>& objects) {
    std::string sizePrefix;
    switch (size) {
        case RoomSize::SMALL: sizePrefix = "Small"; break;
        case RoomSize::MEDIUM: sizePrefix = "Medium"; break;
        case RoomSize::LARGE: sizePrefix = "Large"; break;
    }

    std::string directory = "assets/level";
    if (!fs::exists(directory)) {
        fs::create_directories(directory);
    }

    int index = 1;
    std::string filename;
    while (true) {
        std::ostringstream ss;
        ss << directory << "/" << sizePrefix << "_" << std::setw(2) << std::setfill('0') << index << ".csv";
        filename = ss.str();
        if (!fs::exists(filename)) {
            break;
        }
        index++;
    }

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return "";
    }

    // Write header
    outFile << "ObjectID,GridX,GridY\n";

    // Write objects
    for (const auto& obj : objects) {
        outFile << obj.objectID << "," << obj.gridX << "," << obj.gridY << "\n";
    }

    outFile.close();
    std::cout << "Successfully saved room to " << filename << std::endl;
    return filename;
}
