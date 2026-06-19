#pragma once
#include "raylib.h"

#include "raymath.h"

class IAttackStrategy {
protected:
    Vector2 aimDir;
    float aimAngle;

public:
    virtual ~IAttackStrategy() = default;

    virtual void SetAim(Vector2 dir, float angle) {
        aimDir = dir;
        aimAngle = angle;
    }
    
    // Attack method triggered by the player
    virtual void Attack(Vector2 playerPos) = 0;
    
    // Allows the strategy to update its own timers/projectiles if needed
    virtual void Update(float deltaTime) = 0;
    
    // Allows the strategy to draw its own debug hitboxes/effects
    virtual void Draw(Vector2 playerPos, bool facingLeft) = 0;
};
