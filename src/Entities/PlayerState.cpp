#include "Entities/PlayerState.h"
#include "Entities/Player.h"
#include "raymath.h"

// --- PlayerIdleState ---
void PlayerIdleState::Enter(Player* player) {
    player->SetTexture(player->GetIdleTexture());
    player->SetNumFrames(12); // Sprite sheet has 12 frames
    player->ResetAnimation();
}

void PlayerIdleState::Update(Player* player, float deltaTime) {
    // Check for input to transition to Run state
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN)) {
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
    Vector2 moveDir = { 0.0f, 0.0f };

    if (IsKeyDown(KEY_RIGHT)) moveDir.x += 1.0f;
    if (IsKeyDown(KEY_LEFT))  moveDir.x -= 1.0f;
    if (IsKeyDown(KEY_UP))    moveDir.y -= 1.0f;
    if (IsKeyDown(KEY_DOWN))  moveDir.y += 1.0f;

    // Check if we stopped moving
    if (moveDir.x == 0.0f && moveDir.y == 0.0f) {
        player->ChangeState(player->GetIdleState());
        return;
    }

    // Determine facing direction for texture flipping
    if (moveDir.x < 0.0f) player->SetFacingLeft(true);
    else if (moveDir.x > 0.0f) player->SetFacingLeft(false);

    // Normalize diagonal movement
    if (Vector2Length(moveDir) > 0.0f) {
        moveDir = Vector2Normalize(moveDir);
    }

    // Update position
    Vector2 currentPos = player->GetPosition();
    currentPos.x += moveDir.x * player->GetSpeed() * deltaTime;
    currentPos.y += moveDir.y * player->GetSpeed() * deltaTime;
    player->SetPosition(currentPos);

    player->UpdateAnimation(deltaTime);
}

void PlayerRunState::Exit(Player* player) {
    // Nothing specific needed
}
