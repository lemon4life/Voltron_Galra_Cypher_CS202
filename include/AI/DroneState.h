#pragma once

#include "AI/EnemyState.h"
#include "raymath.h"

#include <cstddef>
#include <vector>

class Drone;

class DroneMovingState : public ITypedEnemyState<Drone> {
private:
    std::vector<Vector2> patrolCandidates;
    std::size_t nextCandidateIndex = 0;
    bool trackingPlayerForFollowUp = false;
    float followUpTrackingTime = 0.0f;

    /// Builds patrol candidates.
    void BuildPatrolCandidates(Drone* drone);
    /// Tries candidate destinations until pathfinding returns a reachable route.
    bool RequestNextPath(Drone* drone);

public:
    /// Prepares this state when it becomes active.
    void Enter(Drone* drone) override;
    /// Advances this component's state for the current frame.
    void Update(Drone* drone, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(Drone* drone) override;
};

class DroneIdleState : public ITypedEnemyState<Drone> {
private:
    float idleTimeRemaining = 0.0f;

public:
    /// Prepares this state when it becomes active.
    void Enter(Drone* drone) override;
    /// Advances this component's state for the current frame.
    void Update(Drone* drone, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(Drone* drone) override;
};
