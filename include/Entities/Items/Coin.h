#pragma once

#include "Entities/GameObject.h"
#include "Core/Manager/AssetManager.h"
#include "raylib.h"

class Paladin;
class TeamManager;

class Coin : public GameObject {
private:
    Vector2 velocity;
    int currentFrame = 0;
    int numFrames = 4;
    float frameDuration = 0.12f;
    float frameTimer = 0.0f;

    bool isCollected = false;
    float burstTimer = 0.25f;
    float magnetSpeed = 320.0f;
    float friction = 0.92f;

    Texture2D texture = {};

public:
    /// Creates a Coin instance from the supplied configuration.
    Coin(Vector2 pos, Vector2 initialVelocity = {0.0f, 0.0f});
    /// Releases resources owned by this Coin instance.
    ~Coin() override = default;

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;

    /// Reports whether the collected condition is satisfied.
    bool IsCollected() const { return isCollected; }
    /// Returns the current bounding box.
    Rectangle GetBoundingBox() const override {
        return { position.x - 4.0f, position.y - 4.5f, 8.0f, 9.0f };
    }
};
