#include "Entities/PlayerDashState.h"
#include "Entities/Player.h"
#include "Core/GameManager.h"
#include "raymath.h"

void PlayerDashState::Enter(Player* player) {
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

void PlayerDashState::Update(Player* player, float deltaTime) {
    float timer = player->GetDashTimer() - deltaTime;
    player->SetDashTimer(timer);

    // After 0.3 seconds, transition back to Idle
    if (timer <= 0.0f) {
        player->ChangeState(player->GetIdleState());
        return;
    }

    // Movement speed should be 2.5x the normal walk speed during dash
    float dashSpeed = player->GetSpeed() * 2.5f;
    const auto& walls = GameManager::GetInstance().GetLevelEntities();

    Vector2 currentPos = player->GetPosition();

    // Check X axis
    currentPos.x += dashDirection.x * dashSpeed * deltaTime;
    player->SetPosition(currentPos);
    if (player->CheckCollision(walls)) {
        currentPos.x -= dashDirection.x * dashSpeed * deltaTime; // revert X
        player->SetPosition(currentPos);
    }

    // Check Y axis
    currentPos.y += dashDirection.y * dashSpeed * deltaTime;
    player->SetPosition(currentPos);
    if (player->CheckCollision(walls)) {
        currentPos.y -= dashDirection.y * dashSpeed * deltaTime; // revert Y
        player->SetPosition(currentPos);
    }

    player->UpdateAnimation(deltaTime);
}

void PlayerDashState::Exit(Player* player) {
    player->SetInvincible(false);
    player->SetDashCooldown(1.0f); // 1.0s cooldown
}
