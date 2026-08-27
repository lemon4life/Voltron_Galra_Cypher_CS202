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
    Coin(Vector2 pos, Vector2 initialVelocity = {0.0f, 0.0f});
    ~Coin() override = default;

    void Update(float deltaTime) override;
    void Draw() override;

    bool IsCollected() const { return isCollected; }
    Rectangle GetBoundingBox() const override {
        return { position.x - 4.0f, position.y - 4.5f, 8.0f, 9.0f };
    }
};
