#include "Combat/RangedAttackStrategy.h"
#include "Entities/Projectile.h"
#include "Core/GameManager.h"

void RangedAttackStrategy::Attack(Vector2 playerPos, bool facingLeft) {
    float speed = 400.0f;
    Vector2 velocity = { facingLeft ? -speed : speed, 0.0f };
    
    // playerPos is the CENTER of the 36x48 sprite.
    // 24px from the bottom is exactly the middle of the sprite, which is playerPos.y.
    Vector2 spawnPos = {
        facingLeft ? playerPos.x - 18.0f : playerPos.x + 18.0f, // 18px is the edge of the 36px wide player
        playerPos.y 
    };

    Projectile* p = new Projectile(spawnPos, velocity, 2.0f);
    GameManager::GetInstance().AddProjectile(p);
}

void RangedAttackStrategy::Update(float deltaTime) {
    // Projectiles are managed by GameManager
}

void RangedAttackStrategy::Draw() {
    // Projectiles are managed by GameManager
}
