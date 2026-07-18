#include "Combat/RangedAttackStrategy.h"
#include "Entities/Projectile.h"
#include "Core/Manager/GameManager.h"

RangedAttackStrategy::RangedAttackStrategy(Texture2D tex, Texture2D muzzleTex, Texture2D bullTex) 
    : weaponTex(tex), muzzleFlashTex(muzzleTex), bulletTex(bullTex), recoilOffset{0,0}, muzzleFlashTimer(0.0f) {
    aimDir = {1.0f, 0.0f};
    aimAngle = 0.0f;
}

#include "Core/Manager/AudioManager.h"

void RangedAttackStrategy::Attack(Vector2 playerPos) {
    Vector2 projVelocity = { aimDir.x * 400.0f, aimDir.y * 400.0f };
    // Create projectile originating at player center
    Projectile* p = new Projectile(playerPos, projVelocity, 2.0f, 34, bulletTex);
    
    recoilOffset = { -aimDir.x * 15.0f, -aimDir.y * 15.0f };
    muzzleFlashTimer = 0.05f;
    GameManager::GetInstance().AddProjectile(p);
    
    AudioManager::GetInstance().PlayRandomLaser();
}

void RangedAttackStrategy::Update(float deltaTime) {
    recoilOffset.x -= recoilOffset.x * 15.0f * deltaTime;
    recoilOffset.y -= recoilOffset.y * 15.0f * deltaTime;
    
    if (muzzleFlashTimer > 0.0f) {
        muzzleFlashTimer -= deltaTime;
    }
}

void RangedAttackStrategy::Draw(Vector2 playerPos, bool facingLeft) {
    Rectangle source = { 0.0f, 0.0f, (float)weaponTex.width, (float)weaponTex.height };
    // If aiming left, flip the gun vertically so it isn't upside down
    if (facingLeft) {
        source.height = -source.height; 
    }

    Vector2 drawPos = { playerPos.x + recoilOffset.x, playerPos.y + recoilOffset.y };
    Rectangle dest = { drawPos.x, drawPos.y, (float)weaponTex.width, (float)weaponTex.height };
    Vector2 origin = { 0.0f, (float)weaponTex.height / 2.0f };

    DrawTexturePro(weaponTex, source, dest, origin, aimAngle, WHITE);
    
    if (muzzleFlashTimer > 0.0f && muzzleFlashTex.id != 0) {
        float barrelLength = weaponTex.width; // rough estimate
        Vector2 tip = {
            drawPos.x + aimDir.x * barrelLength,
            drawPos.y + aimDir.y * barrelLength
        };

        Rectangle mfSource = { 0, 0, (float)muzzleFlashTex.width, (float)muzzleFlashTex.height };
        Rectangle mfDest = { tip.x, tip.y, (float)muzzleFlashTex.width, (float)muzzleFlashTex.height };
        Vector2 mfOrigin = { (float)muzzleFlashTex.width / 2.0f, (float)muzzleFlashTex.height / 2.0f };
        
        DrawTexturePro(muzzleFlashTex, mfSource, mfDest, mfOrigin, aimAngle, WHITE);
    }
}
