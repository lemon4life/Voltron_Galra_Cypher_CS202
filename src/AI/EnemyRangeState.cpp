#include "AI/EnemyState.h"

#include "Core/LevelAccess.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/EnemyEntities/EnemyRange.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Projectile.h"

#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float MIN_DIRECTION_LENGTH = 0.001f;
    constexpr float VELOCITY_SMOOTHING = 0.35f;
    constexpr float PLAYER_VELOCITY_LIMIT_MULTIPLIER = 1.5f;

    void UpdateAttackCooldown(Enemy* enemy, float deltaTime) {
        if (!enemy || enemy->GetAttackCooldown() <= 0.0f) return;

        float remainingCooldown = enemy->GetAttackCooldown() - deltaTime;
        enemy->SetAttackCooldown(std::max(0.0f, remainingCooldown));
    }

    bool IsBlocked(LevelManager* levelManager, Enemy* enemy) {
        return levelManager && levelManager->IsSolidCollision(enemy->GetBoundingBox());
    }

    Vector2 GetProjectileOrigin(EnemyRange* enemy, Vector2 targetPosition) {
        Vector2 enemyPosition = enemy->GetPosition();
        Vector2 direction = Vector2Subtract(targetPosition, enemyPosition);
        if (Vector2Length(direction) <= MIN_DIRECTION_LENGTH) {
            direction = { 1.0f, 0.0f };
        } else {
            direction = Vector2Normalize(direction);
        }

        Rectangle body = enemy->GetBoundingBox();
        float halfExtent =
            std::abs(direction.x) * body.width / 2.0f +
            std::abs(direction.y) * body.height / 2.0f;
        float spawnOffset = halfExtent + enemy->GetProjectileRadius() + 1.0f;

        return Vector2Add(enemyPosition, Vector2Scale(direction, spawnOffset));
    }

    bool HasClearShot(EnemyRange* enemy, Vector2 targetPosition) {
        if (!enemy || !enemy->GetLineOfSightQuery()) return false;

        Vector2 origin = GetProjectileOrigin(enemy, targetPosition);
        return enemy->GetLineOfSightQuery()->HasClearLineOfSight(
            origin,
            targetPosition,
            enemy->GetProjectileRadius()
        );
    }

    float SmallestPositiveRoot(float first, float second) {
        bool firstIsPositive = first > 0.0f;
        bool secondIsPositive = second > 0.0f;

        if (firstIsPositive && secondIsPositive) {
            return std::min(first, second);
        }
        if (firstIsPositive) return first;
        if (secondIsPositive) return second;
        return -1.0f;
    }
}

void EnemyRangeChaseState::Enter(EnemyRange* enemy) {
    enemy->StartPathFinding();
}

void EnemyRangeChaseState::Update(EnemyRange* enemy, float deltaTime) {
    UpdateAttackCooldown(enemy, deltaTime);

    TeamManager* targetTeam = enemy->GetTargetTeam();
    Paladin* player = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    if (!player) {
        enemy->EndPathFinding();
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    Vector2 enemyPosition = enemy->GetPosition();
    Vector2 playerPosition = player->GetPosition();

    if (enemy->IsBeyondDisengageDistance(playerPosition)) {
        enemy->EndPathFinding();
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
    if (enemy->IsWithinShootingDistance(playerPosition) &&
        HasClearShot(enemy, playerPosition)) {
        enemy->EndPathFinding();
        enemy->ChangeState(enemy->GetShootingState());
        return;
    }

    enemy->StartPathFinding();

    Vector2 moveTarget = playerPosition;
    if (levelManager) {
        moveTarget = levelManager->GetEnemyPathManager().GetNextMoveTarget(
            levelManager,
            enemy,
            playerPosition
        );
    }

    Vector2 direction = Vector2Subtract(moveTarget, enemyPosition);
    if (Vector2Length(direction) > MIN_DIRECTION_LENGTH) {
        direction = Vector2Normalize(direction);
    } else {
        direction = { 0.0f, 0.0f };
    }

    if (levelManager) {
        direction = levelManager->GetEnemyPathManager().GetLocalAvoidanceDirection(
            levelManager,
            enemy,
            direction
        );
    }

    float moveDistance = enemy->GetSpeed() * deltaTime;

    enemyPosition.x += direction.x * moveDistance;
    enemy->SetPosition(enemyPosition);
    if (IsBlocked(levelManager, enemy)) {
        enemyPosition.x -= direction.x * moveDistance;
        enemy->SetPosition(enemyPosition);
    }

    enemyPosition = enemy->GetPosition();
    enemyPosition.y += direction.y * moveDistance;
    enemy->SetPosition(enemyPosition);
    if (IsBlocked(levelManager, enemy)) {
        enemyPosition.y -= direction.y * moveDistance;
        enemy->SetPosition(enemyPosition);
    }
}

void EnemyRangeChaseState::Exit(EnemyRange* enemy) {
    enemy->EndPathFinding();
}

void EnemyRangeShootingState::Enter(EnemyRange* enemy) {
    enemy->EndPathFinding();
    estimatedPlayerVelocity = { 0.0f, 0.0f };

    TeamManager* targetTeam = enemy->GetTargetTeam();
    Paladin* player = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    hasPreviousPlayerPosition = player != nullptr;
    if (player) {
        previousPlayerPosition = player->GetPosition();
    }
}

void EnemyRangeShootingState::Update(EnemyRange* enemy, float deltaTime) {
    UpdateAttackCooldown(enemy, deltaTime);

    TeamManager* targetTeam = enemy->GetTargetTeam();
    Paladin* player = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    if (!player) {
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    Vector2 playerPosition = player->GetPosition();
    Vector2 predictedPosition = PredictTargetPosition(enemy, player, deltaTime);

    if (!enemy->IsWithinShootingDistance(playerPosition) ||
        !HasClearShot(enemy, predictedPosition)) {
        enemy->ChangeState(enemy->GetChaseState());
        return;
    }

    if (enemy->GetAttackCooldown() <= 0.0f) {
        FireProjectile(enemy, predictedPosition);
    }
}

void EnemyRangeShootingState::Exit(EnemyRange* enemy) {
    hasPreviousPlayerPosition = false;
    estimatedPlayerVelocity = { 0.0f, 0.0f };
}

Vector2 EnemyRangeShootingState::PredictTargetPosition(
    EnemyRange* enemy,
    Paladin* player,
    float deltaTime
) {
    Vector2 playerPosition = player->GetPosition();

    if (hasPreviousPlayerPosition && deltaTime > 0.0f) {
        Vector2 rawVelocity = Vector2Scale(
            Vector2Subtract(playerPosition, previousPlayerPosition),
            1.0f / deltaTime
        );

        float velocityLimit = std::max(
            1.0f,
            player->GetSpeed() * PLAYER_VELOCITY_LIMIT_MULTIPLIER
        );
        if (Vector2Length(rawVelocity) > velocityLimit) {
            rawVelocity = Vector2Scale(Vector2Normalize(rawVelocity), velocityLimit);
        }

        estimatedPlayerVelocity = Vector2Lerp(
            estimatedPlayerVelocity,
            rawVelocity,
            VELOCITY_SMOOTHING
        );
    }

    previousPlayerPosition = playerPosition;
    hasPreviousPlayerPosition = true;

    Vector2 projectileOrigin = GetProjectileOrigin(enemy, playerPosition);
    Vector2 relativePosition = Vector2Subtract(playerPosition, projectileOrigin);
    float projectileSpeed = enemy->GetProjectileSpeed();

    float a = Vector2DotProduct(estimatedPlayerVelocity, estimatedPlayerVelocity) -
              projectileSpeed * projectileSpeed;
    float b = 2.0f * Vector2DotProduct(relativePosition, estimatedPlayerVelocity);
    float c = Vector2DotProduct(relativePosition, relativePosition);
    float interceptTime = -1.0f;

    if (std::abs(a) <= MIN_DIRECTION_LENGTH) {
        if (std::abs(b) > MIN_DIRECTION_LENGTH) {
            float linearTime = -c / b;
            if (linearTime > 0.0f) {
                interceptTime = linearTime;
            }
        }
    } else {
        float discriminant = b * b - 4.0f * a * c;
        if (discriminant >= 0.0f) {
            float squareRoot = std::sqrt(discriminant);
            float firstRoot = (-b - squareRoot) / (2.0f * a);
            float secondRoot = (-b + squareRoot) / (2.0f * a);
            interceptTime = SmallestPositiveRoot(firstRoot, secondRoot);
        }
    }

    if (interceptTime < 0.0f) {
        interceptTime = Vector2Length(relativePosition) / projectileSpeed;
    }

    interceptTime = std::clamp(interceptTime, 0.0f, enemy->GetMaxPredictionTime());
    return Vector2Add(
        playerPosition,
        Vector2Scale(estimatedPlayerVelocity, interceptTime)
    );
}

void EnemyRangeShootingState::FireProjectile(
    EnemyRange* enemy,
    Vector2 targetPosition
) {
    Vector2 projectileOrigin = GetProjectileOrigin(enemy, targetPosition);
    Vector2 direction = Vector2Subtract(targetPosition, projectileOrigin);
    if (Vector2Length(direction) <= MIN_DIRECTION_LENGTH) {
        direction = { 1.0f, 0.0f };
    } else {
        direction = Vector2Normalize(direction);
    }

    float projectileRadius = enemy->GetProjectileRadius();
    Vector2 projectilePosition = {
        projectileOrigin.x - projectileRadius,
        projectileOrigin.y - projectileRadius
    };
    Vector2 velocity = Vector2Scale(direction, enemy->GetProjectileSpeed());

    Projectile* projectile = new Projectile(
        projectilePosition,
        velocity,
        enemy->GetProjectileLifetime(),
        enemy->GetDamage(),
        true
    );
    GameManager::GetInstance().AddProjectile(projectile);
    enemy->ResetAttackCooldown();
}
