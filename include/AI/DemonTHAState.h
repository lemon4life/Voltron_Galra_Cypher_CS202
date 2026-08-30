#pragma once

#include "AI/IEnemyState.h"
#include "raylib.h"

#include <cstddef>
#include <vector>

class DemonTHA;

class DemonTHAWanderIdleState : public ITypedEnemyState<DemonTHA> {
private:
    float timeRemaining = 0.0f;

public:
    /// Prepares this state when it becomes active.
    void Enter(DemonTHA* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(DemonTHA* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(DemonTHA* enemy) override;
};

class DemonTHAWanderMoveState : public ITypedEnemyState<DemonTHA> {
private:
    std::vector<Vector2> candidates;
    std::size_t nextCandidateIndex = 0;

    /// Builds candidates.
    void BuildCandidates(DemonTHA* enemy);
    /// Tries candidate destinations until pathfinding returns a reachable route.
    bool RequestNextPath(DemonTHA* enemy);

public:
    /// Prepares this state when it becomes active.
    void Enter(DemonTHA* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(DemonTHA* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(DemonTHA* enemy) override;
};

class DemonTHAAggroState : public ITypedEnemyState<DemonTHA> {
private:
    enum class Phase {
        Fire,
        Recovery
    };

    Phase phase = Phase::Fire;
    float timeRemaining = 0.0f;
    int completedCycles = 0;

    /// Begins fire.
    void BeginFire(DemonTHA* enemy);
    /// Begins recovery.
    void BeginRecovery(DemonTHA* enemy);

public:
    /// Prepares this state when it becomes active.
    void Enter(DemonTHA* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(DemonTHA* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(DemonTHA* enemy) override;
};

