#include "Entities/Player.h"

Player::Player(Vector2 pos, Texture2D tIdle, Texture2D tRun)
    : Character(pos, 150.0f, 100, tIdle), // Default to idle texture, 150 speed, 100 HP
      texIdle(tIdle),
      texRun(tRun),
      currentFrame(0),
      frameTimer(0.0f),
      frameDuration(0.1f), // 10 fps animation speed
      facingLeft(false),
      numFrames(12),
      dashCooldown(0.0f),
      dashTimer(0.0f),
      isInvincible(false),
      lastMoveDir{1.0f, 0.0f} // Initialize pointing right
{
    currentState = &idleState;
    currentState->Enter(this);
}

Player::~Player() {
    if (currentState) {
        currentState->Exit(this);
    }
}

void Player::Update(float deltaTime) {
    // Decrement dash cooldown over time
    if (dashCooldown > 0.0f) {
        dashCooldown -= deltaTime;
        if (dashCooldown < 0.0f) {
            dashCooldown = 0.0f;
        }
    }

    if (currentState) {
        currentState->Update(this, deltaTime);
    }
}

void Player::ChangeState(IPlayerState* newState) {
    if (currentState != newState) {
        if (currentState) currentState->Exit(this);
        currentState = newState;
        if (currentState) currentState->Enter(this);
    }
}

void Player::UpdateAnimation(float deltaTime) {
    frameTimer += deltaTime;
    if (frameTimer >= frameDuration) {
        frameTimer -= frameDuration;
        currentFrame = (currentFrame + 1) % numFrames;
    }
}

void Player::Draw() {
    // Calculate the frame width dynamically based on the active texture
    // and the number of frames (both sprite sheets have 12 frames, but idle is 32px wide and run is 36px wide per frame)
    const float frameWidth = (float)texture.width / numFrames;
    const float frameHeight = 48.0f;

    // Source rectangle based on current frame and facing direction
    // A negative width in DrawTexturePro flips the texture horizontally natively in Raylib
    Rectangle sourceRec = {
        (float)currentFrame * frameWidth,
        0.0f,
        facingLeft ? -frameWidth : frameWidth,
        frameHeight
    };

    // Destination rectangle centered exactly on the player's position
    Rectangle destRec = {
        position.x,
        position.y,
        frameWidth,
        frameHeight
    };

    // Origin for rotation/scaling, set to center of the sprite
    Vector2 origin = { frameWidth / 2.0f, frameHeight / 2.0f };

    // Apply a gray tint to indicate invincibility during a dash
    Color tint = isInvincible ? GRAY : WHITE;

    DrawTexturePro(texture, sourceRec, destRec, origin, 0.0f, tint);
}
