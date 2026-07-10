#include "Entities/Projectile.h"

Projectile::Projectile(Vector2 pos, Vector2 vel, float life, int dmg, bool isEnemy)
    : GameObject(pos), velocity(vel), lifetime(life), active(true), damage(dmg), isEnemyProj(isEnemy) {
    // Small bounding box for the projectile
    boundingBox = { pos.x, pos.y, 10.0f, 10.0f };
}

void Projectile::Update(float deltaTime) {
    if (!active) return;

    // Move projectile
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;

    // Update bounding box position
    boundingBox.x = position.x;
    boundingBox.y = position.y;

    // Reduce lifetime
    lifetime -= deltaTime;
    if (lifetime <= 0.0f) {
        active = false;
    }
}

void Projectile::Draw() {
    if (active) {
        // Draw a small BLUE rectangle as the projectile
        DrawRectangleRec(boundingBox, BLUE);
    }
}
