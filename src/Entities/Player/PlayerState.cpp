#include "Entities/Player/PlayerState.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/LevelManager.h"
#include "raymath.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
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
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            player->ChangeState(player->GetAttackState());
            return;
        }

        // Check for Dash Input (Spacebar and cooldown off)
        if (IsKeyPressed(KEY_SPACE) && player->GetDashCooldown() <= 0.0f) {
            player->ChangeState(player->GetDashState());
            return;
        }

        // Check for Parry Input ('F')
        if (IsKeyPressed(KEY_F) && player->GetDashCooldown() <= 0.0f) {
            player->ChangeState(player->GetParryState());
            return;
        }
    }

    // Check for input to transition to Run state
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_A) || IsKeyDown(KEY_W) || IsKeyDown(KEY_S)) {
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
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            player->ChangeState(player->GetAttackState());
            return;
        }

        // Check for Dash Input (Spacebar and cooldown off)
        if (IsKeyPressed(KEY_SPACE) && player->GetDashCooldown() <= 0.0f) {
            player->ChangeState(player->GetDashState());
            return;
        }

        // Check for Parry Input ('F')
        if (IsKeyPressed(KEY_F) && player->GetDashCooldown() <= 0.0f) {
            player->ChangeState(player->GetParryState());
            return;
        }
    }

    Vector2 moveDir = { 0.0f, 0.0f };

    if (IsKeyDown(KEY_D)) moveDir.x += 1.0f;
    if (IsKeyDown(KEY_A)) moveDir.x -= 1.0f;
    if (IsKeyDown(KEY_W)) moveDir.y -= 1.0f;
    if (IsKeyDown(KEY_S)) moveDir.y += 1.0f;

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

    // Update position with axis-separated collision logic
    Vector2 currentPos = player->GetPosition();
    LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
    float speed = player->GetSpeed();

    // Get level bounds
    Rectangle bounds = {0, 0, GameManager::GetInstance().GetLevelWidth(), GameManager::GetInstance().GetLevelHeight()};
    if (levelManager) {
        bounds = levelManager->GetLevelBounds();
    }

    // Check X axis
    currentPos.x += moveDir.x * speed * deltaTime;
    // Keep within level bounds
    if (bounds.width > 0) {
        if (currentPos.x < bounds.x) currentPos.x = bounds.x;
        if (currentPos.x > bounds.x + bounds.width) currentPos.x = bounds.x + bounds.width;
    }



    player->SetPosition(currentPos);
    if (levelManager && levelManager->IsSolidCollision(player->GetCollisionBox())) {
        currentPos.x -= moveDir.x * speed * deltaTime;
        player->SetPosition(currentPos);
    }

    // Check Y axis
    currentPos.y += moveDir.y * speed * deltaTime;
    // Keep within level bounds
    if (bounds.height > 0) {
        if (currentPos.y < bounds.y) currentPos.y = bounds.y;
        if (currentPos.y > bounds.y + bounds.height) currentPos.y = bounds.y + bounds.height;
    }

    player->SetPosition(currentPos);
    if (levelManager && levelManager->IsSolidCollision(player->GetCollisionBox())) {
        currentPos.y -= moveDir.y * speed * deltaTime;
        player->SetPosition(currentPos);
    }

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
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_D) ||
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE)) {
            player->ChangeState(player->GetIdleState());
            return;
        }
    } else {
        if (!IsKeyDown(KEY_F)) {
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
