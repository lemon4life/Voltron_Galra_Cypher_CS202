#pragma once
#include "raylib.h"

class IAttackStrategy {
public:
    virtual ~IAttackStrategy() = default;
    
    // Attack method triggered by the player
    virtual void Attack(Vector2 playerPos, bool facingLeft) = 0;
    
    // Allows the strategy to update its own timers/projectiles if needed
    virtual void Update(float deltaTime) = 0;
    
    // Allows the strategy to draw its own debug hitboxes/effects
    virtual void Draw() = 0;
};
