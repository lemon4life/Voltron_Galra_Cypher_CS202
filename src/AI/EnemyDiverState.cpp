#include "AI/EnemyState.h"

#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include "Entities/EnemyEntities/EnemyDiver.h"
#include "Entities/Player/Player.h"

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

    bool IsBlocked(LevelManager* levelManager, EnemyDiver* enemy) {
        return levelManager && levelManager->IsSolidCollision(enemy->GetBoundingBox());
    }

    Vector2 NormalizeOrFallback(Vector2 direction, Vector2 fallback) {
        if (Vector2Length(direction) <= MIN_DIRECTION_LENGTH) {
            return fallback;
        }
        return Vector2Normalize(direction);
    }
}

EnemyDiverChaseState::EnemyDiverChaseState(float offSightDistance)
    : offSightDistance(offSightDistance) {}

void EnemyDiverChaseState::Enter(EnemyDiver* enemy) {
    enemy->StartPathFinding();
}

void EnemyDiverChaseState::Update(EnemyDiver* enemy, float deltaTime) {
    UpdateAttackCooldown(enemy, deltaTime);

    Player* player = enemy->GetTarget();
    if (!player) {
        enemy->EndPathFinding();
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    Vector2 enemyPosition = enemy->GetPosition();
    Vector2 playerPosition = player->GetPosition();
    if (Vector2Distance(enemyPosition, playerPosition) > offSightDistance) {
        enemy->EndPathFinding();
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
    if (enemy->CanEnterReadyState(levelManager)) {
        enemy->EndPathFinding();
        enemy->ChangeState(enemy->GetReadyState());
        return;
    }

    if (enemy->IsWithinClearDiveRange(levelManager)) {
        enemy->EndPathFinding();
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

    Vector2 direction = NormalizeOrFallback(
        Vector2Subtract(moveTarget, enemyPosition),
        { 0.0f, 0.0f }
    );
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

    if (!CheckCollisionRecs(enemy->GetBoundingBox(), player->GetBoundingBox())) {
        return;
    }

    enemyPosition = enemy->GetPosition();
    Vector2 pushDirection = NormalizeOrFallback(
        Vector2Subtract(enemyPosition, playerPosition),
        { 1.0f, 0.0f }
    );

    Vector2 beforePush = enemyPosition;
    enemyPosition.x += pushDirection.x * PLAYER_SEPARATION_DISTANCE;
    enemy->SetPosition(enemyPosition);
    if (IsBlocked(levelManager, enemy)) {
        enemyPosition.x = beforePush.x;
        enemy->SetPosition(enemyPosition);
    }

    beforePush = enemy->GetPosition();
    enemyPosition = beforePush;
    enemyPosition.y += pushDirection.y * PLAYER_SEPARATION_DISTANCE;
    enemy->SetPosition(enemyPosition);
    if (IsBlocked(levelManager, enemy)) {
        enemyPosition.y = beforePush.y;
        enemy->SetPosition(enemyPosition);
    }
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

    Player* player = enemy->GetTarget();
    if (!player) {
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    Vector2 previousPosition = enemy->GetPosition();
    Vector2 awayDirection = NormalizeOrFallback(
        Vector2Subtract(previousPosition, player->GetPosition()),
        { 1.0f, 0.0f }
    );
    Vector2 nextPosition = Vector2Add(
        previousPosition,
        Vector2Scale(awayDirection, enemy->GetReadySpeed() * activeTime)
    );

    enemy->SetPosition(nextPosition);
    LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
    if (IsBlocked(levelManager, enemy)) {
        enemy->SetPosition(previousPosition);
    }

    if (dTimer <= 0.0f) {
        enemy->ChangeState(enemy->GetLungingState());
    }
}

void EnemyDiverReadyState::Exit(EnemyDiver* enemy) {
    dTimer = 0.0f;
}

void EnemyDiverLungingState::Enter(EnemyDiver* enemy) {
    enemy->EndPathFinding();

    Player* player = enemy->GetTarget();
    Vector2 targetPosition = player ? player->GetPosition() : enemy->GetPosition();
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
    float maximumSubstep = std::max(1.0f, std::min(body.width, body.height) / 2.0f);
    int substepCount = std::max(1, (int)std::ceil(frameDistance / maximumSubstep));
    float substepDistance = frameDistance / (float)substepCount;
    LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
    Player* player = enemy->GetTarget();

    for (int step = 0; step < substepCount; ++step) {
        Vector2 previousPosition = enemy->GetPosition();
        enemy->SetPosition(Vector2Add(
            previousPosition,
            Vector2Scale(lockedDirection, substepDistance)
        ));

        if (IsBlocked(levelManager, enemy)) {
            enemy->SetPosition(previousPosition);
            BeginRecovery(enemy);
            return;
        }

        if (!hasDamagedPlayer && player &&
            CheckCollisionRecs(enemy->GetBoundingBox(), player->GetBoundingBox())) {
            player->TakeDamage(enemy->GetDamage());
            hasDamagedPlayer = true;
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
