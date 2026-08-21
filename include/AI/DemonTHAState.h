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
    void Enter(DemonTHA* enemy) override;
    void Update(DemonTHA* enemy, float deltaTime) override;
    void Exit(DemonTHA* enemy) override;
};

class DemonTHAWanderMoveState : public ITypedEnemyState<DemonTHA> {
private:
    std::vector<Vector2> candidates;
    std::size_t nextCandidateIndex = 0;

    void BuildCandidates(DemonTHA* enemy);
    bool RequestNextPath(DemonTHA* enemy);

public:
    void Enter(DemonTHA* enemy) override;
    void Update(DemonTHA* enemy, float deltaTime) override;
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

    void BeginFire(DemonTHA* enemy);
    void BeginRecovery(DemonTHA* enemy);

public:
    void Enter(DemonTHA* enemy) override;
    void Update(DemonTHA* enemy, float deltaTime) override;
    void Exit(DemonTHA* enemy) override;
};

