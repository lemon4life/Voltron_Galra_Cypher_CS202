#pragma once

#include "raylib.h"

class Enemy;
class GameObject;

class ILevelLineOfSightQuery {
public:
    virtual ~ILevelLineOfSightQuery() = default;

    virtual bool HasClearLineOfSight(
        Vector2 start,
        Vector2 end,
        float projectileRadius = 5.0f
    ) const = 0;
};

class IEntityRemovalAccess {
public:
    virtual ~IEntityRemovalAccess() = default;

    virtual void QueueRemoval(GameObject* entity) = 0;
};

class IEnemyPathAccess {
public:
    virtual ~IEnemyPathAccess() = default;

    virtual void BeginPathFinding(Enemy* enemy) = 0;
    virtual void EndPathFinding(Enemy* enemy) = 0;
};

struct LevelAccessBundle {
    IEntityRemovalAccess* removal = nullptr;
    IEnemyPathAccess* pathFinding = nullptr;
    ILevelLineOfSightQuery* lineOfSight = nullptr;
};
