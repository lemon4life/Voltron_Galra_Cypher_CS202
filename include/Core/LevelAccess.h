#pragma once

#include "raylib.h"

class GameObject;
class Enemy;

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

    virtual void BeginPathFinding(Enemy& enemy) = 0;
    virtual void EndPathFinding(Enemy& enemy) = 0;

    virtual bool IsBlocked(Rectangle bounds) const = 0;
    virtual Rectangle GetLevelBounds() const = 0;

    virtual Vector2 GetNextMoveTarget(
        Enemy& enemy,
        Vector2 fallbackTarget
    ) = 0;

    virtual Vector2 GetLocalDirection(
        Enemy& enemy,
        Vector2 desiredDirection
    ) = 0;
};

struct LevelAccessBundle {
    IEntityRemovalAccess& removal;
    IEnemyPathAccess& pathFinding;
    ILevelLineOfSightQuery& lineOfSight;
};
