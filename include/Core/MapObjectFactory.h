#pragma once

#include "Core/World/MapObject.h"

#include <memory>

// Design Pattern - Simple Factory:
// Client: LevelManager. Product base: MapObject. Concrete products are solid
// stationary Props selected from MapObjectId and initialized with their map cell.
class MapObjectFactory {
public:
    /// Reports whether the map object type condition is satisfied.
    static bool IsMapObjectType(MapObjectId type);

    /// Creates the requested runtime object from the supplied type and configuration.
    static std::unique_ptr<MapObject> Create(
        MapObjectId type,
        Vector2 position,
        GameObjectCell cell
    );
};
