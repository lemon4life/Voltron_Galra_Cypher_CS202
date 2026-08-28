#pragma once

#include "Entities/GameObject.h"
#include "Core/Manager/AssetManager.h"

class Paladin;

class EnhanceMachine : public GameObject {
private:
    Texture2D machineTexture;
    int currentFrame = 0;
    int numFrames = 4;
    float frameTimer = 0.0f;
    float frameDuration = 0.14f; // ~7 FPS

    bool isPlayerInRange = false;

public:
    EnhanceMachine(Vector2 pos);
    ~EnhanceMachine() override = default;

    void Update(float deltaTime) override;
    void Draw() override;

    Rectangle GetBoundingBox() const override;
    Rectangle GetCollisionBox() const override;
    bool IsSolidNavigationObstacle() const override { return true; }

    bool IsPlayerInRange() const { return isPlayerInRange; }
    void OnInteract(Paladin* player);
};
