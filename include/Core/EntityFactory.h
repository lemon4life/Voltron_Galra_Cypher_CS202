#pragma once
#include "Core/LevelAccess.h"
#include "Entities/GameObject.h"
#include <memory>

class TeamManager;

// Design Pattern - Simple Factory:
// Client/creator: ObjectManager. Product base: GameObject. Concrete products
// include enemies, NPCs, pickups, machines, and Hub stands. The MapObjectId
// switch centralizes construction and injects each product's required services.
class EntityFactory {
public:
    /// Creates entity.
    static std::unique_ptr<GameObject> CreateEntity(
        MapObjectId type,
        Vector2 position,
        TeamManager* teamManager,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess,
        ILevelLineOfSightQuery& lineOfSight
    );
};
