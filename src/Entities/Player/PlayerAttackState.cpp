#include "Entities/Player/PlayerAttackState.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/InputManager.h"
#include "raymath.h"

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
    Vector2 moveDir = InputManager::GetMovementVector();

    if (Vector2Length(moveDir) > 0.0f) {
        moveDir = Vector2Normalize(moveDir);
        player->SetLastMoveDir(moveDir);
        player->UpdateFootsteps(deltaTime);
        
        float speed = player->GetSpeed();
        player->MoveAgainstLevel(Vector2Scale(
            moveDir,
            speed * deltaTime
        ));
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
