#include "Entities/Projectiles/DroneBullet.h"
#include <algorithm>

DroneBullet::DroneBullet(
    Vector2 startPos,
    Vector2 targetDir,
    float initialSpeed,
    float targetMinSpeed,
    float dragCoefficient,
    float lifetime,
    float collisionRadius,
    int damage,
    Texture2D tex,
    bool isEnemyProjectile
)
    : Projectile(
          startPos,
          { 0.0f, 0.0f },
          lifetime,
          damage,
          tex,
          isEnemyProjectile,
          collisionRadius
      ),
      currentSpeed(initialSpeed),
      minSpeed(targetMinSpeed),
      drag(dragCoefficient) {
          
    float length = Vector2Length(targetDir);
    if (length > 0) {
        direction = Vector2Normalize(targetDir);
    } else {
        direction = {1.0f, 0.0f}; // fallback
    }
    
    // Set initial velocity
    SetVelocity({direction.x * currentSpeed, direction.y * currentSpeed});
}

void DroneBullet::Update(float deltaTime) {
    if (currentSpeed > minSpeed) {
        currentSpeed = std::max(minSpeed, currentSpeed - (drag * deltaTime));
        SetVelocity({direction.x * currentSpeed, direction.y * currentSpeed});
    }
    
    Projectile::Update(deltaTime);
}
