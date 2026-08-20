#pragma once
#include "Core/LevelAccess.h"
#include "Entities/GameObject.h"
#include <memory>

class TeamManager;

class EntityFactory {
public:
    static std::unique_ptr<GameObject> CreateEntity(
        MapObjectId type,
        Vector2 position,
        TeamManager* teamManager,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess,
        ILevelLineOfSightQuery& lineOfSight
    );
};
