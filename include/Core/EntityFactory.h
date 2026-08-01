#pragma once
#include "Core/LevelAccess.h"
#include "Entities/GameObject.h"

class TeamManager;

class EntityFactory {
public:
    static GameObject* CreateEntity(
        MapObjectId type,
        Vector2 position,
        GameObjectCell cell,
        TeamManager* teamManager,
        const LevelAccessBundle& levelAccess
    );
};
