#include "Entities/Player/PlayerAttackState.h"
#include "Entities/Player/Paladin.h"

void PlayerAttackState::Enter(Paladin* player) {
    // Lock movement for 0.2s
    attackTimer = 0.2f;

    // Trigger the attack
    player->Attack();
}

void PlayerAttackState::Update(Paladin* player, float deltaTime) {
    attackTimer -= deltaTime;
    
    // Animate during attack if needed
    player->UpdateAnimation(deltaTime);

    if (attackTimer <= 0.0f) {
        player->ChangeState(player->GetIdleState());
    }
}

void PlayerAttackState::Exit(Paladin* player) {
    // Attack finished
}
