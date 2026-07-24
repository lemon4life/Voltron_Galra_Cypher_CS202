#include "AI/EnemyCollision.h"

#include "Core/LevelAccess.h"
#include "Entities/Enemy.h"
#include "Entities/Player/Paladin.h"

#include <algorithm>

namespace {
    Vector2 ClampToLevel(Vector2 position, Rectangle levelBounds) {
        return {
            std::clamp(
                position.x,
                levelBounds.x,
                levelBounds.x + levelBounds.width
            ),
            std::clamp(
                position.y,
                levelBounds.y,
                levelBounds.y + levelBounds.height
            )
        };
    }
}

bool EnemyCollision::CheckPlayerCollision(
    const Enemy& enemy,
    const Paladin& player
) {
    return CheckCollisionRecs(
        enemy.GetBoundingBox(),
        player.GetBoundingBox()
    );
}

bool EnemyCollision::CheckEnemyCollision(
    const Enemy& enemy,
    const Enemy& other
) {
    if (&enemy == &other || other.IsDead()) {
        return false;
    }

    return CheckCollisionRecs(
        enemy.GetBoundingBox(),
        other.GetBoundingBox()
    );
}

bool EnemyCollision::CheckAnyEnemyCollision(
    const Enemy& enemy,
    const std::vector<GameObject*>& entities
) {
    for (GameObject* entity : entities) {
        if (entity->GetObjectType() != GameObjectType::Enemy) {
            continue;
        }

        Enemy* other = static_cast<Enemy*>(entity);
        if (CheckEnemyCollision(enemy, *other)) {
            return true;
        }
    }

    return false;
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
    Vector2 startPosition = enemy.GetPosition();
    Rectangle levelBounds = pathAccess.GetLevelBounds();

    if (response == EnemyWallResponse::Stop) {
        Vector2 nextPosition = ClampToLevel(
            Vector2{
                startPosition.x + displacement.x,
                startPosition.y + displacement.y
            },
            levelBounds
        );

        enemy.SetPosition(nextPosition);
        if (pathAccess.IsBlocked(enemy.GetBoundingBox())) {
            enemy.SetPosition(startPosition);
            result.blockedX = displacement.x != 0.0f;
            result.blockedY = displacement.y != 0.0f;
            result.hitWall = true;
        }

        result.finalPosition = enemy.GetPosition();
        return result;
    }

    Vector2 currentPosition = startPosition;
    currentPosition.x = ClampToLevel(
        Vector2{ currentPosition.x + displacement.x, currentPosition.y },
        levelBounds
    ).x;
    enemy.SetPosition(currentPosition);
    if (pathAccess.IsBlocked(enemy.GetBoundingBox())) {
        currentPosition.x = startPosition.x;
        enemy.SetPosition(currentPosition);
        result.blockedX = true;
        result.hitWall = true;
    }

    Vector2 beforeY = enemy.GetPosition();
    currentPosition = beforeY;
    currentPosition.y = ClampToLevel(
        Vector2{ currentPosition.x, currentPosition.y + displacement.y },
        levelBounds
    ).y;
    enemy.SetPosition(currentPosition);
    if (pathAccess.IsBlocked(enemy.GetBoundingBox())) {
        currentPosition.y = beforeY.y;
        enemy.SetPosition(currentPosition);
        result.blockedY = true;
        result.hitWall = true;
    }

    result.finalPosition = enemy.GetPosition();
    return result;
}
