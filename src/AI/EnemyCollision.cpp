#include "AI/EnemyCollision.h"

#include "Core/LevelAccess.h"
#include "Core/Utils/CollisionMovement.h"
#include "Entities/Enemy.h"
#include "Entities/Player/Paladin.h"

#include <optional>

namespace {
    constexpr int EMBEDDED_RECOVERY_RADIUS = 32;

    /// Implements the clamp footprint to level behavior for this component.
    Vector2 ClampFootprintToLevel(
        const Enemy& enemy,
        Vector2 position,
        Rectangle levelBounds
    ) {
        Rectangle footprint = enemy.GetNavigationFootprintAt(position);
        if (footprint.x < levelBounds.x) {
            position.x += levelBounds.x - footprint.x;
        }
        if (footprint.y < levelBounds.y) {
            position.y += levelBounds.y - footprint.y;
        }

        float levelRight = levelBounds.x + levelBounds.width;
        float levelBottom = levelBounds.y + levelBounds.height;
        footprint = enemy.GetNavigationFootprintAt(position);
        if (footprint.x + footprint.width > levelRight) {
            position.x -= footprint.x + footprint.width - levelRight;
        }
        if (footprint.y + footprint.height > levelBottom) {
            position.y -= footprint.y + footprint.height - levelBottom;
        }
        return position;
    }

    /// Reports whether the map position clear condition is satisfied.
    bool IsMapPositionClear(
        const Enemy& enemy,
        Vector2 position,
        const IEnemyPathAccess& pathAccess
    ) {
        return !pathAccess.IsBlocked(
            enemy.GetNavigationFootprintAt(position)
        );
    }

    /// Searches for nearest clear position.
    std::optional<Vector2> FindNearestClearPosition(
        const Enemy& enemy,
        Vector2 origin,
        const IEnemyPathAccess& pathAccess,
        Rectangle levelBounds
    ) {
        origin = ClampFootprintToLevel(enemy, origin, levelBounds);
        if (IsMapPositionClear(enemy, origin, pathAccess)) return origin;

        CollisionMovementResult tinyRecovery =
            CollisionMovement::ResolveSlide(
                enemy.GetNavigationFootprintAt(origin),
                { 0.0f, 0.0f },
                [&pathAccess](Rectangle candidate) {
                    return pathAccess.IsBlocked(candidate);
                }
            );
        Vector2 recoveredOrigin = {
            origin.x + tinyRecovery.appliedDisplacement.x,
            origin.y + tinyRecovery.appliedDisplacement.y
        };
        if (IsMapPositionClear(enemy, recoveredOrigin, pathAccess)) {
            return recoveredOrigin;
        }

        auto testCandidate = [&](float x, float y)
            -> std::optional<Vector2> {
            Vector2 candidate = ClampFootprintToLevel(
                enemy,
                { origin.x + x, origin.y + y },
                levelBounds
            );
            if (IsMapPositionClear(enemy, candidate, pathAccess)) {
                return candidate;
            }
            return std::nullopt;
        };

        for (int radius = 1; radius <= EMBEDDED_RECOVERY_RADIUS; ++radius) {
            for (int offset = -radius; offset <= radius; ++offset) {
                if (auto position = testCandidate((float)offset, (float)-radius)) {
                    return position;
                }
                if (auto position = testCandidate((float)offset, (float)radius)) {
                    return position;
                }
            }
            for (int offset = -radius + 1; offset < radius; ++offset) {
                if (auto position = testCandidate((float)-radius, (float)offset)) {
                    return position;
                }
                if (auto position = testCandidate((float)radius, (float)offset)) {
                    return position;
                }
            }
        }
        return std::nullopt;
    }
}

/// Checks player attack overlap.
bool EnemyCollision::CheckPlayerAttackOverlap(
    const Enemy& enemy,
    const Paladin& player
) {
    if (!enemy.IsEnabled()) return false;

    return CheckCollisionRecs(
        enemy.GetContactAttackBoxAt(enemy.GetPosition()),
        player.GetCollisionBox()
    );
}

/// Checks parry.
bool EnemyCollision::CheckParry(
    const Enemy& enemy,
    const Paladin& player
) {
    return player.CanParryAttack(enemy.GetPosition());
}

/// Moves against walls.
EnemyMoveResult EnemyCollision::MoveAgainstWalls(
    Enemy& enemy,
    Vector2 displacement,
    const IEnemyPathAccess& pathAccess,
    EnemyWallResponse response
) {
    EnemyMoveResult result;
    Rectangle levelBounds = pathAccess.GetLevelBounds();
    Vector2 startPosition = enemy.GetPosition();
    std::optional<Vector2> clearStart = FindNearestClearPosition(
        enemy,
        startPosition,
        pathAccess,
        levelBounds
    );
    if (!clearStart) {
        result.finalPosition = startPosition;
        result.blockedX = displacement.x != 0.0f;
        result.blockedY = displacement.y != 0.0f;
        result.hitWall = result.blockedX || result.blockedY;
        return result;
    }
    enemy.SetPosition(*clearStart);

    auto isBlocked = [&pathAccess](Rectangle candidate) {
        return pathAccess.IsBlocked(candidate);
    };
    CollisionMovementResult movement =
        response == EnemyWallResponse::Stop
        ? CollisionMovement::ResolveStop(
            enemy.GetCollisionBox(),
            displacement,
            isBlocked
        )
        : CollisionMovement::ResolveSlide(
            enemy.GetCollisionBox(),
            displacement,
            isBlocked
        );

    result.finalPosition = {
        clearStart->x + movement.appliedDisplacement.x,
        clearStart->y + movement.appliedDisplacement.y
    };
    result.blockedX = movement.blockedX;
    result.blockedY = movement.blockedY;
    result.hitWall = movement.HitObstacle();
    enemy.SetPosition(result.finalPosition);
    return result;
}
