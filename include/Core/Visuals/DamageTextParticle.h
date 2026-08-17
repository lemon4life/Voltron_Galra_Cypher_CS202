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
    DamageTextParticle(Vector2 pos, Vector2 vel, int dmg, float life);
    
    void Update(float deltaTime) override;
    void Draw() const override;
    bool IsDead() const override;
};
