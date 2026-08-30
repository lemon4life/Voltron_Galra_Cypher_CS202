#pragma once

#include "raylib.h"

#include <optional>
#include <vector>

class GameObject;
class Enemy;

enum class EnemyPathStatus {
    Pending,
    Ready,
    AtGoal,
    Unreachable,
    SearchLimitReached
};
// Data-driven type key shared by CSV maps, factories, spawning, and editor saves.
// Values describe which product to create without serializing C++ class details.
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
    MockWall = 12,
    HubPidgeStand = 13,
    PotEX = 14,
    PotHP = 15,
    PotQuint = 16,
    Drone = 17,
    ShiroNPC = 18,
    DemonTHA = 19,
    Chest = 20,
    EnhanceMachine = 21
};

struct GameObjectCell {
    int row;
    int column;
};

// Design Pattern - Dependency Injection / Interface Segregation:
// Enemy clients depend on these narrow service contracts instead of concrete
// managers. EntityFactory injects LevelManager, ObjectManager, and
// PathFindingManager through only the interfaces each enemy needs.
class ILevelLineOfSightQuery {
public:
    /// Releases resources owned by this ILevelLineOfSightQuery instance.
    virtual ~ILevelLineOfSightQuery() = default;

    /// Reports whether this component has clear line of sight.
    virtual bool HasClearLineOfSight(
        Vector2 start,
        Vector2 end,
        float projectileRadius = 5.0f
    ) const = 0;
};

class IEntityRemovalAccess {
public:
    /// Releases resources owned by this IEntityRemovalAccess instance.
    virtual ~IEntityRemovalAccess() = default;

    /// Queues removal.
    virtual void QueueRemoval(GameObject* entity) = 0;
};

class IEnemyPathAccess {
public:
    /// Releases resources owned by this IEnemyPathAccess instance.
    virtual ~IEnemyPathAccess() = default;

    /// Begins path finding.
    virtual void BeginPathFinding(Enemy& enemy) = 0;
    /// Begins path finding to.
    virtual void BeginPathFindingTo(
        Enemy& enemy,
        Vector2 worldGoal
    ) = 0;
    /// Finishes path finding.
    virtual void EndPathFinding(Enemy& enemy) = 0;

    /// Reports whether the blocked condition is satisfied.
    virtual bool IsBlocked(Rectangle bounds) const = 0;
    /// Returns the current level bounds.
    virtual Rectangle GetLevelBounds() const = 0;

    /// Returns the current next move target.
    virtual std::optional<Vector2> GetNextMoveTarget(Enemy& enemy) = 0;

    /// Returns the current navigable tile centers within.
    virtual std::vector<Vector2> GetNavigableTileCentersWithin(
        const Enemy& enemy,
        Vector2 origin,
        float radius
    ) const = 0;

    /// Returns the current local direction.
    virtual Vector2 GetLocalDirection(
        Enemy& enemy,
        Vector2 desiredDirection
    ) = 0;
};
