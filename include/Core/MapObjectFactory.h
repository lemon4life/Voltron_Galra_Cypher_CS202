#pragma once

#include "Core/World/MapObject.h"

#include <memory>

class MapObjectFactory {
public:
    static bool IsMapObjectType(MapObjectId type);

    static std::unique_ptr<MapObject> Create(
        MapObjectId type,
        Vector2 position,
        GameObjectCell cell
    );
};
