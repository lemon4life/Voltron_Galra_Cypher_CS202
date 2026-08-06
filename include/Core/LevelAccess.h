#pragma once

#include "raylib.h"

#include <optional>

class GameObject;
class Enemy;

enum class EnemyPathStatus {
    Pending,
    Ready,
    AtGoal,
    Unreachable,
    SearchLimitReached
};

enum class MapObjectId : int {
    Empty = -1,
    DestructibleBox = 1,
    Chaser = 2,
    Range = 3,
    Diver = 4,
    Boss = 5,
    NPC = 6,
    HubLanceStand = 7,
    HubKeithStand = 8,
    HubHunkStand = 9,
    Prop1 = 10,
    Prop2 = 11,
    MockWall = 12
};

struct GameObjectCell {
    int row;
    int column;
};

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

class IMapObjectDestroyAccess {
public:
    virtual ~IMapObjectDestroyAccess() = default;

    virtual void QueueMapObjectDestruction(
        GameObject& object,
        GameObjectCell cell
    ) = 0;
};

class IEnemyPathAccess {
public:
    virtual ~IEnemyPathAccess() = default;

    virtual void BeginPathFinding(Enemy& enemy) = 0;
    virtual void EndPathFinding(Enemy& enemy) = 0;

    virtual bool IsBlocked(Rectangle bounds) const = 0;
    virtual Rectangle GetLevelBounds() const = 0;

    virtual std::optional<Vector2> GetNextMoveTarget(Enemy& enemy) = 0;

    virtual Vector2 GetLocalDirection(
        Enemy& enemy,
        Vector2 desiredDirection
    ) = 0;
};

struct LevelAccessBundle {
    IEntityRemovalAccess& removal;
    IEnemyPathAccess& pathFinding;
    ILevelLineOfSightQuery& lineOfSight;
    IMapObjectDestroyAccess& mapObjectDestruction;
};
