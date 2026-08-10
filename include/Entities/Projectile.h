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
    Color tint = WHITE;
    
    // Boomerang mechanics
    bool isReturning = false;
    bool isPiercing = false;
    bool fixedRotation = false;
    float rotationAngle = 0.0f;
    GameObject* owner = nullptr;
    float maxFlyTime = 0.0f;
    float flightTimer = 0.0f;

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
    
    // Boomerang methods
    void SetReturning(bool returning) { isReturning = returning; }
    bool IsReturning() const { return isReturning; }
    void SetPiercing(bool piercing) { isPiercing = piercing; }
    bool IsPiercing() const { return isPiercing; }
    void SetOwner(GameObject* o) { owner = o; }
    GameObject* GetOwner() const { return owner; }
    void SetMaxFlyTime(float time) { maxFlyTime = time; }
    void SetVelocity(Vector2 v) { velocity = v; }
    void SetFixedRotation(bool fixed, float rot) { fixedRotation = fixed; rotationAngle = rot; }
    bool HasFixedRotation() const { return fixedRotation; }
    float GetRotationAngle() const { return rotationAngle; }
    void SetTint(Color c) { tint = c; }
};
