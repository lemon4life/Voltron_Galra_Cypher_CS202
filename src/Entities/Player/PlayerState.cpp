#include "Entities/Player/PlayerState.h"
#include "Entities/Player/Paladin.h"
#include "raymath.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/InputManager.h"
#include <cstdio>

// --- PlayerIdleState ---
void PlayerIdleState::Enter(Paladin* player) {
    player->SetTexture(player->GetIdleTexture());
    player->SetNumFrames(4); // Sprite sheet has 4 frames
    player->ResetAnimation();
}

void PlayerIdleState::Update(Paladin* player, float deltaTime) {
    // Check for Attack Input ('J' or Left Mouse Button)
    if (GameManager::GetInstance().GetState() == GameState::GAMEPLAY) {
        if (InputManager::IsAttackPressed() && !player->IsDoingUltimate()) {
            player->ChangeState(player->GetAttackState());
            return;
        }

        // Check for Dash Input (Spacebar/K and cooldown off)
        if (InputManager::IsDashPressed() && player->GetDashCooldown() <= 0.0f) {
            player->ChangeState(player->GetDashState());
            return;
        }

        // Check for Parry Input
        if (InputManager::IsParryPressed() && player->GetDashCooldown() <= 0.0f && !player->IsDoingUltimate()) {
            player->ChangeState(player->GetParryState());
            return;
        }

        if (InputManager::IsSkillPressed()) {
            player->UseSkill();
        }

        if (InputManager::IsUltimatePressed()) {
            player->UseUltimate();
        }
    }

    // Check for input to transition to Run state
    if (Vector2LengthSqr(InputManager::GetMovementVector()) > 0.0f) {
        player->ChangeState(player->GetRunState());
        return;
    }
    player->UpdateAnimation(deltaTime);
}

void PlayerIdleState::Exit(Paladin* player) {
    // Nothing specific needed
}

// --- PlayerRunState ---
void PlayerRunState::Enter(Paladin* player) {
    player->ResetParryCount();
    player->SetTexture(player->GetRunTexture());
    player->SetNumFrames(8); // Updated to 8 frames
    player->ResetAnimation();
}

void PlayerRunState::Update(Paladin* player, float deltaTime) {
    // Check for Attack Input ('J' or Left Mouse Button)
    if (GameManager::GetInstance().GetState() == GameState::GAMEPLAY) {
        if (InputManager::IsAttackPressed() && !player->IsDoingUltimate()) {
            player->ChangeState(player->GetAttackState());
            return;
        }

        // Check for Dash Input
        if (InputManager::IsDashPressed() && player->GetDashCooldown() <= 0.0f) {
            player->ChangeState(player->GetDashState());
            return;
        }

        // Check for Parry Input
        if (InputManager::IsParryPressed() && player->GetDashCooldown() <= 0.0f && !player->IsDoingUltimate()) {
            player->ChangeState(player->GetParryState());
            return;
        }

        if (InputManager::IsSkillPressed()) {
            player->UseSkill();
        }

        if (InputManager::IsUltimatePressed()) {
            player->UseUltimate();
        }
    }

    Vector2 moveDir = InputManager::GetMovementVector();

    // Check if we stopped moving
    if (moveDir.x == 0.0f && moveDir.y == 0.0f) {
        player->ChangeState(player->GetIdleState());
        return;
    }

    // Normalize diagonal movement and store last direction
    if (Vector2Length(moveDir) > 0.0f) {
        moveDir = Vector2Normalize(moveDir);
        player->SetLastMoveDir(moveDir);
        player->UpdateFootsteps(deltaTime);
    }

    float speed = player->GetSpeed();
    player->MoveAgainstLevel(Vector2Scale(
        moveDir,
        speed * deltaTime
    ));

    player->UpdateAnimation(deltaTime);
}

void PlayerRunState::Exit(Paladin* player) {
    // Nothing specific needed
}

// --- PlayerParryState ---
void PlayerParryState::Enter(Paladin* player) {
    player->SetTexture(player->GetParryTexture());
    player->SetNumFrames(1);
    player->ResetAnimation();
    player->SetParrying(true);
}

void PlayerParryState::Update(Paladin* player, float deltaTime) {
    if (player->IsAutoParrying()) {
        player->DecrementAutoParryDuration(deltaTime);
        if (player->GetAutoParryDurationTimer() <= 0.0f) {
            player->ChangeState(player->GetIdleState());
            return;
        }
        // Break out if movement, attack, or dash is pressed explicitly
        if (Vector2LengthSqr(InputManager::GetMovementVector()) > 0.0f || 
            InputManager::IsAttackPressed() || InputManager::IsDashPressed()) {
            player->ChangeState(player->GetIdleState());
            return;
        }
    } else {
        if (!InputManager::IsParryDown()) {
            player->ChangeState(player->GetIdleState());
        }
    }
}

void PlayerParryState::Exit(Paladin* player) {
    player->SetParrying(false);
    player->SetAutoParry(false);
}

// ----------------------------------------------------
// PlayerDownState
// ----------------------------------------------------
void PlayerDownState::Enter(Paladin* player) {
    player->SetTexture(player->GetDownTexture());
    player->SetNumFrames(1);
    player->ResetAnimation();
    bounceTimer = 0.4f;
    initialY = player->GetPosition().y;
}

void PlayerDownState::Update(Paladin* player, float deltaTime) {
    if (bounceTimer > 0.0f) {
        bounceTimer -= deltaTime;
        
        float yOffset = 0.0f;
        if (bounceTimer > 0.2f) {
            float progress = (bounceTimer - 0.2f) / 0.2f; // 1.0 to 0.0
            yOffset = -15.0f * progress; 
        }
        
        player->SetRenderOffsetY(yOffset);
        
        if (bounceTimer <= 0.0f) {
            player->SetRenderOffsetY(0.0f);
            
            TeamManager* teamManager = player->GetTeamManager();
            if (teamManager) {
                if (teamManager->IsTeamDead()) {
                    GameManager::GetInstance().SetState(GameState::GAME_OVER);
                } else {
                    teamManager->SwapDueToDeath();
                }
            }
        }
    }
}

void PlayerDownState::Exit(Paladin* player) {
    // Reset to initial Y if needed, though they shouldn't exit until reset anyway
}
