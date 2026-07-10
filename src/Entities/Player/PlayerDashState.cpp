#include "Entities/Player/PlayerDashState.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include "raymath.h"

void PlayerDashState::Enter(Paladin* player) {
    player->SetInvincible(true);
    player->SetDashTimer(0.3f); // 0.3s dash duration
    
    // Capture the dash direction.
    // If the player was moving, lastMoveDir will have their last input direction.
    // If it's zero, default to the direction they are facing.
    dashDirection = player->GetLastMoveDir();
    if (dashDirection.x == 0.0f && dashDirection.y == 0.0f) {
        dashDirection = player->IsFacingLeft() ? Vector2{ -1.0f, 0.0f } : Vector2{ 1.0f, 0.0f };
    }
}

void PlayerDashState::Update(Paladin* player, float deltaTime) {
    float timer = player->GetDashTimer() - deltaTime;
    player->SetDashTimer(timer);

    // After 0.3 seconds, transition back to Idle
    if (timer <= 0.0f) {
        player->ChangeState(player->GetIdleState());
        return;
    }

    // Movement speed should be 2.5x the normal walk speed during dash
    float dashSpeed = player->GetSpeed() * 2.5f;
    LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();

    Vector2 currentPos = player->GetPosition();
    float levelWidth = GameManager::GetInstance().GetLevelWidth();
    float levelHeight = GameManager::GetInstance().GetLevelHeight();

    // Check X axis
    currentPos.x += dashDirection.x * dashSpeed * deltaTime;
    // Keep within level bounds
    if (currentPos.x < 0.0f) currentPos.x = 0.0f;
    if (currentPos.x > levelWidth) currentPos.x = levelWidth;
    
    player->SetPosition(currentPos);
    if (levelManager && levelManager->IsSolidCollision(player->GetBoundingBox())) {
        currentPos.x -= dashDirection.x * dashSpeed * deltaTime; // revert X
        player->SetPosition(currentPos);
    }

    // Check Y axis
    currentPos.y += dashDirection.y * dashSpeed * deltaTime;
    // Keep within level bounds
    if (currentPos.y < 0.0f) currentPos.y = 0.0f;
    if (currentPos.y > levelHeight) currentPos.y = levelHeight;
    
    player->SetPosition(currentPos);
    if (levelManager && levelManager->IsSolidCollision(player->GetBoundingBox())) {
        currentPos.y -= dashDirection.y * dashSpeed * deltaTime; // revert Y
        player->SetPosition(currentPos);
    }

    player->UpdateAnimation(deltaTime);
}

void PlayerDashState::Exit(Paladin* player) {
    player->SetInvincible(false);
    player->SetDashCooldown(1.0f); // 1.0s cooldown
}
