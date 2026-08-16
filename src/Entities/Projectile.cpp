#include "Entities/Projectile.h"
#include "Core/Manager/ParticleManager.h"
#include "Entities/Player/Paladin.h"
#include "raymath.h"
#include <cmath>

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

    if (maxFlyTime > 0.0f && !isReturning) {
        flightTimer += deltaTime;
        if (flightTimer >= maxFlyTime) {
            isReturning = true;
        }
    }

    if (isReturning && owner != nullptr) {
        Vector2 ownerPos = owner->GetPosition();
        Vector2 toOwner = { ownerPos.x - position.x, ownerPos.y - position.y };
        float dist = std::sqrt(toOwner.x * toOwner.x + toOwner.y * toOwner.y);
        
        // Immediate catch
        if (dist < 20.0f) { 
            active = false;
        } else {
            float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
            velocity.x = (toOwner.x / dist) * speed;
            velocity.y = (toOwner.y / dist) * speed;
        }
    }

    // Move projectile
    Vector2 oldPos = position;
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;

    // Sweep Collision Bounding Box
    float minX = std::min(oldPos.x, position.x);
    float maxX = std::max(oldPos.x + 10.0f, position.x + 10.0f);
    float minY = std::min(oldPos.y, position.y);
    float maxY = std::max(oldPos.y + 10.0f, position.y + 10.0f);
    boundingBox = { minX, minY, maxX - minX, maxY - minY };

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
            float rot = fixedRotation ? rotationAngle : atan2(velocity.y, velocity.x) * (180.0f / PI);
            Rectangle source = { 0, 0, (float)texture.width, (float)texture.height };
            Rectangle dest = { position.x, position.y, (float)texture.width, (float)texture.height };
            Vector2 origin = { (float)texture.width / 2.0f, (float)texture.height / 2.0f };
            DrawTexturePro(texture, source, dest, origin, rot, tint);
        } else {
            DrawRectangleRec(boundingBox, tint.a == 0 && tint.r == 0 ? BLUE : tint); // fallback
        }
    }
}

bool Projectile::HasHitTarget(GameObject* target) const {
    return hitTargets.find(target) != hitTargets.end();
}

void Projectile::RecordHit(GameObject* target) {
    hitTargets.insert(target);
}
