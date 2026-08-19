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

    void BuildPatrolCandidates(Drone* drone);
    bool RequestNextPath(Drone* drone);

public:
    void Enter(Drone* drone) override;
    void Update(Drone* drone, float deltaTime) override;
    void Exit(Drone* drone) override;
};

class DroneIdleState : public ITypedEnemyState<Drone> {
private:
    float idleTimeRemaining = 0.0f;

public:
    void Enter(Drone* drone) override;
    void Update(Drone* drone, float deltaTime) override;
    void Exit(Drone* drone) override;
};
