#pragma once
#include "Core/LevelAccess.h"
#include "Entities/GameObject.h"

class TeamManager;

class EntityFactory {
public:
    static GameObject* CreateEntity(
        char type,
        Vector2 position,
        TeamManager* teamManager,
        const LevelAccessBundle& levelAccess
    );
};
