#pragma once
#include "IEnemyState.h"


/* 
    A simple default enemy behavior 
    
    - Idle: Do nothing
    - Spot player: If player absolute distance is near enough
    - Chase: Following player by a shortest direct path without 
    considering obstacles/walls
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
    A better enemy chase behavior for enemy "Chaser":

    - Chase: Apply path finding to get to player position
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
