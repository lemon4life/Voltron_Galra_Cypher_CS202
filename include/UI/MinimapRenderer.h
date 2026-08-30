#pragma once
#include "Core/Level/Tilemap.h"
#include "raylib.h"

class MinimapRenderer {
public:
    /// Renders this component using its current state and visual resources.
    static void Draw(
        const LevelMap& levelMap,
        int currentGridX,
        int currentGridY,
        Rectangle bounds,
        int currentFloor
    );
};
