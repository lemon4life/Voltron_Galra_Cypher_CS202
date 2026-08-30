#pragma once
#include "Core/Visuals/IParticle.h"
#include "raylib.h"
#include <string>

class DamageTextParticle : public IParticle {
private:
    Vector2 position;
    Vector2 velocity;
    int damage;
    float lifetime;
    float maxLifetime;
    std::string text;

public:
    /// Creates a DamageTextParticle instance from the supplied configuration.
    DamageTextParticle(Vector2 pos, Vector2 vel, int dmg, float life);
    
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() const override;
    /// Reports whether the dead condition is satisfied.
    bool IsDead() const override;
};
