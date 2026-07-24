#include "Entities/Projectile.h"

Projectile::Projectile(Vector2 pos, Vector2 vel, float life, int dmg, bool isEnemy)
    : GameObject(pos, GameObjectType::Projectile),
      velocity(vel), lifetime(life), active(true), damage(dmg), isEnemyProj(isEnemy) {
    texture.id = 0;
    boundingBox = { pos.x, pos.y, 10.0f, 10.0f };
}

Projectile::Projectile(Vector2 pos, Vector2 vel, float life, int dmg, Texture2D tex, bool isEnemy)
    : GameObject(pos, GameObjectType::Projectile),
      velocity(vel), lifetime(life), active(true), damage(dmg), isEnemyProj(isEnemy), texture(tex) {
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

#include <cmath>
void Projectile::Draw() {
    if (active) {
        if (texture.id != 0) {
            float rot = atan2(velocity.y, velocity.x) * (180.0f / PI);
            Rectangle source = { 0, 0, (float)texture.width, (float)texture.height };
            Rectangle dest = { position.x, position.y, (float)texture.width, (float)texture.height };
            Vector2 origin = { (float)texture.width / 2.0f, (float)texture.height / 2.0f };
            DrawTexturePro(texture, source, dest, origin, rot, WHITE);
        } else {
            DrawRectangleRec(boundingBox, BLUE);
        }
    }
}
