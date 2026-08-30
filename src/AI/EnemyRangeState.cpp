#include "AI/EnemyState.h"

#include "AI/EnemyCollision.h"
#include "Core/LevelAccess.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/AudioManager.h"
#include "Entities/EnemyEntities/EnemyRange.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Projectile.h"

#include "raymath.h"
#include "Core/Constants.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float MIN_DIRECTION_LENGTH = 0.001f;
    constexpr int MIN_AIM_ERROR_CENTIDEGREES = -100;
    constexpr int MAX_AIM_ERROR_CENTIDEGREES = 100;

    /// Updates attack cooldown.
    void UpdateAttackCooldown(Enemy* enemy, float deltaTime) {
        if (!enemy || enemy->GetAttackCooldown() <= 0.0f) return;

        float remainingCooldown = enemy->GetAttackCooldown() - deltaTime;
        enemy->SetAttackCooldown(std::max(0.0f, remainingCooldown));
    }

    /// Returns the current projectile origin.
    Vector2 GetProjectileOrigin(EnemyRange* enemy, Vector2 targetPosition) {
        Vector2 enemyPosition = enemy->GetPosition();
        Vector2 direction = Vector2Subtract(targetPosition, enemyPosition);
        if (Vector2Length(direction) <= MIN_DIRECTION_LENGTH) {
            direction = { 1.0f, 0.0f };
        } else {
            direction = Vector2Normalize(direction);
        }

        bool facingLeft = targetPosition.x < enemyPosition.x;
        Vector2 pivot = { enemyPosition.x, enemyPosition.y + 5.0f };
        
        float localX = Constants::KNIGHT_PROJECTILE_OFFSET.x;
        float localY = facingLeft ? -4.0f : 4.0f;
        float angle = atan2f(direction.y, direction.x);
        
        Vector2 rotatedOffset = {
            localX * cosf(angle) - localY * sinf(angle),
            localX * sinf(angle) + localY * cosf(angle)
        };
        
        return Vector2Add(pivot, rotatedOffset);
    }

    /// Reports whether this component has clear shot.
    bool HasClearShot(EnemyRange* enemy, Vector2 targetPosition) {
        Vector2 enemyPosition = enemy->GetPosition();
        Vector2 projectileOrigin = GetProjectileOrigin(enemy, targetPosition);
        float projectileRadius = enemy->GetProjectileRadius();
        const ILevelLineOfSightQuery& lineOfSight =
            enemy->GetLineOfSightQuery();

        return lineOfSight.HasClearLineOfSight(
                   enemyPosition,
                   targetPosition,
                   projectileRadius
               ) &&
               lineOfSight.HasClearLineOfSight(
                   enemyPosition,
                   projectileOrigin,
                   projectileRadius
               ) &&
               lineOfSight.HasClearLineOfSight(
                   projectileOrigin,
                   targetPosition,
                   projectileRadius
               );
    }

}

/// Prepares this state when it becomes active.
void EnemyRangeChaseState::Enter(EnemyRange* enemy) {
    enemy->StartPathFinding();
}

/// Advances this component's state for the current frame.
void EnemyRangeChaseState::Update(EnemyRange* enemy, float deltaTime) {
    UpdateAttackCooldown(enemy, deltaTime);

    TeamManager* targetTeam = enemy->GetTargetTeam();
    Paladin* player = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    if (!player) {
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    Vector2 enemyPosition = enemy->GetPosition();
    Vector2 playerPosition = player->GetPosition();

    if (enemy->IsBeyondDisengageDistance(playerPosition)) {
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    if (enemy->IsWithinShootingDistance(playerPosition) &&
        HasClearShot(enemy, playerPosition)) {
        enemy->ChangeState(enemy->GetShootingState());
        return;
    }

    IEnemyPathAccess& pathAccess = enemy->GetPathAccess();

    std::optional<Vector2> moveTarget =
        pathAccess.GetNextMoveTarget(*enemy);
    Vector2 direction = { 0.0f, 0.0f };
    if (moveTarget) {
        direction = Vector2Subtract(*moveTarget, enemyPosition);
        if (Vector2Length(direction) > MIN_DIRECTION_LENGTH) {
            direction = Vector2Normalize(direction);
        } else {
            direction = { 0.0f, 0.0f };
        }
        direction = pathAccess.GetLocalDirection(*enemy, direction);
    }

    enemy->UpdateMovement(Vector2Scale(direction, enemy->GetSpeed()), deltaTime, EnemyWallResponse::Slide);
}

/// Cleans up this state before control moves elsewhere.
void EnemyRangeChaseState::Exit(EnemyRange* enemy) {
    enemy->EndPathFinding();
}

/// Prepares this state when it becomes active.
void EnemyRangeShootingState::Enter(EnemyRange* enemy) {
    enemy->EndPathFinding();
}

/// Advances this component's state for the current frame.
void EnemyRangeShootingState::Update(EnemyRange* enemy, float deltaTime) {
    UpdateAttackCooldown(enemy, deltaTime);

    TeamManager* targetTeam = enemy->GetTargetTeam();
    Paladin* player = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    if (!player) {
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    Vector2 playerPosition = player->GetPosition();
    bool hasDirectShot = HasClearShot(enemy, playerPosition);

    if (!enemy->IsWithinShootingDistance(playerPosition) ||
        !hasDirectShot) {
        enemy->ChangeState(enemy->GetChaseState());
        return;
    }

    if (enemy->GetAttackCooldown() <= 0.0f &&
        !TryFireProjectile(enemy, playerPosition)) {
        enemy->ChangeState(enemy->GetChaseState());
    }
}

/// Cleans up this state before control moves elsewhere.
void EnemyRangeShootingState::Exit(EnemyRange*) {
}

/// Attempts to fire projectile.
bool EnemyRangeShootingState::TryFireProjectile(
    EnemyRange* enemy,
    Vector2 targetPosition
) {
    if (!HasClearShot(enemy, targetPosition)) {
        return false;
    }

    Vector2 projectileOrigin = GetProjectileOrigin(enemy, targetPosition);
    Vector2 baseDirection = Vector2Subtract(targetPosition, projectileOrigin);
    float targetDistance = Vector2Length(baseDirection);
    if (targetDistance <= MIN_DIRECTION_LENGTH) {
        return false;
    }
    baseDirection = Vector2Scale(baseDirection, 1.0f / targetDistance);

    float aimErrorDegrees = (float)GetRandomValue(
        MIN_AIM_ERROR_CENTIDEGREES,
        MAX_AIM_ERROR_CENTIDEGREES
    ) / 100.0f;
    Vector2 direction = Vector2Rotate(
        baseDirection,
        aimErrorDegrees * DEG2RAD
    );

    float projectileRadius = enemy->GetProjectileRadius();
    auto hasClearFinalPath = [&](Vector2 candidateDirection) {
        Vector2 validationEnd = Vector2Add(
            projectileOrigin,
            Vector2Scale(candidateDirection, targetDistance)
        );
        return enemy->GetLineOfSightQuery().HasClearLineOfSight(
            projectileOrigin,
            validationEnd,
            projectileRadius
        );
    };

    if (!hasClearFinalPath(direction)) {
        direction = baseDirection;
        if (!hasClearFinalPath(direction)) {
            return false;
        }
    }

    Vector2 velocity = Vector2Scale(direction, enemy->GetProjectileSpeed());

    auto projectile = std::make_unique<Projectile>(
        projectileOrigin,
        velocity,
        enemy->GetProjectileLifetime(),
        enemy->GetDamage(),
        enemy->GetSprites().projectile,
        true
    );
    GameManager::GetInstance().AddProjectile(std::move(projectile));
    enemy->GetKinematics().ApplyRecoil(direction, 15.0f);
    AudioManager::GetInstance().PlayRandomLaser();
    enemy->ResetAttackCooldown();
    return true;
}
