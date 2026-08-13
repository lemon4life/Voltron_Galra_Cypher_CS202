#include "AI/EnemyState.h"

#include "AI/EnemyCollision.h"
#include "Core/LevelAccess.h"

#include "Entities/EnemyEntities/Boss.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/TeamManager.h"

#include "raymath.h"

#include <algorithm>

namespace {
    constexpr int BOSS_IDLE_MIN_MILLISECONDS = 3000;
    constexpr int BOSS_IDLE_MAX_MILLISECONDS = 5000;
    constexpr int BOSS_CHASE_MIN_MILLISECONDS = 5000;
    constexpr int BOSS_CHASE_MAX_MILLISECONDS = 7000;
    constexpr int BOSS_SPELL_MIN_MILLISECONDS = 4000;
    constexpr int BOSS_SPELL_MAX_MILLISECONDS = 6000;
    constexpr float BOSS_SPELL_SUMMON_INTERVAL = 0.5f;

    float RollDuration(int minimumMilliseconds, int maximumMilliseconds) {
        return (float)GetRandomValue(
            minimumMilliseconds,
            maximumMilliseconds
        ) / 1000.0f;
    }
}

// Boss Idling State

void BossIdlingState::Enter(Boss* enemy) {
    elapsedTime = 0.0f;
    idleDuration = RollDuration(
        BOSS_IDLE_MIN_MILLISECONDS,
        BOSS_IDLE_MAX_MILLISECONDS
    );
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
}

void BossIdlingState::Update(Boss* enemy, float deltaTime) {
    elapsedTime += std::max(0.0f, deltaTime);
    if (elapsedTime >= idleDuration) {
        IEnemyState* nextState = GetRandomValue(0, 1) == 0
            ? enemy->GetChaseState()
            : enemy->GetSpellingState();
        enemy->ChangeState(nextState);
    }
}

void BossIdlingState::Exit(Boss* enemy) {
}

// Boss Chase State

void BossChaseState::Enter(Boss* enemy) {
    elapsedTime = 0.0f;
    chaseDuration = RollDuration(
        BOSS_CHASE_MIN_MILLISECONDS,
        BOSS_CHASE_MAX_MILLISECONDS
    );
    enemy->StartPathFinding();
}

void BossChaseState::Update(Boss* enemy, float deltaTime) {
    elapsedTime += std::max(0.0f, deltaTime);
    if (elapsedTime >= chaseDuration) {
        enemy->ChangeState(enemy->GetIdlingState());
        return;
    }

    if (!enemy->GetTargetTeam() ||
        !enemy->GetTargetTeam()->GetActivePaladin()) {
        return;
    }

    Vector2 ePos = enemy->GetPosition();
    Vector2 pPos = enemy->GetTargetTeam()->GetActivePaladin()->GetPosition();

    IEnemyPathAccess& pathAccess = enemy->GetPathAccess();
    std::optional<Vector2> moveTarget =
        pathAccess.GetNextMoveTarget(*enemy);
    Vector2 dir = { 0.0f, 0.0f };
    if (moveTarget) {
        dir = Vector2Subtract(*moveTarget, ePos);
        if (Vector2Length(dir) > 0.0f) {
            dir = Vector2Normalize(dir);
        }
        dir = pathAccess.GetLocalDirection(*enemy, dir);
    }

    EnemyCollision::MoveAgainstWalls(
        *enemy,
        Vector2Scale(dir, enemy->GetSpeed() * deltaTime),
        pathAccess,
        EnemyWallResponse::Slide
    );

    // Handle Attack Cooldown
    if (enemy->GetAttackCooldown() > 0.0f) {
        float remainingCooldown = enemy->GetAttackCooldown() - deltaTime;
        enemy->SetAttackCooldown(remainingCooldown > 0.0f ? remainingCooldown : 0.0f);
    }
    
    // Check collision with Player for overlap resolution and damage
    Paladin* activePaladin = enemy->GetTargetTeam()->GetActivePaladin();
    if (EnemyCollision::CheckPlayerCollision(*enemy, *activePaladin)) {
        // Attack if cooldown allows
        if (enemy->GetAttackCooldown() <= 0.0f) {
            activePaladin->TakeDamage(enemy->GetDamage());
            enemy->ResetAttackCooldown();
        }
        
        // Separation knockback (push enemy away from player to prevent freeze/deadlock)
        ePos = enemy->GetPosition();
        Vector2 pushDir = Vector2Subtract(ePos, pPos);
        if (Vector2Length(pushDir) == 0.0f) pushDir = {1.0f, 0.0f}; // Fallback if exactly on top
        pushDir = Vector2Normalize(pushDir);

        enemy->ApplyCollisionPush(pushDir, 20.0f);
    }
}

void BossChaseState::Exit(Boss* enemy) {
    enemy->EndPathFinding();
}

// Boss Spelling State

void BossSpellingState::Enter(Boss* enemy) {
    elapsedTime = 0.0f;
    spellDuration = RollDuration(
        BOSS_SPELL_MIN_MILLISECONDS,
        BOSS_SPELL_MAX_MILLISECONDS
    );
    nextSummonCheck = BOSS_SPELL_SUMMON_INTERVAL;
    enemy->EndPathFinding();
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
    enemy->ResetAnimationCycle();
}

void BossSpellingState::Update(Boss* enemy, float deltaTime) {
    elapsedTime += std::max(0.0f, deltaTime);

    while (nextSummonCheck <= spellDuration &&
           elapsedTime >= nextSummonCheck) {
        if (GetRandomValue(0, 1) == 0) {
            enemy->TrySummonRandomEnemy();
        }
        nextSummonCheck += BOSS_SPELL_SUMMON_INTERVAL;
    }

    if (elapsedTime >= spellDuration) {
        enemy->ChangeState(enemy->GetIdlingState());
    }
}

void BossSpellingState::Exit(Boss* enemy) {
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
}
