#include "AI/DroneState.h"

#include "Core/LevelAccess.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/EnemyEntities/Drone.h"
#include "Entities/Player/Paladin.h"

#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float PATROL_RADIUS = 150.0f;
    constexpr float NEAR_PLAYER_DISTANCE = 200.0f;
    constexpr float MOVING_FOLLOW_UP_DELAY = 1.0f;
    constexpr float IDLE_COOLDOWN_RATE = 2.0f;
    constexpr float MINIMUM_POST_ATTACK_IDLE_TIME = 0.5f;

    /// Randomizes candidate order so repeated searches do not favor the same destination.
    void ShuffleCandidates(std::vector<Vector2>& candidates) {
        for (std::size_t index = candidates.size(); index > 1; --index) {
            int swapIndex = GetRandomValue(0, (int)index - 1);
            std::swap(candidates[index - 1], candidates[swapIndex]);
        }
    }

    /// Returns the current active target.
    Paladin* GetActiveTarget(Drone* drone) {
        TeamManager* targetTeam = drone->GetTargetTeam();
        return targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    }

    /// Reports whether this component has player line of sight.
    bool HasPlayerLineOfSight(Drone* drone, const Paladin* player) {
        return player && drone->GetLineOfSightQuery().HasClearLineOfSight(
            drone->GetPosition(),
            player->GetPosition(),
            5.0f
        );
    }
}

/// Builds patrol candidates.
void DroneMovingState::BuildPatrolCandidates(Drone* drone) {
    patrolCandidates = drone->GetPathAccess().GetNavigableTileCentersWithin(
        *drone,
        drone->GetPosition(),
        PATROL_RADIUS
    );
    ShuffleCandidates(patrolCandidates);
    nextCandidateIndex = 0;
}

/// Tries candidate destinations until pathfinding returns a reachable route.
bool DroneMovingState::RequestNextPath(Drone* drone) {
    drone->EndPathFinding();
    if (nextCandidateIndex >= patrolCandidates.size()) {
        return false;
    }

    Vector2 target = patrolCandidates[nextCandidateIndex++];
    drone->StartPathFindingTo(target);
    return true;
}

/// Prepares this state when it becomes active.
void DroneMovingState::Enter(Drone* drone) {
    drone->SetCurrentVelocity({ 0.0f, 0.0f });
    trackingPlayerForFollowUp = false;
    followUpTrackingTime = 0.0f;
    BuildPatrolCandidates(drone);
    if (!RequestNextPath(drone)) {
        drone->ChangeState(drone->GetDroneIdleState());
    }
}

/// Advances this component's state for the current frame.
void DroneMovingState::Update(Drone* drone, float deltaTime) {
    drone->TickAttackCooldown(deltaTime);

    Paladin* player = GetActiveTarget(drone);
    bool hasPlayerLineOfSight = HasPlayerLineOfSight(drone, player);
    if (trackingPlayerForFollowUp) {
        drone->SetCurrentVelocity({ 0.0f, 0.0f });
        if (!hasPlayerLineOfSight) {
            trackingPlayerForFollowUp = false;
            followUpTrackingTime = 0.0f;
        } else {
            drone->SetFacingLeft(
                player->GetPosition().x < drone->GetPosition().x
            );
            followUpTrackingTime -= std::max(0.0f, deltaTime);
            if (followUpTrackingTime <= 0.0f) {
                trackingPlayerForFollowUp = false;
                followUpTrackingTime = 0.0f;
                if (drone->Attack() &&
                    Vector2Distance(
                        drone->GetPosition(),
                        player->GetPosition()
                    ) <= NEAR_PLAYER_DISTANCE) {
                    drone->ChangeState(drone->GetDroneIdleState());
                }
            }
            return;
        }
    }

    if (player && drone->GetAttackCooldown() <= 0.0f &&
        hasPlayerLineOfSight) {
        drone->SetCurrentVelocity({ 0.0f, 0.0f });
        drone->SetFacingLeft(
            player->GetPosition().x < drone->GetPosition().x
        );
        if (drone->Attack()) {
            trackingPlayerForFollowUp = true;
            followUpTrackingTime = MOVING_FOLLOW_UP_DELAY;
        }
        return;
    }

    EnemyPathStatus pathStatus = drone->GetPathStatus();
    if (pathStatus == EnemyPathStatus::Unreachable ||
        pathStatus == EnemyPathStatus::SearchLimitReached) {
        if (!RequestNextPath(drone)) {
            drone->ChangeState(drone->GetDroneIdleState());
        }
        return;
    }
    if (pathStatus == EnemyPathStatus::AtGoal) {
        drone->ChangeState(drone->GetDroneIdleState());
        return;
    }

    std::optional<Vector2> moveTarget =
        drone->GetPathAccess().GetNextMoveTarget(*drone);
    if (!moveTarget) {
        drone->SetCurrentVelocity({ 0.0f, 0.0f });
        if (drone->GetPathStatus() == EnemyPathStatus::AtGoal) {
            drone->ChangeState(drone->GetDroneIdleState());
        }
        return;
    }

    Vector2 direction = Vector2Subtract(
        *moveTarget,
        drone->GetPosition()
    );
    if (Vector2Length(direction) <= 0.001f) {
        drone->SetCurrentVelocity({ 0.0f, 0.0f });
        return;
    }

    direction = Vector2Normalize(direction);
    drone->SetFacingLeft(direction.x < 0.0f);
    drone->UpdateMovement(
        Vector2Scale(direction, drone->GetSpeed()),
        deltaTime,
        EnemyWallResponse::Slide
    );
}

/// Cleans up this state before control moves elsewhere.
void DroneMovingState::Exit(Drone* drone) {
    drone->SetCurrentVelocity({ 0.0f, 0.0f });
    drone->EndPathFinding();
    patrolCandidates.clear();
    nextCandidateIndex = 0;
    trackingPlayerForFollowUp = false;
    followUpTrackingTime = 0.0f;
}

/// Prepares this state when it becomes active.
void DroneIdleState::Enter(Drone* drone) {
    drone->EndPathFinding();
    drone->SetCurrentVelocity({ 0.0f, 0.0f });
    idleTimeRemaining = (float)GetRandomValue(300, 500) / 100.0f;
}

/// Advances this component's state for the current frame.
void DroneIdleState::Update(Drone* drone, float deltaTime) {
    drone->SetCurrentVelocity({ 0.0f, 0.0f });
    idleTimeRemaining -= std::max(0.0f, deltaTime);
    drone->TickAttackCooldown(deltaTime, IDLE_COOLDOWN_RATE);

    Paladin* player = GetActiveTarget(drone);
    if (HasPlayerLineOfSight(drone, player)) {
        drone->SetFacingLeft(
            player->GetPosition().x < drone->GetPosition().x
        );
        if (drone->GetAttackCooldown() <= 0.0f &&
            drone->Attack()) {
            idleTimeRemaining = std::max(
                idleTimeRemaining,
                MINIMUM_POST_ATTACK_IDLE_TIME
            );
        }
    }

    if (idleTimeRemaining <= 0.0f) {
        drone->ChangeState(drone->GetMovingState());
    }
}

/// Cleans up this state before control moves elsewhere.
void DroneIdleState::Exit(Drone* drone) {
    drone->SetCurrentVelocity({ 0.0f, 0.0f });
    idleTimeRemaining = 0.0f;
}
