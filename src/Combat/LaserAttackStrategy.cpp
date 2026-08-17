#include "Combat/LaserAttackStrategy.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/AudioManager.h"
#include "Entities/Enemy.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Props/Prop.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/ParticleManager.h"
#include <cmath>
#include <iostream>

LaserAttackStrategy::LaserAttackStrategy(
    Texture2D weapon,
    Texture2D muzzle,
    Texture2D beam,
    Texture2D impact,
    int damage,
    float recoilStrength
)
    : weaponTex(weapon),
      muzzleTex(muzzle),
      beamTex(beam),
      impactTex(impact),
      laserTimer(0.0f),
      maxLaserTime(0.15f),
      recoilStrength(recoilStrength),
      damage(damage) {
    recoilOffset = {0.0f, 0.0f};
    aimDir = {1.0f, 0.0f};
    aimAngle = 0.0f;
}

// Custom 2D Line-AABB intersection
bool CheckCollisionSegmentRec(Vector2 start, Vector2 end, Rectangle rec) {
    Vector2 p1 = {rec.x, rec.y};
    Vector2 p2 = {rec.x + rec.width, rec.y};
    Vector2 p3 = {rec.x + rec.width, rec.y + rec.height};
    Vector2 p4 = {rec.x, rec.y + rec.height};
    Vector2 colPoint;
    if (CheckCollisionLines(start, end, p1, p2, &colPoint)) return true;
    if (CheckCollisionLines(start, end, p2, p3, &colPoint)) return true;
    if (CheckCollisionLines(start, end, p3, p4, &colPoint)) return true;
    if (CheckCollisionLines(start, end, p4, p1, &colPoint)) return true;
    if (CheckCollisionPointRec(start, rec) || CheckCollisionPointRec(end, rec)) return true;
    return false;
}

void LaserAttackStrategy::Attack(Vector2 playerPos) {
    AudioManager::GetInstance().PlayRandomLaserGun();
    
    laserTimer = maxLaserTime;
    float barrelLength = weaponTex.width; 
    barrelTip = {
        playerPos.x + aimDir.x * barrelLength,
        playerPos.y + aimDir.y * barrelLength
    };

    // Raycast to find laserEndPoint (max 1000 pixels)
    float maxDistance = 1000.0f;
    float step = 8.0f;
    Vector2 collisionStart = { barrelTip.x - aimDir.x * 15.0f, barrelTip.y - aimDir.y * 15.0f };
    laserEndPoint = { barrelTip.x + aimDir.x * maxDistance, barrelTip.y + aimDir.y * maxDistance };
    recoilOffset = { -aimDir.x * recoilStrength, -aimDir.y * recoilStrength };
    
    LevelManager* lm = GameManager::GetInstance().GetLevelManager();
    if (lm) {
        for (float d = 0; d < maxDistance; d += step) {
            Vector2 checkPoint = { barrelTip.x + aimDir.x * d, barrelTip.y + aimDir.y * d };
            Rectangle pointRect = { checkPoint.x - 1.0f, checkPoint.y - 1.0f, 2.0f, 2.0f };
            // Pass true to ignore props so the laser pierces through boxes
            if (lm->IsSolidCollision(pointRect, true)) {
                laserEndPoint = checkPoint;
                break;
            }
        }
    }

    // Check intersection with all enemies and boxes
    const auto& entities = GameManager::GetInstance().GetLevelEntities();
    for (auto* entity : entities) {
        if (entity->GetObjectType() == GameObjectType::Enemy) {
            Enemy* e = static_cast<Enemy*>(entity);
            if (!e->IsEnabled()) continue;
            if (CheckCollisionSegmentRec(collisionStart, laserEndPoint, e->GetBoundingBox())) {
                e->TakeDamage(damage); // Piercing laser damage
                
                // Add Impact Effect visually
                GameManager::GetInstance().AddImpactEffect({e->GetPosition().x, e->GetPosition().y});
                if (owner) {
                    owner->OnHitEnemy(damage);
                }
            }
        } else if (entity->GetObjectType() == GameObjectType::Box) {
            Prop* p = static_cast<Prop*>(entity);
            if (CheckCollisionSegmentRec(collisionStart, laserEndPoint, p->GetBoundingBox())) {
                p->TakeDamage(damage);
                GameManager::GetInstance().AddImpactEffect({p->GetPosition().x, p->GetPosition().y});
            }
        }
    }
}

void LaserAttackStrategy::Update(float deltaTime) {
    recoilOffset.x += (0.0f - recoilOffset.x) * 10.0f * deltaTime;
    recoilOffset.y += (0.0f - recoilOffset.y) * 10.0f * deltaTime;
    
    if (laserTimer > 0.0f) {
        laserTimer -= deltaTime;
    }
}

void LaserAttackStrategy::Draw(Vector2 playerPos, bool facingLeft) {
    float drawAngle = aimAngle;
    Vector2 drawDir = aimDir;
    Vector2 weaponPos = { playerPos.x + recoilOffset.x, playerPos.y + recoilOffset.y };

    if (laserTimer > 0.0f) {
        drawDir = Vector2Subtract(laserEndPoint, weaponPos);
        if (Vector2Length(drawDir) > 0.0f) {
            drawDir = Vector2Normalize(drawDir);
            drawAngle = atan2f(drawDir.y, drawDir.x) * RAD2DEG;
        } else {
            drawDir = aimDir;
        }
    }

    Rectangle source = { 0.0f, 0.0f, (float)weaponTex.width, (float)weaponTex.height };
    if (facingLeft) {
        source.height = -source.height; 
    }

    Rectangle dest = { weaponPos.x, weaponPos.y, (float)weaponTex.width, (float)weaponTex.height };
    Vector2 origin = { 0.0f, (float)weaponTex.height / 2.0f };
    DrawTexturePro(weaponTex, source, dest, origin, drawAngle, WHITE);

    if (laserTimer > 0.0f) {
        // Calculate dynamic barrel tip based on recoil
        float barrelLength = weaponTex.width;
        Vector2 dynamicBarrelTip = {
            weaponPos.x + drawDir.x * barrelLength,
            weaponPos.y + drawDir.y * barrelLength
        };

        // Calculate animation frame for 2-frame assets
        float progress = 1.0f - (laserTimer / maxLaserTime);
        int frame = (int)(progress * 2);
        if (frame > 1) frame = 1;

        // Draw Muzzle (2 frames)
        if (muzzleTex.id != 0) {
            float frameW = (float)muzzleTex.width / 2.0f;
            Rectangle mzSource = { frame * frameW, 0, frameW, (float)muzzleTex.height };
            Rectangle mzDest = { dynamicBarrelTip.x, dynamicBarrelTip.y, frameW, (float)muzzleTex.height };
            Vector2 mzOrigin = { frameW / 2.0f, (float)muzzleTex.height / 2.0f };
            DrawTexturePro(muzzleTex, mzSource, mzDest, mzOrigin, drawAngle, WHITE);
        }

        // Draw Beam (2 frames)
        if (beamTex.id != 0) {
            float frameW = (float)beamTex.width / 2.0f;
            float dist = Vector2Distance(dynamicBarrelTip, laserEndPoint);
            Rectangle bmSource = { frame * frameW, 0, frameW, (float)beamTex.height };
            Rectangle bmDest = { dynamicBarrelTip.x, dynamicBarrelTip.y, dist, (float)beamTex.height };
            Vector2 bmOrigin = { 0.0f, (float)beamTex.height / 2.0f };
            DrawTexturePro(beamTex, bmSource, bmDest, bmOrigin, drawAngle, WHITE);
        }

        // Draw Impact at the end of the laser (2 frames)
        if (impactTex.id != 0) {
            float frameW = (float)impactTex.width / 2.0f;
            Rectangle imSource = { frame * frameW, 0, frameW, (float)impactTex.height };
            Rectangle imDest = { laserEndPoint.x, laserEndPoint.y, frameW, (float)impactTex.height };
            Vector2 imOrigin = { frameW / 2.0f, (float)impactTex.height / 2.0f };
            DrawTexturePro(impactTex, imSource, imDest, imOrigin, drawAngle, WHITE);
        }
    }
}
