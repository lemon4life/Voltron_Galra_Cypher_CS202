#pragma once
#include "raylib.h"

#include "raymath.h"

class Paladin;

class IAttackStrategy {
protected:
    Vector2 aimDir;
    float aimAngle;
    Paladin* owner = nullptr;

public:
    virtual ~IAttackStrategy() = default;

    virtual void SetOwner(Paladin* p) { owner = p; }

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

    // Dynamic stat modification
    virtual void SetDamage(int minDmg, int maxDmg) {}
    virtual void SetDamage(int dmg) { SetDamage(dmg, dmg); }
    virtual void SetAttackSpeedScalar(float scalar) {}
};
