#include "Entities/Player/PlayerDashState.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include "raymath.h"

void PlayerDashState::Enter(Paladin* player) {
    player->SetInvincible(true);
    player->SetDashTimer(0.3f); // 0.3s dash duration
    
    // Capture the dash direction.
    dashDirection = player->GetLastMoveDir();
    if (dashDirection.x == 0.0f && dashDirection.y == 0.0f) {
        dashDirection = player->IsFacingLeft() ? Vector2{ -1.0f, 0.0f } : Vector2{ 1.0f, 0.0f };
    }
    
    // Determine which dash texture to use (front vs back)
    bool cursorLeft = player->IsFacingLeft();
    bool dashFront = true;
    
    if (dashDirection.x > 0.0f) {
        dashFront = cursorLeft ? false : true; // Dashing right, if cursor is left, it's dashBack
    } else if (dashDirection.x < 0.0f) {
        dashFront = cursorLeft ? true : false; // Dashing left, if cursor is left, it's dashFront
    }
    
    Texture2D targetTex = dashFront ? player->GetDashFrontTexture() : player->GetDashBackTexture();
    
    // Fallback if texture not loaded
    if (targetTex.id == 0) {
        player->SetTexture(player->GetRunTexture());
        player->SetNumFrames(4);
    } else {
        player->SetTexture(targetTex);
        player->SetNumFrames(1);
    }
    player->ResetAnimation();
}

void PlayerDashState::Update(Paladin* player, float deltaTime) {
    float timer = player->GetDashTimer() - deltaTime;
    player->SetDashTimer(timer);

    // After 0.3 seconds, transition back to Idle
    if (timer <= 0.0f) {
        player->ChangeState(player->GetIdleState());
        return;
    }

    // Inertia-based gliding motion (slow-fast-slow)
    float dashDuration = 0.3f;
    float t = 1.0f - (timer / dashDuration);
    
    // Peak speed scaled by PI/2 to maintain the same total dash distance as linear 2.5x speed
    float peakSpeedMultiplier = 2.5f * (PI / 2.0f); 
    float dashSpeed = player->GetSpeed() * peakSpeedMultiplier * sinf(t * PI);
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
