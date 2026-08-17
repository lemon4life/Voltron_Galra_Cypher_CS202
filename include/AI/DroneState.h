#pragma once

#include "AI/EnemyState.h"
#include "raymath.h"

class Drone;

class DroneState : public IEnemyState {
private:
    float hoverTimer;
    Vector2 hoverTarget;
    bool FindNewHoverTarget(Drone* drone);

public:
    DroneState();
    
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;
};
