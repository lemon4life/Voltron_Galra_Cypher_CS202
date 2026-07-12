# Implementation Plan: Tiled CSV Map Parser

## Goal Description
The objective is to replace the hardcoded character-based tilemap system with a scalable CSV parsing system capable of loading multi-layered maps exported from the Tiled map editor (e.g., `level1_Tile Layer 1.csv`). This allows for dynamic 20x20 visual map rendering using the 32x32 Galra ship tileset.

## Proposed Changes

### `include/Core/Manager/LevelManager.h`
- [MODIFY] Change the underlying map data structure from a 2D array of `char` to `std::vector<std::vector<int>>`.

### `src/Core/Manager/LevelManager.cpp`
- [MODIFY] Overhaul `LoadLevel(const std::string& filepath)` to use `<fstream>` and `<sstream>`.
- [MODIFY] Implement `std::getline` logic to read integers separated by commas and construct the 2D grid.
- [MODIFY] Update the rendering loop to map specific integer IDs (e.g., ID `0` to Void space).

## Verification Plan
1. **Automated Tests:** Verify that the CSV vectors instantiate without crashing.
2. **Manual Verification:** Load `level1_Tile Layer 1.csv` and ensure the collision bounds correctly align with solid tile IDs, while integer ID 0 remains passable.
