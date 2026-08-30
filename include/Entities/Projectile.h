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
    /// Creates a Projectile instance from the supplied configuration.
    Projectile(
        Vector2 pos,
        Vector2 vel,
        float life,
        int dmg,
        bool isEnemy = false,
        float radius = 5.0f
    );
    /// Creates a Projectile instance from the supplied configuration.
    Projectile(
        Vector2 pos,
        Vector2 vel,
        float life,
        int dmg,
        Texture2D tex,
        bool isEnemy = false,
        float radius = 5.0f
    );
    
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
    
    /// Reports whether the active condition is satisfied.
    bool IsActive() const { return active; }
    /// Implements the destroy behavior for this component.
    void Destroy() { active = false; }
    /// Returns the current damage.
    int GetDamage() const { return damage; }
    /// Reports whether the enemy projectile condition is satisfied.
    bool IsEnemyProjectile() const { return isEnemyProj; }
    /// Returns the current velocity.
    Vector2 GetVelocity() const { return velocity; }
    /// Reports whether this projectile intentionally passes through world blockers.
    virtual bool IgnoresWorldCollision() const { return false; }
    
    // Boomerang methods
    /// Updates the stored returning.
    void SetReturning(bool returning) { isReturning = returning; }
    /// Reports whether the returning condition is satisfied.
    bool IsReturning() const { return isReturning; }
    /// Updates the stored piercing.
    void SetPiercing(bool piercing) { isPiercing = piercing; }
    /// Reports whether the piercing condition is satisfied.
    bool IsPiercing() const { return isPiercing; }
    /// Updates the stored owner.
    void SetOwner(GameObject* o) { owner = o; }
    /// Returns the current owner.
    GameObject* GetOwner() const { return owner; }
    /// Updates the stored max fly time.
    void SetMaxFlyTime(float time) { maxFlyTime = time; }
    /// Returns the current max fly time.
    float GetMaxFlyTime() const { return maxFlyTime; }
    /// Returns the current flight timer.
    float GetFlightTimer() const { return flightTimer; }
    /// Updates the stored velocity.
    void SetVelocity(Vector2 value);
    /// Updates the stored fixed rotation.
    void SetFixedRotation(bool fixed, float rot) { fixedRotation = fixed; rotationAngle = rot; }
    /// Reports whether this component has fixed rotation.
    bool HasFixedRotation() const { return fixedRotation; }
    /// Returns the current rotation angle.
    float GetRotationAngle() const { return rotationAngle; }
    /// Updates the stored tint.
    void SetTint(Color c) { tint = c; }
    
    // Hit tracking
    /// Reports whether this component has hit target.
    bool HasHitTarget(GameObject* target) const;
    /// Records hit.
    void RecordHit(GameObject* target);
    /// Reports whether this component has hit map object.
    bool HasHitMapObject(MapObjectHandle handle) const {
        for (MapObjectHandle recorded : hitMapObjects) {
            if (recorded == handle) return true;
        }
        return false;
    }
    /// Records hit map object.
    void RecordHitMapObject(MapObjectHandle handle) {
        if (handle != INVALID_MAP_OBJECT_HANDLE &&
            !HasHitMapObject(handle)) {
            hitMapObjects.push_back(handle);
        }
    }
};
