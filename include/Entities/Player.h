#pragma once
#include "Character.h"
#include "PlayerState.h"
#include "Entities/PlayerDashState.h"

class Player : public Character {
private:
    IPlayerState* currentState;
    
    Texture2D texIdle;
    Texture2D texRun;

    // Animation specific
    int currentFrame;
    float frameTimer;
    float frameDuration;
    bool facingLeft;
    int numFrames; // Default 12

    // State instances to avoid allocating memory frequently
    PlayerIdleState idleState;
    PlayerRunState runState;
    PlayerDashState dashState; // Added dash state instance

    // Dash mechanic properties
    float dashCooldown;
    float dashTimer;
    bool isInvincible;
    Vector2 lastMoveDir; // Store last movement vector for locking dash direction

public:
    Player(Vector2 pos, Texture2D tIdle, Texture2D tRun);
    ~Player() override;

    void Update(float deltaTime) override;
    void Draw() override;

    void ChangeState(IPlayerState* newState);

    // Getters for states and textures
    PlayerIdleState* GetIdleState() { return &idleState; }
    PlayerRunState* GetRunState() { return &runState; }
    PlayerDashState* GetDashState() { return &dashState; }
    Texture2D GetIdleTexture() const { return texIdle; }
    Texture2D GetRunTexture() const { return texRun; }

    // Animation helpers
    void UpdateAnimation(float deltaTime);
    void SetNumFrames(int frames) { numFrames = frames; }
    void ResetAnimation() { currentFrame = 0; frameTimer = 0.0f; }
    void SetFacingLeft(bool left) { facingLeft = left; }
    bool IsFacingLeft() const { return facingLeft; }

    // Dash Getters and Setters
    float GetDashCooldown() const { return dashCooldown; }
    void SetDashCooldown(float cooldown) { dashCooldown = cooldown; }
    float GetDashTimer() const { return dashTimer; }
    void SetDashTimer(float timer) { dashTimer = timer; }
    bool IsInvincible() const { return isInvincible; }
    void SetInvincible(bool invincible) { isInvincible = invincible; }
    Vector2 GetLastMoveDir() const { return lastMoveDir; }
    void SetLastMoveDir(Vector2 dir) { lastMoveDir = dir; }
};
