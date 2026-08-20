#include "Core/MapObjectFactory.h"

#include "Entities/Props/DoorGate.h"
#include "Entities/Props/Prop.h"

bool MapObjectFactory::IsMapObjectType(MapObjectId type) {
    return type == MapObjectId::DestructibleBox ||
        type == MapObjectId::Prop1 ||
        type == MapObjectId::Prop2 ||
        type == MapObjectId::MockWall;
}

std::unique_ptr<MapObject> MapObjectFactory::Create(
    MapObjectId type,
    Vector2 position,
    GameObjectCell cell
) {
    if (!IsMapObjectType(type)) return nullptr;
    return std::make_unique<Prop>(position, cell, type);
}
