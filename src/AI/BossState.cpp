#include "AI/EnemyState.h"

#include "AI/EnemyCollision.h"
#include "Core/LevelAccess.h"

#include "Entities/EnemyEntities/Boss.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/TeamManager.h"

#include "raymath.h"

#include <algorithm>
#include <array>

namespace {
    constexpr int BOSS_CHASE_MIN_MILLISECONDS = 5000;
    constexpr int BOSS_CHASE_MAX_MILLISECONDS = 7000;
    constexpr int BOSS_SPELL_MIN_MILLISECONDS = 4000;
    constexpr int BOSS_SPELL_MAX_MILLISECONDS = 6000;
    constexpr int BOSS_PUNCH_READY_FRAME_COUNT = 10;
    constexpr int BOSS_PUNCH_PLAY_FRAME_COUNT = 4;
    constexpr int BOSS_PUNCH_FIRE_FRAME_INDEX = 2;
    constexpr float BOSS_PUNCH_FRAME_DURATION = 0.06f;
    constexpr int BOSS_STOMP_FRAME_COUNT = 5;
    constexpr float BOSS_STOMP_SLOW_FRAME_DURATION = 0.35f;
    constexpr float BOSS_STOMP_FAST_FRAME_DURATION = 0.10f;

    enum class BossOffense {
        Chase,
        Spell,
        Punch,
        Stomp
    };

    struct BossOffenseChoice {
        BossOffense offense;
        int probabilityPercent;
    };

    // The percentage of Boss entering each offense state after the idle state
    constexpr std::array<BossOffenseChoice, 4> BOSS_OFFENSE_CHOICES = {{
        { BossOffense::Chase, 20 },
        { BossOffense::Spell, 27 },
        { BossOffense::Punch, 26 },
        { BossOffense::Stomp, 27 }
    }};

    float GetStompFrameDuration(int frameIndex) {
        return frameIndex < 2
            ? BOSS_STOMP_SLOW_FRAME_DURATION
            : BOSS_STOMP_FAST_FRAME_DURATION;
    }

    float RollDuration(int minimumMilliseconds, int maximumMilliseconds) {
        return (float)GetRandomValue(
            minimumMilliseconds,
            maximumMilliseconds
        ) / 1000.0f;
    }

    BossOffense RollBossOffense() {
        int roll = GetRandomValue(1, 100);
        int cumulativeProbability = 0;
        for (const BossOffenseChoice& choice : BOSS_OFFENSE_CHOICES) {
            cumulativeProbability += choice.probabilityPercent;
            if (roll <= cumulativeProbability) {
                return choice.offense;
            }
        }

        return BossOffense::Chase;
    }
}

// Boss Idling State

void BossIdlingState::Enter(Boss* enemy) {
    elapsedTime = 0.0f;
    idleDuration = RollDuration(
        enemy->GetIdleMinimumMilliseconds(),
        enemy->GetIdleMaximumMilliseconds()
    );
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
}

void BossIdlingState::Update(Boss* enemy, float deltaTime) {
    elapsedTime += std::max(0.0f, deltaTime);
    if (elapsedTime >= idleDuration) {
        IEnemyState* nextState = enemy->GetChaseState();
        switch (RollBossOffense()) {
            case BossOffense::Spell:
                nextState = enemy->GetSpellingState();
                break;
            case BossOffense::Punch:
                nextState = enemy->GetPunchState();
                break;
            case BossOffense::Stomp:
                nextState = enemy->GetStompingState();
                break;
            case BossOffense::Chase:
            default:
                break;
        }
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
    
    // Contact is an attack overlap only; enemies do not physically separate
    // from or collide with the player.
    Paladin* activePaladin = enemy->GetTargetTeam()->GetActivePaladin();
    if (EnemyCollision::CheckPlayerAttackOverlap(*enemy, *activePaladin)) {
        // Attack if cooldown allows
        if (enemy->GetAttackCooldown() <= 0.0f) {
            activePaladin->TakeDamage(enemy->GetDamage());
            enemy->ResetAttackCooldown();
        }
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
    nextSummonCheck = enemy->GetSpellSummonInterval();
    demonsSummoned = 0;
    enemy->EndPathFinding();
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
    enemy->ResetAnimationCycle();
}

void BossSpellingState::Update(Boss* enemy, float deltaTime) {
    elapsedTime += std::max(0.0f, deltaTime);

    while (nextSummonCheck <= spellDuration &&
           elapsedTime >= nextSummonCheck) {
        if (GetRandomValue(1, 100) <=
            enemy->GetSpellSummonChancePercent()) {
            enemy->TrySummonRandomEnemy(demonsSummoned);
        }
        nextSummonCheck += enemy->GetSpellSummonInterval();
    }

    if (elapsedTime >= spellDuration) {
        enemy->ChangeState(enemy->GetIdlingState());
    }
}

void BossSpellingState::Exit(Boss* enemy) {
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
}

// Boss Punch State (animation test only; no hitbox or damage yet)

void BossPunchState::Enter(Boss* enemy) {
    phase = Phase::Ready;
    frameTimer = 0.0f;
    frameIndex = 0;
    completedPunches = 0;
    punchesForState = enemy->GetPunchesPerState();
    enemy->EndPathFinding();
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
}

void BossPunchState::Update(Boss* enemy, float deltaTime) {
    frameTimer += std::max(0.0f, deltaTime);

    while (frameTimer >= BOSS_PUNCH_FRAME_DURATION) {
        frameTimer -= BOSS_PUNCH_FRAME_DURATION;
        ++frameIndex;

        if (phase == Phase::Ready) {
            if (frameIndex >= BOSS_PUNCH_READY_FRAME_COUNT) {
                phase = Phase::Punch;
                frameIndex = 0;
            }
            continue;
        }

        if (frameIndex == BOSS_PUNCH_FIRE_FRAME_INDEX) {
            enemy->FirePunchProjectile(
                bulletSpeed,
                changeAngleDegreesPerSecond
            );
        }

        if (frameIndex >= BOSS_PUNCH_PLAY_FRAME_COUNT) {
            frameIndex = 0;
            ++completedPunches;
            if (completedPunches >= punchesForState) {
                enemy->ChangeState(enemy->GetIdlingState());
                return;
            }
        }
    }
}

void BossPunchState::Exit(Boss* enemy) {
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
    enemy->ResetAnimationCycle();
}

// Boss Stomping State

void BossStompingState::Enter(Boss* enemy) {
    frameTimer = 0.0f;
    frameIndex = 0;
    completedStomps = 0;
    stompsForState = enemy->GetStompsPerState();
    enemy->EndPathFinding();
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
    enemy->ResetAnimationCycle();
}

void BossStompingState::Update(Boss* enemy, float deltaTime) {
    frameTimer += std::max(0.0f, deltaTime);

    while (frameTimer >= GetStompFrameDuration(frameIndex)) {
        frameTimer -= GetStompFrameDuration(frameIndex);
        ++frameIndex;

        if (frameIndex == BOSS_STOMP_FRAME_COUNT - 1) {
            enemy->SpawnStompSmoke();
            enemy->FireStompProjectiles();
        }

        if (frameIndex >= BOSS_STOMP_FRAME_COUNT) {
            frameIndex = 0;
            ++completedStomps;
            if (completedStomps >= stompsForState) {
                enemy->ChangeState(enemy->GetIdlingState());
                return;
            }
        }
    }
}

void BossStompingState::Exit(Boss* enemy) {
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
    enemy->ResetAnimationCycle();
}
