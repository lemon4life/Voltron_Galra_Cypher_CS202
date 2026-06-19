#include "Combat/RangedAttackStrategy.h"
#include "Entities/Projectile.h"
#include "Core/GameManager.h"

RangedAttackStrategy::RangedAttackStrategy(Texture2D tex) : weaponTex(tex) {
    aimDir = {1.0f, 0.0f};
    aimAngle = 0.0f;
}

void RangedAttackStrategy::Attack(Vector2 playerPos) {
    Vector2 projVelocity = { aimDir.x * 400.0f, aimDir.y * 400.0f };
    // Create projectile originating at player center
    Projectile* p = new Projectile(playerPos, projVelocity, 2.0f, 34);
    GameManager::GetInstance().AddProjectile(p);
}

void RangedAttackStrategy::Update(float deltaTime) {
    // Implementation not needed for basic projectiles (GameManager handles updates)
}

void RangedAttackStrategy::Draw(Vector2 playerPos, bool facingLeft) {
    Rectangle source = { 0.0f, 0.0f, (float)weaponTex.width, (float)weaponTex.height };
    // If aiming left, flip the gun vertically so it isn't upside down
    if (facingLeft) {
        source.height = -source.height; 
    }

    Rectangle dest = { playerPos.x, playerPos.y, (float)weaponTex.width, (float)weaponTex.height };
    // Origin at the base/hilt
    Vector2 origin = { 0.0f, (float)weaponTex.height / 2.0f };

    DrawTexturePro(weaponTex, source, dest, origin, aimAngle, WHITE);
}
