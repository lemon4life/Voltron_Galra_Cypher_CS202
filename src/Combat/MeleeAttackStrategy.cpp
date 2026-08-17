#include "Combat/MeleeAttackStrategy.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Entities/Enemy.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Props/Prop.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/ParticleManager.h"
#include <algorithm>
#include <cstdlib>
#include "Core/Constants.h"

namespace {
    constexpr float MELEE_KNOCKBACK_FORCE = 350.0f;
}

MeleeAttackStrategy::MeleeAttackStrategy(
    Texture2D weapon,
    Texture2D att1,
    Texture2D att2,
    int lightDamage,
    int heavyDamage
)
    : weaponTex(weapon),
      attack1Tex(att1),
      attack2Tex(att2),
      comboStep(0),
      nextComboStep(1),
      frameTimer(0.0f),
      currentFrame(0),
      inputBuffered(false),
      kinematics(WeaponKinematicsType::Melee),
      lightDamage(lightDamage),
      heavyDamage(heavyDamage)
{
    aimDir = {1.0f, 0.0f};
    aimAngle = 0.0f;
    timePerFrame = 0.05f; // Fast melee swing (4 frames = 0.2s total)
}

void MeleeAttackStrategy::Attack(Vector2 playerPos) {
    lastPlayerPos = playerPos;
    if (comboStep == 0) {
        // Start combo
        comboStep = (rand() % 2) + 1;
        currentFrame = 0;
        frameTimer = 0.0f;
        inputBuffered = false;
        objectsHit.clear();
        AudioManager::GetInstance().PlayRandomSwordSlash();
        kinematics.ApplySwing(0.2f, 120.0f, (comboStep == 2));
    } else if (comboStep == 1 || comboStep == 2) {
        // Buffer the next hit if clicked during the active swing
        if (currentFrame >= 1) {
            inputBuffered = true;
        }
    }
}

void MeleeAttackStrategy::Update(float deltaTime) {
    kinematics.Update(deltaTime);
    if (comboStep == 0) return;

    frameTimer += deltaTime;
    if (frameTimer >= timePerFrame) {
        frameTimer -= timePerFrame;
        currentFrame++;

        // Process Hitbox Logic ON Impact Frames (frames 1 & 2, since 0-indexed)
        if (currentFrame == 1 || currentFrame == 2) {
            Vector2 playerPos = lastPlayerPos; // Set from Attack() or Draw()
            
            float hitboxWidth = 48.0f;
            float hitboxHeight = 64.0f;
            
            // Spawn AABB attached to Keith's front side
            float distanceOut = 24.0f;
            Vector2 hitCenter = { playerPos.x + aimDir.x * distanceOut, playerPos.y + aimDir.y * distanceOut };
            Rectangle hitbox = { hitCenter.x - hitboxWidth/2.0f, hitCenter.y - hitboxHeight/2.0f, hitboxWidth, hitboxHeight };
            
            const auto& entities = GameManager::GetInstance().GetLevelEntities();
            for (auto* entity : entities) {
                if (std::find(objectsHit.begin(), objectsHit.end(), entity)
                    != objectsHit.end()) {
                    continue;
                }
                if (!CheckCollisionRecs(hitbox, entity->GetBoundingBox())) {
                    continue;
                }

                int damage =
                    (comboStep == 1) ? lightDamage : heavyDamage;
                bool damagedObject = false;

                if (entity->GetObjectType() == GameObjectType::Enemy) {
                    Enemy& enemy = static_cast<Enemy&>(*entity);
                    if (!enemy.IsEnabled()) continue;
                    
                    enemy.TakeDamage(damage);
                    
                    enemy.ApplyKnockback(aimDir, MELEE_KNOCKBACK_FORCE);
                    damagedObject = true;
                    if (owner) owner->OnHitEnemy(damage);
                } else if (entity->GetObjectType() == GameObjectType::Box) {
                    Prop& box =
                        static_cast<Prop&>(*entity);
                    box.TakeDamage(damage);
                    damagedObject = true;
                }

                if (damagedObject) {
                    objectsHit.push_back(entity);
                    GameManager::GetInstance().AddImpactEffect(
                        entity->GetPosition()
                    );
                }
            }
        }

        // Handle animation end & combo transition
        if (currentFrame >= 4) {
            if (inputBuffered) {
                // Chain to a random attack
                comboStep = (rand() % 2) + 1;
                currentFrame = 0;
                inputBuffered = false;
                objectsHit.clear();
                AudioManager::GetInstance().PlayRandomSwordSlash();
                kinematics.ApplySwing(0.2f, 120.0f, (comboStep == 2));
            } else {
                // End combo
                comboStep = 0;
                currentFrame = 0;
                inputBuffered = false;
            }
        }
    }
}

void MeleeAttackStrategy::Draw(Vector2 playerPos, bool facingLeft) {
    lastPlayerPos = playerPos;
    float currentAngle = aimAngle + (facingLeft ? -kinematics.GetAngleOffset() : kinematics.GetAngleOffset());

    // 1. Draw Weapon
    if (weaponTex.id != 0) {
        Rectangle source = { 0.0f, 0.0f, (float)weaponTex.width, (float)weaponTex.height };
        if (facingLeft) {
            source.height = -source.height; 
        }
        Rectangle dest = { playerPos.x, playerPos.y, (float)weaponTex.width, (float)weaponTex.height };
        Vector2 origin = { 0.0f, (float)weaponTex.height / 2.0f }; // Pivot at hilt
        DrawTexturePro(weaponTex, source, dest, origin, currentAngle, WHITE);
    }

    // 2. Draw Effect ON TOP (triggered at +30 degrees approx -> frame 1)
    if (comboStep != 0 && currentFrame >= 1 && currentFrame <= 3) {
        Texture2D activeTex = (comboStep == 1) ? attack1Tex : attack2Tex;
        if (activeTex.id != 0) {
            int slashFrame = currentFrame - 1;
            float frameWidth = (float)activeTex.width / 3.0f; // Sword_slash_small is 3 frames
            float sourceHeight = (float)activeTex.height;
            
            if (facingLeft) sourceHeight = -sourceHeight;
            if (comboStep == 2) sourceHeight = -sourceHeight; // Reverse visual direction for upward swing

            Rectangle source = { slashFrame * frameWidth, 0.0f, frameWidth, sourceHeight };
            
            float distanceOut = weaponTex.id != 0 ? (float)weaponTex.width : 32.0f; // Calculate offset distance
            float rad = currentAngle * PI / 180.0f;
            Vector2 drawPos = { playerPos.x + std::cos(rad) * distanceOut, playerPos.y + std::sin(rad) * distanceOut };
            
            float scale = Constants::GLOBAL_SCALE; // Scale up the slash effect so it's proportional to the sword
            Rectangle dest = { drawPos.x, drawPos.y, frameWidth * scale, (float)activeTex.height * scale };
            Vector2 origin = { (frameWidth * scale) / 2.0f, ((float)activeTex.height * scale) / 2.0f };

            BeginBlendMode(BLEND_ADDITIVE);
            DrawTexturePro(activeTex, source, dest, origin, currentAngle, WHITE); // Align rotation exactly to sword
            EndBlendMode();
        }
    }
}
