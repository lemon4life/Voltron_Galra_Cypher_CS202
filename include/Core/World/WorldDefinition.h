#pragma once

#include "Core/LevelAccess.h"
#include "raylib.h"

#include <vector>

struct DynamicSpawnRequest {
    MapObjectId type = MapObjectId::Empty;
    Vector2 position = { 0.0f, 0.0f };
    GameObjectCell cell = { -1, -1 };
};

using DynamicSpawnList = std::vector<DynamicSpawnRequest>;
