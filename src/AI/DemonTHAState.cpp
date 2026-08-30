#include "AI/DemonTHAState.h"

#include "Core/LevelAccess.h"
#include "Entities/EnemyEntities/DemonTHA.h"
#include "Entities/Player/Paladin.h"

#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr int IDLE_MIN_MILLISECONDS = 2000;
    constexpr int IDLE_MAX_MILLISECONDS = 5000;
    constexpr int FIRE_MIN_MILLISECONDS = 2000;
    constexpr int FIRE_MAX_MILLISECONDS = 4000;
    constexpr int RECOVERY_MIN_MILLISECONDS = 1000;
    constexpr int RECOVERY_MAX_MILLISECONDS = 3000;
    constexpr int AGGRO_CYCLE_COUNT = 3;
    constexpr float MIN_DIRECTION_LENGTH = 0.001f;

    /// Returns a random real-time duration inside the supplied range.
    float RandomSeconds(int minimumMilliseconds, int maximumMilliseconds) {
        return (float)GetRandomValue(
            minimumMilliseconds,
            maximumMilliseconds
        ) / 1000.0f;
    }

    /// Randomizes candidate order so repeated searches do not favor the same destination.
    void ShuffleCandidates(std::vector<Vector2>& candidates) {
        for (std::size_t index = candidates.size(); index > 1; --index) {
            int swapIndex = GetRandomValue(0, (int)index - 1);
            std::swap(candidates[index - 1], candidates[swapIndex]);
        }
    }
}

/// Prepares this state when it becomes active.
void DemonTHAWanderIdleState::Enter(DemonTHA* enemy) {
    enemy->EndPathFinding();
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
    enemy->SetGunShooting(false);

    if (enemy->ShouldImmediatelyAggro() ||
        enemy->ShouldRollDistantAggroOnIdleEntry()) {
        enemy->ChangeState(enemy->GetAggroState());
        return;
    }

    timeRemaining = RandomSeconds(
        IDLE_MIN_MILLISECONDS,
        IDLE_MAX_MILLISECONDS
    );
}

/// Advances this component's state for the current frame.
void DemonTHAWanderIdleState::Update(
    DemonTHA* enemy,
    float deltaTime
) {
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
    if (enemy->ShouldImmediatelyAggro()) {
        enemy->ChangeState(enemy->GetAggroState());
        return;
    }

    timeRemaining -= std::max(0.0f, deltaTime);
    if (timeRemaining <= 0.0f) {
        enemy->ChangeState(enemy->GetWanderMoveState());
    }
}

/// Cleans up this state before control moves elsewhere.
void DemonTHAWanderIdleState::Exit(DemonTHA*) {
}

/// Builds candidates.
void DemonTHAWanderMoveState::BuildCandidates(DemonTHA* enemy) {
    std::vector<Vector2> allCandidates =
        enemy->GetPathAccess().GetNavigableTileCentersWithin(
            *enemy,
            enemy->GetPosition(),
            enemy->GetCurrentRoomCandidateRadius()
        );

    bool requireLineOfSight =
        enemy->ConsumeNextWanderGoalUsesLineOfSight();
    Paladin* target = enemy->GetActiveTarget();
    if (requireLineOfSight && target) {
        candidates.reserve(allCandidates.size());
        for (Vector2 candidate : allCandidates) {
            if (enemy->HasClearShotFrom(candidate, *target)) {
                candidates.push_back(candidate);
            }
        }
    }

    if (candidates.empty()) {
        candidates = std::move(allCandidates);
    }
    ShuffleCandidates(candidates);
    nextCandidateIndex = 0;
}

/// Tries candidate destinations until pathfinding returns a reachable route.
bool DemonTHAWanderMoveState::RequestNextPath(DemonTHA* enemy) {
    enemy->EndPathFinding();
    if (nextCandidateIndex >= candidates.size()) return false;

    enemy->StartPathFindingTo(candidates[nextCandidateIndex++]);
    return true;
}

/// Prepares this state when it becomes active.
void DemonTHAWanderMoveState::Enter(DemonTHA* enemy) {
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
    enemy->SetGunShooting(false);
    candidates.clear();
    BuildCandidates(enemy);
    if (!RequestNextPath(enemy)) {
        enemy->ChangeState(enemy->GetWanderIdleState());
    }
}

/// Advances this component's state for the current frame.
void DemonTHAWanderMoveState::Update(
    DemonTHA* enemy,
    float deltaTime
) {
    if (enemy->ShouldImmediatelyAggro()) {
        enemy->ChangeState(enemy->GetAggroState());
        return;
    }

    EnemyPathStatus status = enemy->GetPathStatus();
    if (status == EnemyPathStatus::Unreachable ||
        status == EnemyPathStatus::SearchLimitReached) {
        if (!RequestNextPath(enemy)) {
            enemy->ChangeState(enemy->GetWanderIdleState());
        }
        return;
    }
    if (status == EnemyPathStatus::AtGoal) {
        enemy->ChangeState(enemy->GetWanderIdleState());
        return;
    }

    std::optional<Vector2> moveTarget =
        enemy->GetPathAccess().GetNextMoveTarget(*enemy);
    if (!moveTarget) {
        enemy->SetCurrentVelocity({ 0.0f, 0.0f });
        return;
    }

    Vector2 direction = Vector2Subtract(
        *moveTarget,
        enemy->GetPosition()
    );
    if (Vector2Length(direction) <= MIN_DIRECTION_LENGTH) {
        enemy->SetCurrentVelocity({ 0.0f, 0.0f });
        return;
    }

    direction = Vector2Normalize(direction);
    direction = enemy->GetPathAccess().GetLocalDirection(*enemy, direction);
    if (Vector2Length(direction) <= MIN_DIRECTION_LENGTH) {
        enemy->SetCurrentVelocity({ 0.0f, 0.0f });
        return;
    }

    direction = Vector2Normalize(direction);
    if (std::fabs(direction.x) > MIN_DIRECTION_LENGTH) {
        enemy->SetFacingLeft(direction.x < 0.0f);
    }
    enemy->UpdateMovement(
        Vector2Scale(direction, enemy->GetSpeed()),
        deltaTime,
        EnemyWallResponse::Slide
    );
}

/// Cleans up this state before control moves elsewhere.
void DemonTHAWanderMoveState::Exit(DemonTHA* enemy) {
    enemy->EndPathFinding();
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
    candidates.clear();
    nextCandidateIndex = 0;
}

/// Begins fire.
void DemonTHAAggroState::BeginFire(DemonTHA* enemy) {
    phase = Phase::Fire;
    timeRemaining = RandomSeconds(
        FIRE_MIN_MILLISECONDS,
        FIRE_MAX_MILLISECONDS
    );
    enemy->SetGunShooting(true);
}

/// Begins recovery.
void DemonTHAAggroState::BeginRecovery(DemonTHA* enemy) {
    phase = Phase::Recovery;
    timeRemaining = RandomSeconds(
        RECOVERY_MIN_MILLISECONDS,
        RECOVERY_MAX_MILLISECONDS
    );
    enemy->SetGunShooting(false);
}

/// Prepares this state when it becomes active.
void DemonTHAAggroState::Enter(DemonTHA* enemy) {
    enemy->EndPathFinding();
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
    completedCycles = 0;
    BeginFire(enemy);
}

/// Advances this component's state for the current frame.
void DemonTHAAggroState::Update(DemonTHA* enemy, float deltaTime) {
    Paladin* target = enemy->GetActiveTarget();
    if (!target) {
        enemy->ChangeState(enemy->GetWanderIdleState());
        return;
    }

    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
    enemy->SetFacingLeft(
        target->GetPosition().x < enemy->GetPosition().x
    );
    timeRemaining -= std::max(0.0f, deltaTime);
    if (timeRemaining > 0.0f) return;

    if (phase == Phase::Fire) {
        BeginRecovery(enemy);
        return;
    }

    ++completedCycles;
    if (completedCycles >= AGGRO_CYCLE_COUNT) {
        enemy->ChangeState(enemy->GetWanderIdleState());
        return;
    }
    BeginFire(enemy);
}

/// Cleans up this state before control moves elsewhere.
void DemonTHAAggroState::Exit(DemonTHA* enemy) {
    enemy->SetGunShooting(false);
    enemy->SetCurrentVelocity({ 0.0f, 0.0f });
}
