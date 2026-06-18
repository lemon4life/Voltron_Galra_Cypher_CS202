#include "Combat/MeleeAttackStrategy.h"
#include "Core/GameManager.h"
#include "Entities/Enemy.h"
#include <algorithm>

MeleeAttackStrategy::MeleeAttackStrategy() : attackTimer(0.0f), isActive(false), hitbox{0, 0, 0, 0} {}

void MeleeAttackStrategy::Attack(Vector2 playerPos, bool facingLeft) {
    isActive = true;
    attackTimer = 0.2f; // Active for 0.2 seconds
    enemiesHit.clear(); // Reset hit enemies for new attack

    // The melee attack should be a 36x48 hitbox exactly left or right of the player
    float hitboxWidth = 36.0f;
    float hitboxHeight = 48.0f; 

    // playerPos is the CENTER of the player sprite.
    // To align the hitbox, we calculate from the player's edge (18px from center)
    // The hitbox's (x,y) in Raylib is its top-left corner. Top edge is playerPos.y - 24.
    if (facingLeft) {
        // Immediately to the left of the player
        hitbox = { playerPos.x - 18.0f - hitboxWidth, playerPos.y - 24.0f, hitboxWidth, hitboxHeight };
    } else {
        // Immediately to the right of the player
        hitbox = { playerPos.x + 18.0f, playerPos.y - 24.0f, hitboxWidth, hitboxHeight };
    }
}

void MeleeAttackStrategy::Update(float deltaTime) {
    if (isActive) {
        // Check collision with enemies
        for (auto* entity : GameManager::GetInstance().GetLevelEntities()) {
            if (Enemy* e = dynamic_cast<Enemy*>(entity)) {
                if (CheckCollisionRecs(hitbox, e->GetBoundingBox())) {
                    // Make sure we only hit each enemy once per swing
                    if (std::find(enemiesHit.begin(), enemiesHit.end(), e) == enemiesHit.end()) {
                        e->TakeDamage(50); // Keith's melee deals 50 damage
                        enemiesHit.push_back(e);
                    }
                }
            }
        }

        attackTimer -= deltaTime;
        if (attackTimer <= 0.0f) {
            isActive = false;
        }
    }
}

void MeleeAttackStrategy::Draw() {
    if (isActive) {
        // Draw the temporary hitbox in RED for debugging
        DrawRectangleRec(hitbox, RED);
    }
}
