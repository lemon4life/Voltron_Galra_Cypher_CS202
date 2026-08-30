#pragma once

#include "Entities/Projectile.h"
#include "raymath.h"

class DroneBullet : public Projectile {
private:
    float currentSpeed;
    float minSpeed;
    float drag;
    Vector2 direction;

public:
    /// Creates a DroneBullet instance from the supplied configuration.
    DroneBullet(
        Vector2 startPos,
        Vector2 targetDir,
        float initialSpeed,
        float targetMinSpeed,
        float dragCoefficient,
        float lifetime,
        float collisionRadius,
        int damage,
        Texture2D tex,
        bool isEnemyProjectile = true
    );
    
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
};
