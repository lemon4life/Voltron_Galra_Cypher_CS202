#pragma once
#include "raylib.h"
#include <vector>

class Enemy;

enum class EffectType {
    NONE,
    BURN,
    FREEZE,
    DIZZY,
    POISON,
    SLOW
};

struct StatusModifier {
    EffectType type;
    float duration;
    float tickTimer;
    float magnitude;
};

// Design Pattern - Component:
// Owner/context: Enemy. Component: StatusComponent. StatusModifier values are
// composed into an enemy to add burn/freeze/dizzy/poison/slow behavior without
// expanding the Enemy inheritance tree for every effect combination.
class StatusComponent {
private:
    std::vector<StatusModifier> activeModifiers;

public:
    /// Creates a StatusComponent instance from the supplied configuration.
    StatusComponent() = default;
    
    /// Adds effect.
    void AddEffect(EffectType type, float duration, float magnitude = 1.0f);
    /// Reports whether this component has effect.
    bool HasEffect(EffectType type) const;
    
    // Returns true if the enemy is frozen (to allow early return in enemy Update)
    /// Advances this component's state for the current frame.
    bool Update(float deltaTime, Enemy* owner);
    
    /// Returns the current status tint.
    Color GetStatusTint() const;
    /// Removes all runtime entries owned by this component and resets transient state.
    void Clear();
};
