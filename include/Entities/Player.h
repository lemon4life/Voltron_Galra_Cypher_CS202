#pragma once
#include "Character.h"
#include "PlayerState.h"

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

public:
    Player(Vector2 pos, Texture2D tIdle, Texture2D tRun);
    ~Player() override;

    void Update(float deltaTime) override;
    void Draw() override;

    void ChangeState(IPlayerState* newState);

    // Getters for states and textures
    PlayerIdleState* GetIdleState() { return &idleState; }
    PlayerRunState* GetRunState() { return &runState; }
    Texture2D GetIdleTexture() const { return texIdle; }
    Texture2D GetRunTexture() const { return texRun; }

    // Animation helpers
    void UpdateAnimation(float deltaTime);
    void SetNumFrames(int frames) { numFrames = frames; }
    void ResetAnimation() { currentFrame = 0; frameTimer = 0.0f; }
    void SetFacingLeft(bool left) { facingLeft = left; }
    bool IsFacingLeft() const { return facingLeft; }
};
