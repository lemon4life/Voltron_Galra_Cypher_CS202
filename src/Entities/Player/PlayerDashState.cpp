#include "Entities/Player/PlayerDashState.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/GameManager.h"
#include "raymath.h"

/// Prepares this state when it becomes active.
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
    trailTimer = 0.0f; // Reset trail timer at the start of every dash
}

/// Advances this component's state for the current frame.
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
    player->MoveAgainstLevel(Vector2Scale(
        dashDirection,
        dashSpeed * deltaTime
    ));

    // Emit a sprite ghost every 0.05s for the dash trail
    trailTimer -= deltaTime;
    if (trailTimer <= 0.0f) {
        trailTimer = 0.05f;
        GameManager::GetInstance().GetEffectManager().SpawnDashTrail(
            player->GetPosition(),
            player->GetCurrentSourceRect(),
            player->GetTexture(),
            0.0f,
            player->IsFacingLeft()
        );
    }

    player->UpdateAnimation(deltaTime);
}

/// Cleans up this state before control moves elsewhere.
void PlayerDashState::Exit(Paladin* player) {
    player->SetInvincible(false);
    player->SetDashCooldown(0.7f); // 1.0s cooldown
}
