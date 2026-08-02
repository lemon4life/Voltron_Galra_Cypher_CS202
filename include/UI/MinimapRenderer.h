#pragma once
#include "Core/Level/Tilemap.h"
#include "raylib.h"

class MinimapRenderer {
public:
    static void Draw(const LevelMap& levelMap, int currentGridX, int currentGridY);
};
