#pragma once
#include "IEnemyState.h"


/* 
    A simple default enemy behavior 
*/

class EnemyIdleState : public IEnemyState {
private:
    float spotDistance = 500.f;
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    void UpdateDistance(float nsd) override { spotDistance = nsd; };
};

class EnemyChaseState : public IEnemyState {
private:
    float offSightDistance = 1000.f;
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    void UpdateDistance(float nsd) override { offSightDistance = nsd; };
};

/* 
    A Path finding chase behavior for enemy "Chaser":
*/

class EnemyChaserChaseState : public IEnemyState {
private:
    float offSightDistance = 1000.f;
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    void UpdateDistance(float nsd) override { offSightDistance = nsd; };
};

/* 
    Boss Ranged Attack State
*/

class BossChaseState : public IEnemyState {
private:
    float offSightDistance = 1000.f;
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    void UpdateDistance(float nsd) override { offSightDistance = nsd; };
};

class BossRangedAttackState : public IEnemyState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;
    void UpdateDistance(float nsd) override {}
};
