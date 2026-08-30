#pragma once
#include "raylib.h"

#include "raymath.h"

class Paladin;

// Design Pattern - Strategy:
// Context: Paladin (currentWeapon). Strategy interface: IAttackStrategy.
// Concrete strategies: MeleeAttackStrategy, RangedAttackStrategy, and
// LaserAttackStrategy. A Paladin delegates attack/update/draw to its weapon.
class IAttackStrategy {
protected:
    Vector2 aimDir;
    float aimAngle;
    Paladin* owner = nullptr;

public:
    /// Releases resources owned by this IAttackStrategy instance.
    virtual ~IAttackStrategy() = default;

    /// Updates the stored owner.
    virtual void SetOwner(Paladin* p) { owner = p; }

    /// Updates the stored aim.
    virtual void SetAim(Vector2 dir, float angle) {
        aimDir = dir;
        aimAngle = angle;
    }
    
    // Attack method triggered by the player
    /// Starts this attack behavior when its current conditions allow it.
    virtual void Attack(Vector2 playerPos) = 0;
    
    // Allows the strategy to update its own timers/projectiles if needed
    /// Advances this component's state for the current frame.
    virtual void Update(float deltaTime) = 0;
    
    // Allows the strategy to draw its own debug hitboxes/effects
    /// Renders this component using its current state and visual resources.
    virtual void Draw(Vector2 playerPos, bool facingLeft) = 0;

    // Dynamic stat modification
    /// Updates the stored damage.
    virtual void SetDamage(int minDmg, int maxDmg) {}
    /// Updates the stored damage.
    virtual void SetDamage(int dmg) { SetDamage(dmg, dmg); }
    /// Updates the stored attack speed scalar.
    virtual void SetAttackSpeedScalar(float scalar) {}
};
