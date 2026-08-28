#include "AI/EnemyState.h"

#include "AI/EnemyCollision.h"
#include "Core/LevelAccess.h"
#include "Core/Manager/AudioManager.h"

#include "Entities/EnemyEntities/Boss.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/TeamManager.h"

#include "raymath.h"

#include <algorithm>
#include <array>

namespace {
    constexpr int BOSS_MOVE_MIN_MILLISECONDS = 2000;
    constexpr int BOSS_MOVE_MAX_MILLISECONDS = 3000;
    constexpr float BOSS_FINAL_IDLE_DURATION = 1.0f;
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
        Spell,
        Punch,
        Stomp
    };

    struct BossOffenseChoice {
        BossOffense offense;
        int probabilityPercent;
    };

    // The percentage of Boss entering each offense state after the idle state
    constexpr std::array<BossOffenseChoice, 3> BOSS_OFFENSE_CHOICES = {{
        { BossOffense::Spell, 34 },
        { BossOffense::Punch, 33 },
        { BossOffense::Stomp, 33 }
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

        return BossOffense::Stomp;
    }
}

// Boss Idling State

void BossIdlingState::Enter(Boss* enemy) {
    stage = Stage::InitialIdle;
    stageTimeRemaining = RollDuration(
        enemy->GetIdleMinimumMilliseconds(),
        enemy->GetIdleMaximumMilliseconds()
    );
    enemy->EndPathFinding();
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
}

void BossIdlingState::Update(Boss* enemy, float deltaTime) {
    float safeDeltaTime = std::max(0.0f, deltaTime);
    stageTimeRemaining -= safeDeltaTime;

    if (stage == Stage::InitialIdle) {
        enemy->SetCurrentVelocity({ 0.0f, 0.0f });
        if (stageTimeRemaining <= 0.0f) {
            stage = Stage::MoveToPlayer;
            stageTimeRemaining = RollDuration(
                BOSS_MOVE_MIN_MILLISECONDS,
                BOSS_MOVE_MAX_MILLISECONDS
            );
            enemy->StartPathFinding();
        }
        return;
    }

    if (stage == Stage::MoveToPlayer) {
        IEnemyPathAccess& pathAccess = enemy->GetPathAccess();
        std::optional<Vector2> moveTarget =
            pathAccess.GetNextMoveTarget(*enemy);
        Vector2 direction = { 0.0f, 0.0f };
        if (moveTarget) {
            direction = Vector2Subtract(
                *moveTarget,
                enemy->GetPosition()
            );
            if (Vector2Length(direction) > 0.0f) {
                direction = Vector2Normalize(direction);
            }
            direction = pathAccess.GetLocalDirection(*enemy, direction);
        }

        Vector2 velocity = Vector2Scale(
            direction,
            enemy->GetSpeed() * enemy->GetIdleMovementSpeedScale()
        );
        enemy->SetCurrentVelocity(velocity);
        EnemyCollision::MoveAgainstWalls(
            *enemy,
            Vector2Scale(velocity, safeDeltaTime),
            pathAccess,
            EnemyWallResponse::Slide
        );

        if (enemy->GetAttackCooldown() > 0.0f) {
            enemy->SetAttackCooldown(std::max(
                0.0f,
                enemy->GetAttackCooldown() - safeDeltaTime
            ));
        }

        TeamManager* targetTeam = enemy->GetTargetTeam();
        Paladin* activePaladin = targetTeam
            ? targetTeam->GetActivePaladin()
            : nullptr;
        if (activePaladin &&
            EnemyCollision::CheckPlayerAttackOverlap(
                *enemy,
                *activePaladin
            ) && enemy->GetAttackCooldown() <= 0.0f) {
            activePaladin->TakeDamage(enemy->GetDamage());
            enemy->ResetAttackCooldown();
        }

        if (stageTimeRemaining <= 0.0f) {
            stage = Stage::FinalIdle;
            stageTimeRemaining = BOSS_FINAL_IDLE_DURATION;
            enemy->EndPathFinding();
            enemy->SetCurrentVelocity({ 0.0f, 0.0f });
        }
        return;
    }

    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
    if (stageTimeRemaining > 0.0f) return;

    IEnemyState* nextState = enemy->GetSpellingState();
    switch (RollBossOffense()) {
        case BossOffense::Punch:
            nextState = enemy->GetPunchState();
            break;
        case BossOffense::Stomp:
            nextState = enemy->GetStompingState();
            break;
        case BossOffense::Spell:
        default:
            break;
    }
    enemy->ChangeState(nextState);
}

void BossIdlingState::Exit(Boss* enemy) {
    enemy->EndPathFinding();
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
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

        if (frameIndex == BOSS_PUNCH_PLAY_FRAME_COUNT - 1) {
            AudioManager::GetInstance().PlaySoundEffect(
                "boss_fire_punch"
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
            AudioManager::GetInstance().PlaySoundEffect("boss_stomping");
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
