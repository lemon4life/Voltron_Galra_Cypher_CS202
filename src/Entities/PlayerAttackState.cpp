#include "Entities/PlayerAttackState.h"
#include "Entities/Player.h"

void PlayerAttackState::Enter(Player* player) {
    // Lock movement for 0.2s
    attackTimer = 0.2f;

    // Trigger the attack
    player->Attack();
}

void PlayerAttackState::Update(Player* player, float deltaTime) {
    attackTimer -= deltaTime;
    
    // Animate during attack if needed
    player->UpdateAnimation(deltaTime);

    if (attackTimer <= 0.0f) {
        player->ChangeState(player->GetIdleState());
    }
}

void PlayerAttackState::Exit(Player* player) {
    // Attack finished
}
