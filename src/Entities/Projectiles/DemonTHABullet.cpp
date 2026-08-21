#include "Entities/Projectiles/DemonTHABullet.h"

#include <algorithm>

namespace {
    constexpr float COLLISION_SIZE = 5.0f;
    constexpr float COLLISION_HALF_SIZE = COLLISION_SIZE * 0.5f;
}

DemonTHABullet::DemonTHABullet(
    Vector2 startPosition,
    Vector2 initialVelocity,
    float lifetime,
    int damage,
    Texture2D texture
)
    : Projectile(
          startPosition,
          initialVelocity,
          lifetime,
          damage,
          texture,
          true,
          0.0f
      ) {
    UpdateSweptCollisionBox(startPosition);
}

void DemonTHABullet::Update(float deltaTime) {
    Vector2 previousPosition = position;
    Projectile::Update(deltaTime);
    UpdateSweptCollisionBox(previousPosition);
}

void DemonTHABullet::UpdateSweptCollisionBox(
    Vector2 previousPosition
) {
    float minimumX = std::min(previousPosition.x, position.x) -
        COLLISION_HALF_SIZE;
    float maximumX = std::max(previousPosition.x, position.x) +
        COLLISION_HALF_SIZE;
    float minimumY = std::min(previousPosition.y, position.y) -
        COLLISION_HALF_SIZE;
    float maximumY = std::max(previousPosition.y, position.y) +
        COLLISION_HALF_SIZE;
    boundingBox = {
        minimumX,
        minimumY,
        maximumX - minimumX,
        maximumY - minimumY
    };
}
