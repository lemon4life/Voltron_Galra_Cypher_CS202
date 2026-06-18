#pragma once
#include "Entities/GameObject.h"

class Projectile : public GameObject {
private:
    Vector2 velocity;
    float lifetime;
    bool active;
    int damage;

public:
    Projectile(Vector2 pos, Vector2 vel, float life, int dmg);
    
    void Update(float deltaTime) override;
    void Draw() override;
    
    bool IsActive() const { return active; }
    void Destroy() { active = false; }
    int GetDamage() const { return damage; }
};
