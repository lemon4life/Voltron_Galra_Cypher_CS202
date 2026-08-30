#pragma once
#include "raylib.h"

class ComboMeter {
    enum class State { ACTIVE, LINGERING, SLIDING };
    State state = State::ACTIVE;

    int accumulatedDamage = 0;
    float comboTimer = 0.0f;
    float lingerTimer = 0.0f;
    float slideOffsetX = -300.0f;
    float popScale = 1.0f;

public:
    /// Adds damage.
    void AddDamage(int amount);
    /// Advances this component's state for the current frame.
    void Update(float deltaTime);
    /// Renders this component using its current state and visual resources.
    void Draw(Vector2 basePosition);
    
    /// Returns the current accumulated damage.
    int GetAccumulatedDamage() const { return accumulatedDamage; }
    /// Restores this component to its initial runtime state.
    void Reset();
};
