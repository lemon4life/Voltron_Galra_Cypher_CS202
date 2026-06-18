#pragma once
#include "IEnemyState.h"

class EnemyIdleState : public IEnemyState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;
};

class EnemyChaseState : public IEnemyState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;
};
