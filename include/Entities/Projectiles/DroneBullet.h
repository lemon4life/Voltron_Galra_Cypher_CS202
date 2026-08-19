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
    
    void Update(float deltaTime) override;
};
