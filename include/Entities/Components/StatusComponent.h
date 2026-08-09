#pragma once
#include "raylib.h"
#include <vector>

class Enemy;

enum class EffectType {
    NONE,
    BURN,
    FREEZE,
    DIZZY
};

struct StatusModifier {
    EffectType type;
    float duration;
    float tickTimer;
    float magnitude;
};

class StatusComponent {
private:
    std::vector<StatusModifier> activeModifiers;

public:
    StatusComponent() = default;
    
    void AddEffect(EffectType type, float duration, float magnitude = 1.0f);
    
    // Returns true if the enemy is frozen (to allow early return in enemy Update)
    bool Update(float deltaTime, Enemy* owner);
    
    Color GetStatusTint() const;
    void Clear();
};
