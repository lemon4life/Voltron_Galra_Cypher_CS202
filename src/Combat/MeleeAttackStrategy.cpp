#include "Combat/MeleeAttackStrategy.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Entities/Enemy.h"
#include <algorithm>
#include <cstdlib>

MeleeAttackStrategy::MeleeAttackStrategy(Texture2D weapon, Texture2D att1, Texture2D att2) 
    : weaponTex(weapon), attack1Tex(att1), attack2Tex(att2), comboStep(0), nextComboStep(1), frameTimer(0.0f), currentFrame(0), inputBuffered(false) 
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
        enemiesHit.clear();
        AudioManager::GetInstance().PlaySoundEffect("swing");
    } else if (comboStep == 1 || comboStep == 2) {
        // Buffer the next hit if clicked during the active swing
        if (currentFrame >= 1) {
            inputBuffered = true;
        }
    }
}

void MeleeAttackStrategy::Update(float deltaTime) {
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
                if (Enemy* enemy = dynamic_cast<Enemy*>(entity)) {
                    if (std::find(enemiesHit.begin(), enemiesHit.end(), enemy) == enemiesHit.end()) {
                        if (CheckCollisionRecs(hitbox, enemy->GetBoundingBox())) {
                            // Deal damage
                            int dmg = (comboStep == 1) ? 35 : 45; // 2nd hit hits harder
                            enemy->TakeDamage(dmg);
                            
                            // Instant positional knockback
                            Vector2 enemyPos = enemy->GetPosition();
                            enemyPos.x += aimDir.x * 20.0f;
                            enemyPos.y += aimDir.y * 20.0f;
                            
                            // Basic bounds check (if we had levelManager access here, we'd check walls)
                            // For simplicity, just push them.
                            enemy->SetPosition(enemyPos);
                            
                            enemiesHit.push_back(enemy);
                            
                            // Visual impact effect
                            GameManager::GetInstance().AddImpactEffect({enemy->GetPosition().x, enemy->GetPosition().y});
                        }
                    }
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
                enemiesHit.clear();
                AudioManager::GetInstance().PlaySoundEffect("swing");
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
    if (comboStep == 0) {
        if (weaponTex.id != 0) {
            Rectangle source = { 0.0f, 0.0f, (float)weaponTex.width, (float)weaponTex.height };
            if (facingLeft) {
                source.height = -source.height; 
            }
            Rectangle dest = { playerPos.x, playerPos.y, (float)weaponTex.width, (float)weaponTex.height };
            Vector2 origin = { 0.0f, (float)weaponTex.height / 2.0f };
            DrawTexturePro(weaponTex, source, dest, origin, aimAngle, WHITE);
        }
        return;
    }

    Texture2D activeTex = (comboStep == 1) ? attack1Tex : attack2Tex;
    
    // Safety check
    if (activeTex.id == 0) return;

    // Sprite sheet has 4 frames
    float frameWidth = (float)activeTex.width / 4.0f;
    
    Rectangle source = { currentFrame * frameWidth, 0.0f, frameWidth, (float)activeTex.height };
    if (facingLeft) {
        source.width = -source.width; 
    }

    // Since this is a swing animation, we'll draw it directly centered on the player 
    // with some offset so it sweeps in front of them.
    float distanceOut = 16.0f;
    Vector2 drawPos = { playerPos.x + aimDir.x * distanceOut, playerPos.y + aimDir.y * distanceOut };
    
    Rectangle dest = { drawPos.x, drawPos.y, frameWidth, (float)activeTex.height };
    Vector2 origin = { frameWidth / 2.0f, (float)activeTex.height / 2.0f };

    DrawTexturePro(activeTex, source, dest, origin, aimAngle, WHITE);
}
