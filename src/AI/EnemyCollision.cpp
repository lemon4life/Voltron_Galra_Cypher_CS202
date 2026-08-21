#include "AI/EnemyCollision.h"

#include "Core/LevelAccess.h"
#include "Entities/Enemy.h"
#include "Entities/Player/Paladin.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace {
    constexpr float MAX_MOVEMENT_SUBSTEP = 2.0f;
    constexpr int EMBEDDED_RECOVERY_RADIUS = 32;

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

    bool IsMapPositionClear(
        const Enemy& enemy,
        Vector2 position,
        const IEnemyPathAccess& pathAccess
    ) {
        return !pathAccess.IsBlocked(
            enemy.GetNavigationFootprintAt(position)
        );
    }

    std::optional<Vector2> FindNearestClearPosition(
        const Enemy& enemy,
        Vector2 origin,
        const IEnemyPathAccess& pathAccess,
        Rectangle levelBounds
    ) {
        origin = ClampFootprintToLevel(enemy, origin, levelBounds);
        if (IsMapPositionClear(enemy, origin, pathAccess)) return origin;

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

bool EnemyCollision::CheckParry(
    const Enemy& enemy,
    const Paladin& player
) {
    return player.CanParryAttack(enemy.GetPosition());
}

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

    float maximumAxisDistance = std::max(
        std::abs(displacement.x),
        std::abs(displacement.y)
    );
    int stepCount = std::max(
        1,
        (int)std::ceil(maximumAxisDistance / MAX_MOVEMENT_SUBSTEP)
    );
    Vector2 step = {
        displacement.x / (float)stepCount,
        displacement.y / (float)stepCount
    };

    for (int stepIndex = 0; stepIndex < stepCount; ++stepIndex) {
        Vector2 currentPosition = enemy.GetPosition();
        if (response == EnemyWallResponse::Stop) {
            Vector2 nextPosition = ClampFootprintToLevel(
                enemy,
                {
                    currentPosition.x + step.x,
                    currentPosition.y + step.y
                },
                levelBounds
            );
            if (!IsMapPositionClear(enemy, nextPosition, pathAccess)) {
                result.blockedX = result.blockedX || step.x != 0.0f;
                result.blockedY = result.blockedY || step.y != 0.0f;
                result.hitWall = true;
                break;
            }
            enemy.SetPosition(nextPosition);
            continue;
        }

        if (step.x != 0.0f) {
            Vector2 nextX = ClampFootprintToLevel(
                enemy,
                { currentPosition.x + step.x, currentPosition.y },
                levelBounds
            );
            if (IsMapPositionClear(enemy, nextX, pathAccess)) {
                enemy.SetPosition(nextX);
            } else {
                result.blockedX = true;
                result.hitWall = true;
            }
        }

        currentPosition = enemy.GetPosition();
        if (step.y != 0.0f) {
            Vector2 nextY = ClampFootprintToLevel(
                enemy,
                { currentPosition.x, currentPosition.y + step.y },
                levelBounds
            );
            if (IsMapPositionClear(enemy, nextY, pathAccess)) {
                enemy.SetPosition(nextY);
            } else {
                result.blockedY = true;
                result.hitWall = true;
            }
        }
    }

    result.finalPosition = enemy.GetPosition();
    return result;
}
