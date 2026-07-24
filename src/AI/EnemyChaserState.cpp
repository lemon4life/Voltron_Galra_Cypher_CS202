#include "AI/EnemyState.h"

#include "AI/EnemyCollision.h"
#include "Core/LevelAccess.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/EnemyEntities/EnemyChaser.h"
#include "Entities/Player/Paladin.h"

#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float MIN_DIRECTION_LENGTH = 0.001f;

    Vector2 NormalizeOrFallback(Vector2 direction, Vector2 fallback) {
        if (Vector2Length(direction) <= MIN_DIRECTION_LENGTH) {
            return fallback;
        }
        return Vector2Normalize(direction);
    }

    void FinishDamageAttempt(EnemyChaser* enemy) {
        enemy->ResetAttackCooldown();
        enemy->ChangeState(enemy->GetChaseState());
    }
}

void EnemyChaserChaseState::Enter(EnemyChaser* enemy) {
    enemy->StartPathFinding();
}

void EnemyChaserChaseState::Update(EnemyChaser* enemy, float deltaTime) {
    TeamManager* targetTeam = enemy->GetTargetTeam();
    Paladin* activePaladin = targetTeam
        ? targetTeam->GetActivePaladin()
        : nullptr;
    if (!activePaladin) return;

    Vector2 enemyPosition = enemy->GetPosition();
    Vector2 playerPosition = activePaladin->GetPosition();
    bool isWithinAggroRange =
        enemy->IsWithinAggroRange(playerPosition);
    bool isWithinAttackRange =
        enemy->IsWithinStopPathFindingDistance(playerPosition);
    enemy->UpdateAggroMeter(isWithinAggroRange, deltaTime);

    if (enemy->IsBeyondDisengageDistance(playerPosition)) {
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    if (enemy->GetAttackCooldown() > 0.0f) {
        float remainingCooldown = enemy->GetAttackCooldown() - deltaTime;
        enemy->SetAttackCooldown(std::max(0.0f, remainingCooldown));
    }

    if (isWithinAttackRange) {
        if (enemy->GetAttackCooldown() <= 0.0f &&
            enemy->IsAggroReady()) {
            enemy->ResetAggroMeter();
            enemy->ChangeState(enemy->GetDamageState());
        }
        return;
    }

    IEnemyPathAccess& pathAccess = enemy->GetPathAccess();
    Vector2 moveTarget = pathAccess.GetNextMoveTarget(
        *enemy,
        playerPosition
    );
    Vector2 direction = NormalizeOrFallback(
        Vector2Subtract(moveTarget, enemyPosition),
        { 0.0f, 0.0f }
    );
    direction = pathAccess.GetLocalDirection(*enemy, direction);

    EnemyCollision::MoveAgainstWalls(
        *enemy,
        Vector2Scale(direction, enemy->GetSpeed() * deltaTime),
        pathAccess,
        EnemyWallResponse::Slide
    );
}

void EnemyChaserChaseState::Exit(EnemyChaser* enemy) {
    enemy->EndPathFinding();
}

void EnemyChaserDamageState::Enter(EnemyChaser* enemy) {
    TeamManager* targetTeam = enemy->GetTargetTeam();
    Paladin* player = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    Vector2 targetPosition = player
        ? player->GetPosition()
        : enemy->GetPosition();

    chargeDirection = NormalizeOrFallback(
        Vector2Subtract(targetPosition, enemy->GetPosition()),
        { 1.0f, 0.0f }
    );
    remainingChargeDistance = std::max(
        0.0f,
        enemy->GetDamageChargeDistance()
    );
    dTimer = std::max(0.0f, enemy->GetDamageChargeDuration());
    attackResolved = false;
}

void EnemyChaserDamageState::Update(
    EnemyChaser* enemy,
    float deltaTime
) {
    TeamManager* targetTeam = enemy->GetTargetTeam();
    Paladin* player = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    if (!player) {
        FinishDamageAttempt(enemy);
        return;
    }

    float activeTime = std::min(std::max(deltaTime, 0.0f), dTimer);
    dTimer = std::max(0.0f, dTimer - deltaTime);

    float chargeDuration = std::max(
        MIN_DIRECTION_LENGTH,
        enemy->GetDamageChargeDuration()
    );
    float chargeSpeed = enemy->GetDamageChargeDistance() / chargeDuration;
    float frameDistance = std::min(
        remainingChargeDistance,
        chargeSpeed * activeTime
    );

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

    for (int step = 0; step < substepCount; ++step) {
        EnemyMoveResult moveResult = EnemyCollision::MoveAgainstWalls(
            *enemy,
            Vector2Scale(chargeDirection, substepDistance),
            enemy->GetPathAccess(),
            EnemyWallResponse::Stop
        );

        if (moveResult.hitWall) {
            FinishDamageAttempt(enemy);
            return;
        }

        remainingChargeDistance = std::max(
            0.0f,
            remainingChargeDistance - substepDistance
        );

        if (attackResolved ||
            !EnemyCollision::CheckPlayerCollision(*enemy, *player)) {
            continue;
        }

        attackResolved = true;
        if (EnemyCollision::CheckParry(*enemy, *player)) {
            Vector2 playerPosition = player->GetPosition();
            player->TriggerParrySuccess(enemy);
            player->IncrementParryCount();
            GameManager::GetInstance().TriggerHitstop(0.3f);
            GameManager::GetInstance().AddImpactEffect(playerPosition);

            Vector2 pushDirection = NormalizeOrFallback(
                Vector2Subtract(enemy->GetPosition(), playerPosition),
                Vector2Negate(chargeDirection)
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
        if (player->IsParrying()) {
            player->ChangeState(player->GetIdleState());
        }
    }

    if (dTimer <= 0.0f || remainingChargeDistance <= 0.0f) {
        FinishDamageAttempt(enemy);
    }
}

void EnemyChaserDamageState::Exit(EnemyChaser* enemy) {
    dTimer = 0.0f;
    remainingChargeDistance = 0.0f;
    chargeDirection = { 0.0f, 0.0f };
    attackResolved = false;
}
