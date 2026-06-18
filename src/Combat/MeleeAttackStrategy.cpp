#include "Combat/MeleeAttackStrategy.h"

MeleeAttackStrategy::MeleeAttackStrategy() : attackTimer(0.0f), isActive(false), hitbox{0, 0, 0, 0} {}

void MeleeAttackStrategy::Attack(Vector2 playerPos, bool facingLeft) {
    isActive = true;
    attackTimer = 0.2f; // Active for 0.2 seconds

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
