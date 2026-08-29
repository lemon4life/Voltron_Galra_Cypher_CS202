#pragma once
#include "Entities/GameObject.h"
#include "Core/World/ObjectId.h"
#include <vector>

class Projectile : public GameObject {
protected:
    Vector2 velocity;
    float lifetime;
    float collisionRadius;
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
    std::vector<ObjectId> hitTargets;
    std::vector<MapObjectHandle> hitMapObjects;

public:
    Projectile(
        Vector2 pos,
        Vector2 vel,
        float life,
        int dmg,
        bool isEnemy = false,
        float radius = 5.0f
    );
    Projectile(
        Vector2 pos,
        Vector2 vel,
        float life,
        int dmg,
        Texture2D tex,
        bool isEnemy = false,
        float radius = 5.0f
    );
    
    void Update(float deltaTime) override;
    void Draw() override;
    
    bool IsActive() const { return active; }
    void Destroy() { active = false; }
    int GetDamage() const { return damage; }
    bool IsEnemyProjectile() const { return isEnemyProj; }
    Vector2 GetVelocity() const { return velocity; }
    virtual bool IgnoresWorldCollision() const { return false; }
    
    // Boomerang methods
    void SetReturning(bool returning) { isReturning = returning; }
    bool IsReturning() const { return isReturning; }
    void SetPiercing(bool piercing) { isPiercing = piercing; }
    bool IsPiercing() const { return isPiercing; }
    void SetOwner(GameObject* o) { owner = o; }
    GameObject* GetOwner() const { return owner; }
    void SetMaxFlyTime(float time) { maxFlyTime = time; }
    float GetMaxFlyTime() const { return maxFlyTime; }
    float GetFlightTimer() const { return flightTimer; }
    void SetVelocity(Vector2 value);
    void SetFixedRotation(bool fixed, float rot) { fixedRotation = fixed; rotationAngle = rot; }
    bool HasFixedRotation() const { return fixedRotation; }
    float GetRotationAngle() const { return rotationAngle; }
    void SetTint(Color c) { tint = c; }
    
    // Hit tracking
    bool HasHitTarget(GameObject* target) const;
    void RecordHit(GameObject* target);
    bool HasHitMapObject(MapObjectHandle handle) const {
        for (MapObjectHandle recorded : hitMapObjects) {
            if (recorded == handle) return true;
        }
        return false;
    }
    void RecordHitMapObject(MapObjectHandle handle) {
        if (handle != INVALID_MAP_OBJECT_HANDLE &&
            !HasHitMapObject(handle)) {
            hitMapObjects.push_back(handle);
        }
    }
};
