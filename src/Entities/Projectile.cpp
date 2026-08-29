#include "Entities/Projectile.h"
#include "Core/Manager/ParticleManager.h"
#include "Entities/Player/Paladin.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
void ValidateProjectileInputs(
    Vector2 velocity,
    float lifetime,
    int damage,
    float radius
) {
    if (!std::isfinite(velocity.x) || !std::isfinite(velocity.y) ||
        !std::isfinite(lifetime) || lifetime <= 0.0f ||
        damage < 0 || !std::isfinite(radius) || radius < 0.0f) {
        throw std::invalid_argument("Projectile constructed with invalid values");
    }
}
}

Projectile::Projectile(
    Vector2 pos,
    Vector2 vel,
    float life,
    int dmg,
    bool isEnemy,
    float radius
)
    : GameObject(pos, GameObjectType::Projectile),
      velocity(vel), lifetime(life), collisionRadius(std::max(0.0f, radius)),
      active(true), damage(dmg), isEnemyProj(isEnemy) {
    ValidateProjectileInputs(vel, life, dmg, radius);
    texture.id = 0;
    boundingBox = {
        pos.x - collisionRadius,
        pos.y - collisionRadius,
        collisionRadius * 2.0f,
        collisionRadius * 2.0f
    };
}

Projectile::Projectile(
    Vector2 pos,
    Vector2 vel,
    float life,
    int dmg,
    Texture2D tex,
    bool isEnemy,
    float radius
)
    : GameObject(pos, GameObjectType::Projectile),
      velocity(vel), lifetime(life), collisionRadius(std::max(0.0f, radius)),
      active(true), damage(dmg), isEnemyProj(isEnemy), texture(tex) {
    ValidateProjectileInputs(vel, life, dmg, radius);
    boundingBox = {
        pos.x - collisionRadius,
        pos.y - collisionRadius,
        collisionRadius * 2.0f,
        collisionRadius * 2.0f
    };
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
            speed = std::max(speed, 1200.0f); // Fast, responsive return velocity
            velocity.x = (toOwner.x / dist) * speed;
            velocity.y = (toOwner.y / dist) * speed;
        }
    }

    // Move projectile
    Vector2 oldPos = position;
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;

    // Sweep Collision Bounding Box
    float minX = std::min(oldPos.x, position.x) - collisionRadius;
    float maxX = std::max(oldPos.x, position.x) + collisionRadius;
    float minY = std::min(oldPos.y, position.y) - collisionRadius;
    float maxY = std::max(oldPos.y, position.y) + collisionRadius;
    boundingBox = { minX, minY, maxX - minX, maxY - minY };

    // Reduce lifetime
    lifetime -= deltaTime;
    if (lifetime <= 0.0f) {
        active = false;
    }
}

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
    if (!target) return false;
    return std::find(
        hitTargets.begin(),
        hitTargets.end(),
        target->GetObjectId()
    ) !=
        hitTargets.end();
}

void Projectile::SetVelocity(Vector2 value) {
    if (!std::isfinite(value.x) || !std::isfinite(value.y)) {
        throw std::invalid_argument("Projectile velocity must be finite");
    }
    velocity = value;
}

void Projectile::RecordHit(GameObject* target) {
    if (target && !HasHitTarget(target)) {
        hitTargets.push_back(target->GetObjectId());
    }
}
