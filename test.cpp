#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

int main() {
    std::string filepath = "assets/map/level1_Tile Layer 1.csv";
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open level file: " << filepath << std::endl;
        return 1;
    }

    std::string line;
    std::vector<std::vector<int>> mapGrid;
    int gridCols = 0;
    
    bool isCSV = (filepath.length() >= 4 && filepath.substr(filepath.length() - 4) == ".csv");
    std::cout << "isCSV: " << isCSV << std::endl;

    while (std::getline(file, line)) {
        if (line.length() > 0 && line.back() == '\r') {
            line.pop_back(); // Handle Windows line endings
        }
        
        std::vector<int> row;
        if (isCSV) {
            std::stringstream ss(line);
            std::string cellString;
            while (std::getline(ss, cellString, ',')) {
                try {
                    row.push_back(std::stoi(cellString));
                } catch (const std::invalid_argument& e) {
                    row.push_back(0);
                }
            }
        }
        
        if (row.size() > gridCols) gridCols = row.size();
        mapGrid.push_back(row);
    }
    file.close();
    
    std::cout << "gridRows: " << mapGrid.size() << std::endl;
    std::cout << "gridCols: " << gridCols << std::endl;
    
    // Print first 5 rows and 5 cols
    for (int r = 0; r < std::min(5, (int)mapGrid.size()); ++r) {
        for (int c = 0; c < std::min(5, (int)mapGrid[r].size()); ++c) {
            std::cout << mapGrid[r][c] << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
