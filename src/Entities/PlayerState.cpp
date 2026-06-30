#include "Entities/PlayerState.h"
#include "Entities/Player.h"
#include "Core/LevelManager.h"
#include "raymath.h"
#include "Core/GameManager.h"

// --- PlayerIdleState ---
void PlayerIdleState::Enter(Player* player) {
    player->SetTexture(player->GetIdleTexture());
    player->SetNumFrames(12); // Sprite sheet has 12 frames
    player->ResetAnimation();
}

void PlayerIdleState::Update(Player* player, float deltaTime) {
    // Check for Attack Input ('J' or Left Mouse Button)
    if (GameManager::GetInstance().GetState() == GameState::PLAYING) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            player->ChangeState(player->GetAttackState());
            return;
        }

        // Check for Dash Input (Spacebar and cooldown off)
        if (IsKeyPressed(KEY_SPACE) && player->GetDashCooldown() <= 0.0f) {
            player->ChangeState(player->GetDashState());
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

void PlayerIdleState::Exit(Player* player) {
    // Nothing specific needed
}

// --- PlayerRunState ---
void PlayerRunState::Enter(Player* player) {
    player->SetTexture(player->GetRunTexture());
    player->SetNumFrames(12); // Sprite sheet has 12 frames
    player->ResetAnimation();
}

void PlayerRunState::Update(Player* player, float deltaTime) {
    // Check for Attack Input ('J' or Left Mouse Button)
    if (GameManager::GetInstance().GetState() == GameState::PLAYING) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            player->ChangeState(player->GetAttackState());
            return;
        }

        // Check for Dash Input (Spacebar and cooldown off)
        if (IsKeyPressed(KEY_SPACE) && player->GetDashCooldown() <= 0.0f) {
            player->ChangeState(player->GetDashState());
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
    float levelWidth = GameManager::GetInstance().GetLevelWidth();
    float levelHeight = GameManager::GetInstance().GetLevelHeight();

    // Check X axis
    currentPos.x += moveDir.x * speed * deltaTime;
    // Keep within level bounds
    if (currentPos.x < 0.0f) currentPos.x = 0.0f;
    if (currentPos.x > levelWidth) currentPos.x = levelWidth;
    
    player->SetPosition(currentPos);
    if (levelManager && levelManager->IsSolidCollision(player->GetBoundingBox())) {
        currentPos.x -= moveDir.x * speed * deltaTime;
        player->SetPosition(currentPos);
    }

    // Check Y axis
    currentPos.y += moveDir.y * speed * deltaTime;
    // Keep within level bounds
    if (currentPos.y < 0.0f) currentPos.y = 0.0f;
    if (currentPos.y > levelHeight) currentPos.y = levelHeight;

    player->SetPosition(currentPos);
    if (levelManager && levelManager->IsSolidCollision(player->GetBoundingBox())) {
        currentPos.y -= moveDir.y * speed * deltaTime;
        player->SetPosition(currentPos);
    }

    player->UpdateAnimation(deltaTime);
}

void PlayerRunState::Exit(Player* player) {
    // Nothing specific needed
}
