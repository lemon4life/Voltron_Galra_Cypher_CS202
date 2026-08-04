#include "Entities/Player/PlayerAttackState.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"

void PlayerAttackState::Enter(Paladin* player) {
    // Lock movement for 0.2s
    attackTimer = player->GetAttackCooldown();

    // Trigger the attack
    player->Attack();
    player->ResetParryCount();
}

#include "Combat/MeleeAttackStrategy.h"

void PlayerAttackState::Update(Paladin* player, float deltaTime) {
    attackTimer -= deltaTime;
    
    // Animate during attack if needed
    player->UpdateAnimation(deltaTime);

    // If player has a MeleeAttackStrategy, we only transition back to idle when comboStep == 0.
    // For other strategies (ranged), we fall back to the timer.
    bool canExit = false;
    if (MeleeAttackStrategy* melee = dynamic_cast<MeleeAttackStrategy*>(player->GetCurrentWeapon())) {
        if (melee->GetComboStep() == 0) {
            canExit = true;
        }
    } else {
        if (attackTimer <= 0.0f) {
            canExit = true;
        }
    }

    // Check movement input during attack
    Vector2 moveDir = { 0.0f, 0.0f };
    if (IsKeyDown(KEY_D)) moveDir.x += 1.0f;
    if (IsKeyDown(KEY_A)) moveDir.x -= 1.0f;
    if (IsKeyDown(KEY_W)) moveDir.y -= 1.0f;
    if (IsKeyDown(KEY_S)) moveDir.y += 1.0f;

    if (Vector2Length(moveDir) > 0.0f) {
        moveDir = Vector2Normalize(moveDir);
        player->SetLastMoveDir(moveDir);
        player->UpdateFootsteps(deltaTime);
        
        Vector2 currentPos = player->GetPosition();
        Vector2 prevPos = currentPos;
        LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
        float speed = player->GetSpeed();
        
        Rectangle bounds = {0, 0, GameManager::GetInstance().GetLevelWidth(), GameManager::GetInstance().GetLevelHeight()};
        if (levelManager) {
            bounds = levelManager->GetLevelBounds();
        }

        // Check X axis
        currentPos.x += moveDir.x * speed * deltaTime;
        if (bounds.width > 0) {
            if (currentPos.x < bounds.x) currentPos.x = bounds.x;
            if (currentPos.x > bounds.x + bounds.width) currentPos.x = bounds.x + bounds.width;
        }
        player->SetPosition(currentPos);
        if (levelManager && levelManager->IsSolidCollision(player->GetCollisionBox())) {
            currentPos.x = prevPos.x;
            player->SetPosition(currentPos);
        }

        prevPos = player->GetPosition();

        // Check Y axis
        currentPos.y += moveDir.y * speed * deltaTime;
        if (bounds.height > 0) {
            if (currentPos.y < bounds.y) currentPos.y = bounds.y;
            if (currentPos.y > bounds.y + bounds.height) currentPos.y = bounds.y + bounds.height;
        }
        player->SetPosition(currentPos);
        if (levelManager && levelManager->IsSolidCollision(player->GetCollisionBox())) {
            currentPos.y = prevPos.y;
            player->SetPosition(currentPos);
        }
    }

    if (canExit) {
        if (Vector2Length(moveDir) > 0.0f) {
            player->ChangeState(player->GetRunState());
        } else {
            player->ChangeState(player->GetIdleState());
        }
    }
}


void PlayerAttackState::Exit(Paladin* player) {
    // Attack finished
}