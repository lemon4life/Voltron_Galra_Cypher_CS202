#pragma once
#include "Entities/GameObject.h"

class Projectile : public GameObject {
private:
    Vector2 velocity;
    float lifetime;
    bool active;
    int damage;
    bool isEnemyProj;
    Texture2D texture;

public:
    Projectile(Vector2 pos, Vector2 vel, float life, int dmg, bool isEnemy = false);
    Projectile(Vector2 pos, Vector2 vel, float life, int dmg, Texture2D tex, bool isEnemy = false);
    
    void Update(float deltaTime) override;
    void Draw() override;
    
    bool IsActive() const { return active; }
    void Destroy() { active = false; }
    int GetDamage() const { return damage; }
    bool IsEnemyProjectile() const { return isEnemyProj; }
    Vector2 GetVelocity() const { return velocity; }
};
