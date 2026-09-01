#include "AI/EnemyState.h"

#include "AI/EnemyCollision.h"
#include "Core/LevelAccess.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/EnemyEntities/EnemyDiver.h"
#include "Entities/Player/Paladin.h"

#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float MIN_DIRECTION_LENGTH = 0.001f;

    /// Updates attack cooldown.
    void UpdateAttackCooldown(EnemyDiver* enemy, float deltaTime) {
        if (enemy->GetAttackCooldown() <= 0.0f) return;

        enemy->SetAttackCooldown(std::max(
            0.0f,
            enemy->GetAttackCooldown() - deltaTime
        ));
    }

    /// Implements the normalize or fallback behavior for this component.
    Vector2 NormalizeOrFallback(Vector2 direction, Vector2 fallback) {
        if (Vector2Length(direction) <= MIN_DIRECTION_LENGTH) {
            return fallback;
        }
        return Vector2Normalize(direction);
    }
}

/// Prepares this state when it becomes active.
void EnemyDiverChaseState::Enter(EnemyDiver* enemy) {
    enemy->StartPathFinding();
}

/// Advances this component's state for the current frame.
void EnemyDiverChaseState::Update(EnemyDiver* enemy, float deltaTime) {
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

    if (enemy->CanEnterReadyState()) {
        enemy->ChangeState(enemy->GetReadyState());
        return;
    }

    float playerDistance = Vector2Distance(enemyPosition, playerPosition);
    if (playerDistance < enemy->GetMinimumPlayerDistance()) {
        Vector2 retreatDirection = NormalizeOrFallback(
            Vector2Subtract(enemyPosition, playerPosition),
            { enemy->IsFacingLeft() ? 1.0f : -1.0f, 0.0f }
        );
        enemy->UpdateMovement(
            Vector2Scale(retreatDirection, enemy->GetSpeed()),
            deltaTime,
            EnemyWallResponse::Slide
        );
        return;
    }

    if (enemy->IsWithinClearDiveRange()) {
        return;
    }

    IEnemyPathAccess& pathAccess = enemy->GetPathAccess();
    std::optional<Vector2> moveTarget =
        pathAccess.GetNextMoveTarget(*enemy);
    Vector2 direction = { 0.0f, 0.0f };
    if (moveTarget) {
        direction = NormalizeOrFallback(
            Vector2Subtract(*moveTarget, enemyPosition),
            { 0.0f, 0.0f }
        );
        direction = pathAccess.GetLocalDirection(*enemy, direction);
    }

    enemy->UpdateMovement(
        Vector2Scale(direction, enemy->GetSpeed()),
        deltaTime,
        EnemyWallResponse::Slide
    );
}

/// Cleans up this state before control moves elsewhere.
void EnemyDiverChaseState::Exit(EnemyDiver* enemy) {
    enemy->EndPathFinding();
}

/// Prepares this state when it becomes active.
void EnemyDiverReadyState::Enter(EnemyDiver* enemy) {
    enemy->EndPathFinding();
    dTimer = enemy->GetReadyDuration();

    TeamManager* targetTeam = enemy->GetTargetTeam();
    Paladin* player = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    Vector2 attackDirection = player
        ? Vector2Subtract(player->GetPosition(), enemy->GetPosition())
        : Vector2{ 1.0f, 0.0f };
    enemy->BeginAttackPreparation(attackDirection);
}

/// Advances this component's state for the current frame.
void EnemyDiverReadyState::Update(EnemyDiver* enemy, float deltaTime) {
    float activeTime = std::min(std::max(deltaTime, 0.0f), dTimer);
    dTimer = std::max(0.0f, dTimer - deltaTime);
    enemy->AdvanceAttackPreparation(activeTime);

    TeamManager* targetTeam = enemy->GetTargetTeam();
    Paladin* player = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    if (!player) {
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    Vector2 awayDirection = Vector2Negate(
        enemy->GetLockedAttackDirection()
    );

    EnemyCollision::MoveAgainstWalls(
        *enemy,
        Vector2Scale(awayDirection, enemy->GetReadySpeed() * activeTime),
        enemy->GetPathAccess(),
        EnemyWallResponse::Stop
    );

    if (dTimer <= 0.0f) {
        enemy->ChangeState(enemy->GetLungingState());
    }
}

/// Cleans up this state before control moves elsewhere.
void EnemyDiverReadyState::Exit(EnemyDiver* enemy) {
    enemy->EndAttackPreparation();
    dTimer = 0.0f;
}

/// Prepares this state when it becomes active.
void EnemyDiverLungingState::Enter(EnemyDiver* enemy) {
    enemy->EndPathFinding();
    enemy->BeginAttackEffect();
    AudioManager::GetInstance().PlayRandomSwordSlash();
    dTimer = enemy->GetDiveDuration();
    isWaitingToChase = false;
    hasDamagedPlayer = false;
}

/// Advances this component's state for the current frame.
void EnemyDiverLungingState::Update(EnemyDiver* enemy, float deltaTime) {
    if (isWaitingToChase) {
        dTimer = std::max(0.0f, dTimer - deltaTime);
        if (dTimer <= 0.0f) {
            enemy->ResetAttackCooldown();
            enemy->ChangeState(enemy->GetChaseState());
        }
        return;
    }

    float activeTime = std::min(std::max(deltaTime, 0.0f), dTimer);
    dTimer = std::max(0.0f, dTimer - deltaTime);

    float frameDistance = enemy->GetDiveSpeed() * activeTime;
    Rectangle body = enemy->GetBoundingBox();
    float maximumSubstep = std::max(
        1.0f,
        std::min(body.width, body.height) / 2.0f
    );
    int substepCount = std::max(
        1,
        (int)std::ceil(frameDistance / maximumSubstep)
    );
    float substepDistance = frameDistance / (float)substepCount;
    float substepTime = activeTime / (float)substepCount;

    TeamManager* targetTeam = enemy->GetTargetTeam();
    Paladin* player = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    Vector2 attackDirection = enemy->GetLockedAttackDirection();

    for (int step = 0; step < substepCount; ++step) {
        EnemyMoveResult moveResult = EnemyCollision::MoveAgainstWalls(
            *enemy,
            Vector2Scale(attackDirection, substepDistance),
            enemy->GetPathAccess(),
            EnemyWallResponse::Stop
        );

        if (moveResult.hitWall) {
            BeginRecovery(enemy);
            return;
        }

        enemy->AdvanceAttackEffect(substepTime);

        if (!hasDamagedPlayer && player &&
            enemy->DoesAttackHit(player->GetCollisionBox())) {
            hasDamagedPlayer = true;
            Vector2 attackSource = enemy->GetAttackParrySourcePosition();

            if (player->CanParryAttack(attackSource)) {
                Vector2 playerPosition = player->GetPosition();
                player->TriggerParrySuccess(enemy);
                player->IncrementParryCount();
                GameManager::GetInstance().TriggerHitstop(0.3f);
                GameManager::GetInstance().AddImpactEffect(playerPosition);

                Vector2 pushDirection = NormalizeOrFallback(
                    Vector2Subtract(enemy->GetPosition(), playerPosition),
                    Vector2Negate(attackDirection)
                );
                enemy->ApplyCollisionPush(pushDirection, 60.0f);

                enemy->ResetAttackCooldown();
                enemy->ChangeState(enemy->GetDazeState());
                return;
            }

            player->TakeDamage(enemy->GetDamage());
        }
    }

    if (dTimer <= 0.0f) {
        BeginRecovery(enemy);
    }
}

/// Cleans up this state before control moves elsewhere.
void EnemyDiverLungingState::Exit(EnemyDiver* enemy) {
    enemy->EndAttackEffect();
    isWaitingToChase = false;
    hasDamagedPlayer = false;
    dTimer = 0.0f;
}

/// Begins recovery.
void EnemyDiverLungingState::BeginRecovery(EnemyDiver* enemy) {
    if (isWaitingToChase) return;

    enemy->EndPathFinding();
    enemy->EndAttackEffect();
    isWaitingToChase = true;
    dTimer = enemy->GetDiveRecoveryDuration();
}
