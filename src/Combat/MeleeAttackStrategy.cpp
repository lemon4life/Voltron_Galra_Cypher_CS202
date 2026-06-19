#include "Combat/MeleeAttackStrategy.h"
#include "Core/GameManager.h"
#include "Entities/Enemy.h"
#include <algorithm>

MeleeAttackStrategy::MeleeAttackStrategy(Texture2D tex) : weaponTex(tex), isAttacking(false), attackTimer(0.0f) {
    aimDir = {1.0f, 0.0f};
    aimAngle = 0.0f;
}

#include "Core/AudioManager.h"

void MeleeAttackStrategy::Attack(Vector2 playerPos) {
    isAttacking = true;
    attackTimer = 0.2f; // Active for 0.2 seconds
    enemiesHit.clear(); // Reset hit enemies for new attack

    float hitboxWidth = 54.0f; 
    float hitboxHeight = 48.0f; 

    // Offset the center of the hitbox along the aimDir vector
    float distanceOut = 20.0f;
    Vector2 hitCenter = { playerPos.x + aimDir.x * distanceOut, playerPos.y + aimDir.y * distanceOut };
    
    hitbox = { hitCenter.x - hitboxWidth/2.0f, hitCenter.y - hitboxHeight/2.0f, hitboxWidth, hitboxHeight };
    
    AudioManager::GetInstance().PlaySoundEffect("swing");
}

void MeleeAttackStrategy::Update(float deltaTime) {
    if (isAttacking) {
        attackTimer -= deltaTime;
        if (attackTimer <= 0.0f) {
            isAttacking = false;
        } else {
            // Check collisions with all entities using GameManager
            std::vector<GameObject*> entities = GameManager::GetInstance().GetLevelEntities();
            for (auto* entity : entities) {
                if (Enemy* enemy = dynamic_cast<Enemy*>(entity)) {
                    // If we haven't hit this enemy yet during this attack swing
                    if (std::find(enemiesHit.begin(), enemiesHit.end(), enemy) == enemiesHit.end()) {
                        if (CheckCollisionRecs(hitbox, enemy->GetBoundingBox())) {
                            // Keith deals exactly 50 damage
                            enemy->TakeDamage(50);
                            enemiesHit.push_back(enemy);
                        }
                    }
                }
            }
        }
    }
}

void MeleeAttackStrategy::Draw(Vector2 playerPos, bool facingLeft) {
    Rectangle source = { 0.0f, 0.0f, (float)weaponTex.width, (float)weaponTex.height };
    if (facingLeft) {
        source.height = -source.height; 
    }

    Rectangle dest = { playerPos.x, playerPos.y, (float)weaponTex.width, (float)weaponTex.height };
    Vector2 origin = { 0.0f, (float)weaponTex.height / 2.0f };

    // If attacking, maybe add a visual sweep effect here later, 
    // but for now, hold it exactly like Lance's gun.
    DrawTexturePro(weaponTex, source, dest, origin, aimAngle, WHITE);
}
