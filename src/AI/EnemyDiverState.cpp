#include "AI/EnemyState.h"

#include "AI/EnemyCollision.h"
#include "Core/LevelAccess.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/EnemyEntities/EnemyDiver.h"
#include "Entities/Player/Paladin.h"

#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float MIN_DIRECTION_LENGTH = 0.001f;
    constexpr float PLAYER_SEPARATION_DISTANCE = 20.0f;

    void UpdateAttackCooldown(EnemyDiver* enemy, float deltaTime) {
        if (enemy->GetAttackCooldown() <= 0.0f) return;

        enemy->SetAttackCooldown(std::max(
            0.0f,
            enemy->GetAttackCooldown() - deltaTime
        ));
    }

    Vector2 NormalizeOrFallback(Vector2 direction, Vector2 fallback) {
        if (Vector2Length(direction) <= MIN_DIRECTION_LENGTH) {
            return fallback;
        }
        return Vector2Normalize(direction);
    }
}

void EnemyDiverChaseState::Enter(EnemyDiver* enemy) {
    enemy->StartPathFinding();
}

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

    enemy->UpdateMovement(Vector2Scale(direction, enemy->GetSpeed()), deltaTime, EnemyWallResponse::Slide);

    if (!EnemyCollision::CheckPlayerCollision(*enemy, *player)) {
        return;
    }

    enemyPosition = enemy->GetPosition();
    Vector2 pushDirection = NormalizeOrFallback(
        Vector2Subtract(enemyPosition, playerPosition),
        { 1.0f, 0.0f }
    );

    EnemyCollision::MoveAgainstWalls(
        *enemy,
        Vector2Scale(pushDirection, PLAYER_SEPARATION_DISTANCE),
        pathAccess,
        EnemyWallResponse::Slide
    );
}

void EnemyDiverChaseState::Exit(EnemyDiver* enemy) {
    enemy->EndPathFinding();
}

void EnemyDiverReadyState::Enter(EnemyDiver* enemy) {
    enemy->EndPathFinding();
    dTimer = enemy->GetReadyDuration();
}

void EnemyDiverReadyState::Update(EnemyDiver* enemy, float deltaTime) {
    float activeTime = std::min(std::max(deltaTime, 0.0f), dTimer);
    dTimer = std::max(0.0f, dTimer - deltaTime);

    TeamManager* targetTeam = enemy->GetTargetTeam();
    Paladin* player = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    if (!player) {
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    Vector2 awayDirection = NormalizeOrFallback(
        Vector2Subtract(enemy->GetPosition(), player->GetPosition()),
        { 1.0f, 0.0f }
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

void EnemyDiverReadyState::Exit(EnemyDiver* enemy) {
    dTimer = 0.0f;
}

void EnemyDiverLungingState::Enter(EnemyDiver* enemy) {
    enemy->EndPathFinding();

    TeamManager* targetTeam = enemy->GetTargetTeam();
    Paladin* player = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    Vector2 targetPosition = player
        ? player->GetPosition()
        : enemy->GetPosition();
    lockedDirection = NormalizeOrFallback(
        Vector2Subtract(targetPosition, enemy->GetPosition()),
        { 1.0f, 0.0f }
    );
    dTimer = enemy->GetDiveDuration();
    isWaitingToChase = false;
    hasDamagedPlayer = false;
}

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

    TeamManager* targetTeam = enemy->GetTargetTeam();
    Paladin* player = targetTeam ? targetTeam->GetActivePaladin() : nullptr;

    for (int step = 0; step < substepCount; ++step) {
        EnemyMoveResult moveResult = EnemyCollision::MoveAgainstWalls(
            *enemy,
            Vector2Scale(lockedDirection, substepDistance),
            enemy->GetPathAccess(),
            EnemyWallResponse::Stop
        );

        if (moveResult.hitWall) {
            BeginRecovery(enemy);
            return;
        }

        if (!hasDamagedPlayer && player &&
            EnemyCollision::CheckPlayerCollision(*enemy, *player)) {
            hasDamagedPlayer = true;

            if (EnemyCollision::CheckParry(*enemy, *player)) {
                Vector2 playerPosition = player->GetPosition();
                player->TriggerParrySuccess(enemy);
                player->IncrementParryCount();
                GameManager::GetInstance().TriggerHitstop(0.3f);
                GameManager::GetInstance().AddImpactEffect(playerPosition);

                Vector2 pushDirection = NormalizeOrFallback(
                    Vector2Subtract(enemy->GetPosition(), playerPosition),
                    Vector2Negate(lockedDirection)
                );
                EnemyCollision::MoveAgainstWalls(
                    *enemy,
                    Vector2Scale(pushDirection, 60.0f),
                    enemy->GetPathAccess(),
                    EnemyWallResponse::Slide
                );

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

void EnemyDiverLungingState::Exit(EnemyDiver* enemy) {
    lockedDirection = { 0.0f, 0.0f };
    isWaitingToChase = false;
    hasDamagedPlayer = false;
    dTimer = 0.0f;
}

void EnemyDiverLungingState::BeginRecovery(EnemyDiver* enemy) {
    if (isWaitingToChase) return;

    enemy->EndPathFinding();
    isWaitingToChase = true;
    dTimer = enemy->GetDiveRecoveryDuration();
}
