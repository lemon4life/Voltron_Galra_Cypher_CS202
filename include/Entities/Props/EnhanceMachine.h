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
    /// Creates a EnhanceMachine instance from the supplied configuration.
    EnhanceMachine(Vector2 pos);
    /// Releases resources owned by this EnhanceMachine instance.
    ~EnhanceMachine() override = default;

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;

    /// Returns the current bounding box.
    Rectangle GetBoundingBox() const override;
    /// Returns the current collision box.
    Rectangle GetCollisionBox() const override;
    /// Reports whether the solid navigation obstacle condition is satisfied.
    bool IsSolidNavigationObstacle() const override { return true; }

    /// Reports whether the player in range condition is satisfied.
    bool IsPlayerInRange() const { return isPlayerInRange; }
    /// Handles the interact event.
    void OnInteract(Paladin* player);
};
