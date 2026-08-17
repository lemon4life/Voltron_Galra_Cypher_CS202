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
    void AddDamage(int amount);
    void Update(float deltaTime);
    void Draw(Vector2 basePosition);
    
    int GetAccumulatedDamage() const { return accumulatedDamage; }
    void Reset();
};
